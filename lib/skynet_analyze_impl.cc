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
#include <volk/volk.h>
#include "skynet_wireshark_sink_impl.h"
#include "bachelor/constants.h"

#define debug false

namespace gr {
  namespace bachelor {




    skynet_analyze::sptr
    skynet_analyze::make(int samp_rate, float symbols_per_second, int threshold)
    {
      return gnuradio::get_initial_sptr
        (new skynet_analyze_impl(samp_rate, symbols_per_second,threshold));
    }

    /*
     * The private constructor
     */
    skynet_analyze_impl::skynet_analyze_impl(int samp_rate, float sampels_per_symbol, int threshold)
      : gr::sync_block("skynet_analyze",
              gr::io_signature::make(2,3, sizeof(float)),
              gr::io_signature::make(0,0,0))
    {
        message_port_register_out(PMT_STRING_OUT);
        d_rec_buf = new std::vector<float>;
        d_rssi_buf = new std::vector<float>;
        this->d_sample_rate = samp_rate;
        this->d_sampels_per_symbol = sampels_per_symbol;
        d_threshold = threshold;
        d_packageFound = false;
        clear_buffer();
        d_puffer = -2;
    }

    /*
     * Our virtual destructor.
     */
    skynet_analyze_impl::~skynet_analyze_impl()
    {
    }

    void
    skynet_analyze_impl::clear_buffer()
    {
        d_rec_buf->clear();
        d_rssi_buf->clear();
    }

    void
    skynet_analyze_impl::add_item_to_buffer(float item)
    {
        add_item_to_buffer(item,d_rec_buf);
    }

    void
    skynet_analyze_impl::add_item_to_buffer(float item, std::vector<float> *buffer)
    {
        buffer->push_back(item);
    }

