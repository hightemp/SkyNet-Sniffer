/* -*- c++ -*- */
/*
 * Copyright 2015 Fabian Härer.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "skynet_analyze_impl.h"
#include <numeric>
#include <stdio.h>
#include "skynet_wireshark_sink_impl.h"
#include "bachelor/constants.h"

#define debug false

namespace gr {
  namespace bachelor {

    skynet_analyze::sptr
    skynet_analyze::make(int samp_rate, float symbols_per_second)
    {
      return gnuradio::get_initial_sptr
        (new skynet_analyze_impl(samp_rate, symbols_per_second));
    }

    /*
     * The private constructor
     */
    skynet_analyze_impl::skynet_analyze_impl(int samp_rate, float symbols_per_second)
      : gr::sync_block("skynet_analyze",
              gr::io_signature::make(2,3, sizeof(float)),
              gr::io_signature::make(0,0,0))
    {
        message_port_register_out(PMT_STRING_OUT);
        rec_buf = new std::vector<float>;
        rssi_buf = new std::vector<float>;
        this->sample_rate = samp_rate;
        this->symb_per_second = symbols_per_second;
        packageFound = false;
        clear_buffer();

    }

    /*
     * Our virtual destructor.
     */
    skynet_analyze_impl::~skynet_analyze_impl()
    {
    }

    void skynet_analyze_impl::clear_buffer(){
        rec_buf->clear();
        rssi_buf->clear();
    }

    void skynet_analyze_impl::add_item_to_buffer(float item) {
        add_item_to_buffer(item,rec_buf);
    }

    void skynet_analyze_impl::add_item_to_buffer(float item, std::vector<float> *buffer){
        buffer->push_back(item);
    }

    void skynet_analyze_impl::handlePackage(){

        struct timeval packet_end_time;
        gettimeofday(&packet_end_time,NULL);




        uint start = 0;
        uint end = rec_buf->size();
        int blank = 50e-6*sample_rate;// Skip first 50 us.
        start = start+blank;
#if debug
        FILE* file;
        file = fopen("./debug.csv","a");
        for(int i = 0;i<end;i++) {
            fprintf(file,"%f;",rec_buf->at(i));
        }
        fprintf(file,"~");
#endif
        float average = ((float)std::accumulate(rec_buf->begin()+start+2*symb_per_second,rec_buf->begin()+start+78*symb_per_second,0))/((float)76*symb_per_second);
        uint firstMax = 0, secondMax = 0;
        for(uint i = start+5;i< (end-1) && rec_buf->at(i) <= rec_buf->at(i+1) ; i++) {
            firstMax = i;
        }
        std::vector<float>::iterator it =  std::max_element(rec_buf->begin()+firstMax,rec_buf->begin()+firstMax+symb_per_second*2.5);
        firstMax = it-rec_buf->begin() - symb_per_second*2;




        std::cout<< "Packet length: "<<end-start<<" Symbpl per Second " << symb_per_second<<" Average: " << average << " First Maximum: "<< firstMax << " SecondMax: " << it-rec_buf->begin() << std::endl;

        int preamble_location = -1;
        for(int i = 0;i<end;i++) {

            if(rec_buf->at(i) >= average && preamble_location == -1) {
                preamble_location = i;

            }
        }
        preamble_location = firstMax;

        bool search = false;
        float startP=0;
        float lastSwitch = preamble_location;
        float count = -1;
        for(int i = 20;i<end;i++){
            if ((rec_buf->at(i) >= average ) == search) {
                if (startP == 0)
                    startP = i;
                search = !search;
                lastSwitch = i;
                count+=1;
                if (count >= 60)
                    break;
            }
          }



        std::vector<char>bits;
        float rssi = 0;
        int offset = 0;
        if(preamble_location< 0)
            preamble_location = 0;
        for(float i= preamble_location;i < rec_buf->size() &&  i <std::floor((end-start)/symb_per_second)*symb_per_second;i+= symb_per_second) {
            offset = i/10000;
#if debug
                fprintf(file,"%d;",(int) i+offset);
#endif

            bits.push_back(rec_buf->at(i+offset)>=average);
            rssi+= rssi_buf->at(i+offset);

        }

        rssi/= bits.size();
        std::cout << "RSSI " << rssi << " Packet size:" << bits.size() << std::endl;

        int shift = 0;
        if (bits.size() >=1 && bits.at(0) == 0)
            shift = 1;

        for(int i = 2; i< bits.size(); i++) {
            if(!(bits.at(i-2) == 1 && bits.at(i-1) == 0 && bits.at(i) == 1))
                shift++;
            else if(bits.at(i-2) == 1 && bits.at(i-1) == 0)
                break;
        }

        for(shift = 0;shift+69 < bits.size() &&( bits.at(shift+68) == 0 || bits.at(shift +69) == 0); shift++) {
        }
        std::cout<< "Shift " <<  shift << std::endl;

#if debug
        fprintf(file,"~%f~%d~",average, shift);
        for(std::vector<char>::iterator it = bits.begin()+shift;it !=  bits.end(); ++it )
            fprintf(file,"%d;",*it);
        fprintf(file,"\n");
        fclose(file);
#endif
//        if (bits.size()>=2 && !(bits.at(0) == 1 && bits.at(1)== 0))
//            shift = 1;

        pmt::pmt_t header = pmt::make_dict();
        header = pmt::dict_add(header,PMT_STRING_TIME_START_SEC,pmt::mp(packet_start_time.tv_sec));
        header = pmt::dict_add(header,PMT_STRING_TIME_START_USEC,pmt::mp(packet_start_time.tv_usec));
        header = pmt::dict_add(header,PMT_STRING_TIME_STOP_SEC,pmt::mp(packet_end_time.tv_sec));
        header = pmt::dict_add(header,PMT_STRING_TIME_STOP_USEC,pmt::mp(packet_end_time.tv_usec));
        header = pmt::dict_add(header,PMT_STRING_RSSI,pmt::mp(rssi));

        pmt::pmt_t vec = pmt::make_u8vector(bits.size()-shift,0);
        size_t vec_size;
        u_int8_t *elements = pmt::u8vector_writable_elements(vec,vec_size);
        for(int i = 0;i< vec_size;i++)
            elements[i] = bits.at(i+shift);
        pmt::pmt_t message = pmt::cons(header,vec);
        message_port_pub(PMT_STRING_OUT, message);
    }

    int
    skynet_analyze_impl::work(int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items)
    {
        const float *trig = (const float *) input_items[0];
        const float *in = (const float *) input_items[1];
        const float *rssi = NULL;
        if(input_items.size()==3)
            rssi = (const float *) input_items[2];

        //packageFound = false; // True wenn der Trigger einmal 1 war, zum Erkennen des PacketEndes
        for(uint i = 0;i< noutput_items;i++) {
            if(trig[i] >= 0.9) {
                if(!packageFound) // Anfang der Übertragung
                    gettimeofday(&packet_start_time,NULL);
                add_item_to_buffer(in[i]);
                if(rssi != NULL)
                    add_item_to_buffer(rssi[i], rssi_buf);
                packageFound = true;
            } else if(trig[i] == 0 && packageFound) {
                packageFound = false;
                handlePackage();
                clear_buffer();
            }
        }
        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace bachelor */
} /* namespace gr */