    void
    skynet_analyze_impl::handle_package()
    {

        if(d_rec_buf->size()< d_sampels_per_symbol*32) // Nichtmal ein Byte enhalten, nutzlos
            return;
        struct timeval packet_end_time;
        gettimeofday(&packet_end_time,NULL);




        uint start = 0;
        uint end = d_rec_buf->size();
#if debug
        FILE* file;
        file = fopen("./debug_analyze.csv","a");
        for(int i = 0;i<end;i++) {
            fprintf(file,"%f;",d_rec_buf->at(i));
        }
        fprintf(file,"~");
#endif

        //Durchschnitt der Werte der Präambel, alle Werte größer -> 1, alle kleiner -> 0
        float average = 0;
        for(std::vector<float>::iterator it = d_rec_buf->begin()+start+10*d_sampels_per_symbol;it !=  d_rec_buf->begin()+start+48*d_sampels_per_symbol; ++it )
            average +=*it;
        average /= 38*d_sampels_per_symbol;

        average = 0;
        uint firstMax = 0;
        if(d_rec_buf->size()> 10*d_sampels_per_symbol) {
            firstMax = std::max_element(d_rec_buf->begin()+5*d_sampels_per_symbol, d_rec_buf->begin()+10*d_sampels_per_symbol) - d_rec_buf->begin();
            firstMax = firstMax%((uint)d_sampels_per_symbol);
        }

        //std::cout<< "Packet length: "<<end-start<<" Symbol per Second " << symb_per_second<<" Average: " << average << " First Maximum: "<< firstMax << " SecondMax: " << it-rec_buf->begin() << std::endl;

        int preamble_start = firstMax;


        int pos = 0;
        bool sync_word_found= false;
        uint32_t sync_word = 0xaaaa2dd4; // 0xaaaa wird benötigt, da noch vor der Präambel Bits so erkannt werden, dass 0x2dd4 nur einen bitfehler aufweist
        unsigned char preamble_byte = 0xaa;
        int preamble_start_pos = -1;
        uint32_t data = 0;
        for(int i = preamble_start; i< end; i+= d_sampels_per_symbol) { // Suche nach dem Syncword
            pos ++;

            data = (data << 1) | d_rec_buf->at(i) >= average;

            if(preamble_start_pos <= -1) { // noch kein 0xaa vorhanden
                uint32_t wrong_bits = ((unsigned char)data) ^ preamble_byte;
                uint32_t  wrong_bits_count =0;
                volk_32u_popcnt(&wrong_bits_count, wrong_bits);
                if(wrong_bits_count <= d_threshold) {
                    preamble_start_pos = pos-8;
                }
            }

            uint32_t wrong_bits = data ^ sync_word;
            uint32_t  wrong_bits_count =0;
            volk_32u_popcnt(&wrong_bits_count, wrong_bits);

            if(wrong_bits_count <= d_threshold) {
                sync_word_found = true;
                pos -= 16;
                break;
            }
        }
        if(!sync_word_found)
            return;

        int shift = 0;
        if(sync_word_found) {
            shift = (pos -64)*d_sampels_per_symbol;
        } else  if(preamble_start_pos > 0){
            shift = preamble_start_pos *d_sampels_per_symbol;
        }
        preamble_start += shift;



        std::vector<char>bits;

        if(preamble_start< 0) {
            while (preamble_start< 0) {
                preamble_start+=d_sampels_per_symbol;
                bits.push_back(0);
            }

        }

        float rssi = 0;
        int offset = 0;
        float ende = std::min(((float)d_rec_buf->size()-10),preamble_start + std::floor((end-preamble_start)/d_sampels_per_symbol)*d_sampels_per_symbol);
        for(float i= preamble_start;i < ende ;i+= d_sampels_per_symbol) {
            offset = i/12000;
#if debug
                fprintf(file,"%d;",(int) i+offset);
#endif

            bits.push_back(d_rec_buf->at(i+offset)>=average);
            rssi+= d_rssi_buf->at(i+offset);
        }
        rssi/= bits.size();
        if(bits.size() < 8) { // nichtmal ein Byte empfangen, kann verworfen werden.
            return;
        }

#if debug
        fprintf(file,"~%f~0~",average);
        for(std::vector<char>::iterator it = bits.begin();it !=  bits.end(); ++it )
            fprintf(file,"%d;",*it);
        fprintf(file,"~%d",shift);
        fprintf(file,"\n");
        fclose(file);
#endif
        int packetLength =0;
        if(bits.size() >=96) {
            const char* bits_data =  bits.data();
            size_t length;
            bits_data += 80;
            unsigned char *a = to_byte_array(bits_data,16,length);
            packetLength =a[0];
            packetLength = (packetLength<<8) + a[1];
        }

        pmt::pmt_t header = pmt::make_dict();
        header = pmt::dict_add(header,PMT_STRING_TIME_START_SEC,pmt::mp(d_packet_start_time.tv_sec));
        header = pmt::dict_add(header,PMT_STRING_TIME_START_USEC,pmt::mp(d_packet_start_time.tv_usec));
        header = pmt::dict_add(header,PMT_STRING_TIME_STOP_SEC,pmt::mp(packet_end_time.tv_sec));
        header = pmt::dict_add(header,PMT_STRING_TIME_STOP_USEC,pmt::mp(packet_end_time.tv_usec));
        header = pmt::dict_add(header,PMT_STRING_RSSI,pmt::mp(rssi));

        pmt::pmt_t vec = pmt::make_u8vector(std::min(96+packetLength*8,(int) bits.size()),0);
        size_t vec_size;
        u_int8_t *elements = pmt::u8vector_writable_elements(vec,vec_size);
        for(int i = 0;i< vec_size;i++)
            elements[i] = bits.at(i);
        pmt::pmt_t message = pmt::cons(header,vec);
        message_port_pub(PMT_STRING_OUT, message);
//        std::cout << "OLDPacket received: Size: " << ((float)vec_size)/(float)8 << " Byte, RSSI: " << rssi << "  " << average<< std::endl;
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
            if(trig[i] >= 0.9 || d_puffer >0) {
                if(!d_packageFound) // Anfang der Übertragung
                    gettimeofday(&d_packet_start_time,NULL);
                add_item_to_buffer(in[i]);
                if(rssi != NULL)
                    add_item_to_buffer(rssi[i], d_rssi_buf);
                d_packageFound = true;
                if(d_puffer > 0)
                    d_puffer--;
            } else if(trig[i] == 0 && d_packageFound) { // noch 2 Bit Puffer hinzufügen

                if(d_puffer == -2) {

                    d_puffer = 16*d_sampels_per_symbol;
                }
            } if(d_puffer == 0) {
                d_packageFound = false;
                handle_package();
                clear_buffer();
                d_puffer = -2;
            }
        }
        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

    unsigned char*
    skynet_analyze_impl::to_byte_array(const char* bitArray, const size_t& inLength, size_t& outputLength) {
        outputLength = inLength/8;
        unsigned char* array =  new unsigned char[outputLength];
        uint bitIndex;
        for(int i = 0;i<outputLength;i+=1) {
            bitIndex = i*8;
            array[i] = bitArray[bitIndex]<<7 | bitArray[bitIndex+1]<< 6| bitArray[bitIndex+2]<< 5| bitArray[bitIndex+3]<< 4| bitArray[bitIndex+4]<< 3| bitArray[bitIndex+5]<<2 | bitArray[bitIndex+6]<<1 | bitArray[bitIndex+7];
        }
        return array;
    }

  } /* namespace bachelor */
} /* namespace gr */
