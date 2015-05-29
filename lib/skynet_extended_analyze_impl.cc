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
#include "skynet_extended_analyze_impl.h"
#include <volk/volk.h>
#include <stdio.h>
#include <iostream>
#include "bachelor/constants.h"

#define debug false

namespace gr {
  namespace bachelor {

    skynet_extended_analyze::sptr
    skynet_extended_analyze::make()
    {
      return gnuradio::get_initial_sptr
        (new skynet_extended_analyze_impl());
    }

    /*
     * The private constructor
     */
    skynet_extended_analyze_impl::skynet_extended_analyze_impl()
      : gr::sync_block("skynet_extended_analyze",
                       gr::io_signature::make(1,3,sizeof(float)),
                       gr::io_signature::make(0,0,0))
    {
        message_port_register_out(PMT_STRING_OUT);
        d_sampels_per_symbol = 2;
        d_rssi_buf = new std::vector<float>;
        d_real_buf = new std::vector<float>;
        d_packageFound = false;
        d_min_buffer_size = 0;
        d_threshold = 1;
        d_data = 0;
        d_mag_data = 0;
        d_last_package_end_pos = 0;


        d_rssi_buffer = (float*) malloc(sizeof(float)*80*d_sampels_per_symbol);
        memset(d_rssi_buffer,0,80*sizeof(float)*d_sampels_per_symbol);

        d_real_buffer = (float*) malloc(sizeof(float)*80*d_sampels_per_symbol);
        memset(d_real_buffer,0,80*sizeof(float)*d_sampels_per_symbol);

        clear_buffer();
        d_average = 0;
    }

    /*
     * Our virtual destructor.
     */
    skynet_extended_analyze_impl::~skynet_extended_analyze_impl()
    {
        free(d_rssi_buffer);
        free(d_real_buffer);
        delete d_rssi_buf;
        delete d_real_buf;
    }

    void
    skynet_extended_analyze_impl::clear_buffer()
    {
        d_rssi_buf->clear();
        d_real_buf->clear();
        d_packageFound = false;
        d_min_buffer_size = 0;
        d_package_start_pos = 0;
        d_preamble_pos = -1;
        d_detected =0;
        d_average = 0;
    }

    int
    skynet_extended_analyze_impl::search_sync_word(int preamble_start) {
        int pos_0 = 0, pos_1 = 0;
        bool sync_word_found= false;
        int sync_word_pos[3] = {-1,-1,-1};
        int sync_word_shift[3] = {-1,-1,-1};
        uint64_t sync_word = 0x2dd4;
        uint64_t mask = 0;
        mask = (~mask) >>48;
        mask = 0xFFFF;
        uint64_t data_0 =0, data_1 = 0;
        for(int i = preamble_start; i< d_real_buf->size(); i++) { // Suche nach dem Syncword
            if(i%2 == 0) {
                data_0 = (data_0 << 1) | d_real_buf->at(i)>= d_average;
                pos_0++;
            } else {
                data_1 = (data_1 << 1) | d_real_buf->at(i)>= d_average;
                pos_1++;
            }
            uint64_t wrong_bits_0 = (data_0 ^ sync_word) & mask;
            uint64_t  wrong_bits_count_0 =0;
            volk_64u_popcnt(&wrong_bits_count_0, wrong_bits_0);

            uint64_t wrong_bits_1 = (data_1 ^ sync_word) & mask;
            uint64_t  wrong_bits_count_1 =0;
            volk_64u_popcnt(&wrong_bits_count_1, wrong_bits_1);

            if(wrong_bits_count_0 <= d_threshold  && pos_0 >= 16) {
                if(sync_word_pos[wrong_bits_count_0] == -1) {
                    sync_word_pos[wrong_bits_count_0] = pos_0-16;
                    sync_word_shift[wrong_bits_count_0] = 0;
                }
                sync_word_found = true;
            }
            if(wrong_bits_count_1 <= d_threshold && pos_1 >= 16) {
                if(sync_word_pos[wrong_bits_count_1] == -1) {
                    sync_word_pos[wrong_bits_count_1] = pos_1-16;
                    sync_word_shift[wrong_bits_count_1] = 1;
                }
                sync_word_found = true;
            }
        }
        if(!sync_word_found) {
            return NO_SYNC_FOUND;
        }

        int shift = 0;
        if(sync_word_found) {

            for(int i = 2; i>=0; i--) {
                if(sync_word_pos[i] != -1) {
                    shift = (sync_word_pos[i] -64) *d_sampels_per_symbol;
                    shift += sync_word_shift[i];
                }
            }
        }
        return shift;
    }

    int
    skynet_extended_analyze_impl::search_preamble() {


        int preamble_start = 0;

        int shift = search_sync_word(preamble_start);
        if(shift == NO_SYNC_FOUND)
            return NO_SYNC_FOUND;

        preamble_start += shift;

        //Synchronisation am Peak sicherstellen
        uint firstMax = std::max_element(d_real_buf->begin()+preamble_start+50*d_sampels_per_symbol, d_real_buf->begin()+preamble_start+60*d_sampels_per_symbol) - d_real_buf->begin();
        firstMax = (preamble_start+ firstMax)%((uint)d_sampels_per_symbol);
        preamble_start+= firstMax ;



        return preamble_start;
    }

    int skynet_extended_analyze_impl::handle_package()
    {

        if(d_last_package_end_pos > d_package_start_pos) {
            clear_buffer();
            return -4;
        }
        if( d_min_buffer_size > d_real_buf->size()) {
            return MORE_BITS_NEEDED;
        }

        if(d_preamble_pos == -1) {
            d_preamble_pos = search_preamble();
            if(d_preamble_pos == NO_SYNC_FOUND){
                clear_buffer();
                return NO_SYNC_FOUND;
            }
        }


        struct timeval packet_end_time;
        gettimeofday(&packet_end_time,NULL);

        int preamble_pos = d_preamble_pos;
        std::vector<char>bits;

        if(preamble_pos< 0) {
            // 0en einfügen, damit es für wireshark analyse passt
            while (preamble_pos< 0) {
                preamble_pos+=d_sampels_per_symbol;
                bits.push_back(0);
            }

        }
#if debug
        FILE* file;
        file = fopen("./debug_test2.csv","a");
        for(int i = 0;i<d_real_buf->size();i++) {
            fprintf(file,"%f;",d_real_buf->at(i));
        }
        fprintf(file,"~");
#endif

        float rssi = 0;
        int offset = 0;
        float ende = std::min((float) (d_real_buf->size()-d_sampels_per_symbol),(float)( preamble_pos+(96-bits.size())*d_sampels_per_symbol));
        for(float i= preamble_pos;i < ende ;i+= d_sampels_per_symbol) {
#if debug
                fprintf(file,"%d;",(int) i+offset);
#endif
            bits.push_back(d_real_buf->at(i)>= d_average);
            rssi+= d_rssi_buf->at(i);
        }

        if(bits.size()<96) {
            d_min_buffer_size+=(97- bits.size())*d_sampels_per_symbol;
#if debug
        fprintf(file,"~0~0~");
        for(std::vector<char>::iterator it = bits.begin();it !=  bits.end(); ++it )
            fprintf(file,"%d;",*it);
        fprintf(file,"~0");
        fprintf(file,"\n");
        fclose(file);
#endif
            return -4;
        }

        const char* bits_data =  bits.data();
        size_t length;
        bits_data += 64;
        unsigned char *bytes = to_byte_array(bits_data,16,length);
        uint16_t syncword =bytes[0];
        syncword = (syncword<<8) + bytes[1];
        uint16_t wrong_bits = syncword ^0x2dd4;
        uint32_t  wrong_bits_count =0;
        volk_32u_popcnt(&wrong_bits_count, wrong_bits);



        bits_data =  bits.data();

        bits_data += 80;
        unsigned char *a = to_byte_array(bits_data,16,length);
        int packetLength =a[0];
        packetLength = (packetLength<<8) + a[1];



        if(packetLength> 4096 || wrong_bits_count > d_threshold) { // Nicht korrekt empfangen, kann verworfen werden
#if debug
            fprintf(file,"~0~0~");
            for(std::vector<char>::iterator it = bits.begin();it !=  bits.end(); ++it )
                fprintf(file,"%d;",*it);
            fprintf(file,"~0");
            fprintf(file,"\n");
            fclose(file);
#endif
            clear_buffer();
            return NO_SYNC_FOUND;
        }



        float start = ende;
        ende =  start+packetLength*8*d_sampels_per_symbol;
        d_min_buffer_size = ende+50;
        if(d_min_buffer_size > d_real_buf->size()) {
#if debug
        fprintf(file,"~0~0~");
        for(std::vector<char>::iterator it = bits.begin();it !=  bits.end(); ++it )
            fprintf(file,"%d;",*it);
        fprintf(file,"~0");
        fprintf(file,"\n");
        fclose(file);
#endif
            return MORE_BITS_NEEDED;
        }
        for(float i= start;i < ende ;i+= d_sampels_per_symbol) {
            offset = i/12000;
#if debug
                fprintf(file,"%d;",(int) i+offset);
#endif
            bits.push_back(d_real_buf->at(i+offset) >= d_average);
            rssi+= d_rssi_buf->at(i+offset);
        }
        rssi/= bits.size();




        pmt::pmt_t header = pmt::make_dict();
        header = pmt::dict_add(header,PMT_STRING_TIME_START_SEC,pmt::mp(d_packet_start_time.tv_sec));
        header = pmt::dict_add(header,PMT_STRING_TIME_START_USEC,pmt::mp(d_packet_start_time.tv_usec));
        header = pmt::dict_add(header,PMT_STRING_TIME_STOP_SEC,pmt::mp(packet_end_time.tv_sec));
        header = pmt::dict_add(header,PMT_STRING_TIME_STOP_USEC,pmt::mp(packet_end_time.tv_usec));
        header = pmt::dict_add(header,PMT_STRING_RSSI,pmt::mp(rssi));

        pmt::pmt_t vec = pmt::make_u8vector(96+packetLength*8,0);
        size_t vec_size;
        u_int8_t *elements = pmt::u8vector_writable_elements(vec,vec_size);
        for(int i = 0;i< vec_size;i++)
            elements[i] = bits.at(i);
        pmt::pmt_t message = pmt::cons(header,vec);
        message_port_pub(PMT_STRING_OUT, message);
        d_last_package_end_pos = d_package_start_pos + ende;
        std::cout << "Packet received: Size: " << vec_size/8 << " Byte, RSSI: " << rssi  << std::endl;

#if debug
        fprintf(file,"~0~%d~",d_detected);
        for(std::vector<char>::iterator it = bits.begin();it !=  bits.end(); ++it )
            fprintf(file,"%d;",*it);
        fprintf(file,"~0");
        fprintf(file,"\n");
        fclose(file);
#endif

        clear_buffer();
        return ende;
    }

    uint
    skynet_extended_analyze_impl::copy_to_buffer(const float* real,const float* rssi,uint start, uint length){
        uint needed_bits = d_min_buffer_size-d_real_buf->size()+start;
        uint count = 0;
        for(int i = start;i< std::min(length,needed_bits);i++){
            d_real_buf->push_back(real[i]);
            d_rssi_buf->push_back(rssi[i]);
            count ++;
        }
        return count;
    }

    int
    skynet_extended_analyze_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        const float *real =(const float *) input_items[0];
        const float *trigger =(const float *) input_items[1];
        const float *rssi =(const float *) input_items[2];


        uint consumed = 0;
        int result = 0;
        u_int64_t itemsRead = nitems_read(0);
        if(d_packageFound){
            do{
                consumed += copy_to_buffer(real,rssi,consumed,noutput_items);
                result = handle_package();
            } while(consumed < noutput_items && result == MORE_BITS_NEEDED);
        }

        uint32_t blaa = 0b11;
        uint32_t mask= 0b11;
        bool wasGefunden= false;
        for(int i = 0; i< noutput_items; i++) { // Suche nach einem Anfang

            // Erkennung anhand der Preamble
            d_data = (d_data << 1) | real[i] >= d_average;

            if((d_data & mask) == blaa) {
                uint32_t data_0 =0, data_1 = 0;
                uint32_t mask2 =0xFFFFFF;
                uint32_t blubbb = 0xAAAAAA;
                int end = std::min(i+4+30*d_sampels_per_symbol,noutput_items);
                for(int k = std::max(i-2, 0); k <  end;k++) {
                    if(k%2 == 0) {
                        data_0 = (data_0 << 1) | real[k] >= 0;
                    } else {
                        data_1 = (data_1 << 1) | real[k] >= 0;
                    }

                    uint32_t wrong_bits_0 = (data_0 ^ blubbb) & mask2;
                    uint32_t  wrong_bits_count_0 =0;
                    volk_32u_popcnt(&wrong_bits_count_0, wrong_bits_0);

                    uint32_t wrong_bits_1 = (data_1 ^ blubbb) & mask2;
                    uint32_t  wrong_bits_count_1 =0;
                    volk_32u_popcnt(&wrong_bits_count_1, wrong_bits_1);
                    if(wrong_bits_count_0 <= 2 || wrong_bits_count_1 <= 2) {
                        wasGefunden =true;
                        break;
                    }

                }

            }

            //Erkennung, an empfangener Leistung

            d_mag_data = (d_mag_data << 1) | trigger[i]>0.9f;
            uint64_t mag_data_high_count =0;
            volk_64u_popcnt(&mag_data_high_count,d_mag_data);


            if(wasGefunden || mag_data_high_count >= 63) {

                if(d_packageFound) {
                    d_min_buffer_size =(itemsRead+i+200*d_sampels_per_symbol) -d_package_start_pos ;
                }else {
                    int start_pos = 0;
                    gettimeofday(&d_packet_start_time,NULL);
                    if(i > 300) {
                        start_pos= i- 300;
                    }
                    d_package_start_pos = itemsRead+start_pos;
                    d_detected = i-start_pos;

                    d_min_buffer_size = 200*d_sampels_per_symbol;
                    d_min_buffer_size += (i-start_pos); // Backlog
                    if(i< 300) {
                         copy_to_buffer(d_real_buffer,d_rssi_buffer,0,80*d_sampels_per_symbol);
                         d_min_buffer_size+= 80*d_sampels_per_symbol;
                    }
                    d_packageFound = true;
                    consumed = start_pos + copy_to_buffer(real,rssi,start_pos,noutput_items);
                }
                int result = 0;
                int added = 0;
                do{
                    added = copy_to_buffer(real,rssi,consumed,noutput_items);
                    consumed += added;
                    if(added> 0)
                        result = handle_package();
                } while(consumed < noutput_items && result == MORE_BITS_NEEDED);
                if(result > 0) { // Damit ein Paket nicht mehr mals erkannt wird
                    int shift = d_last_package_end_pos -itemsRead-64;
                    if(shift > i) {
                        i= shift;
                        d_data = 0;
                        d_mag_data =0;
                    }

                }
            }
        }

        uint len = std::min(80,noutput_items);
        if(len < 80) {
            uint copy_size = std::min((uint) 80*d_sampels_per_symbol ,len*2); // Muss doppelt in den Buffer passen.
            uint actual_copy_length = copy_size/2;
            uint dest_pos = 80*d_sampels_per_symbol -copy_size;
            uint src_pos = 80*d_sampels_per_symbol-actual_copy_length;
            memcpy(d_rssi_buffer+dest_pos,d_rssi_buffer+src_pos, actual_copy_length*sizeof(float));//Bestehende Daten im Puffer nach vorne kopieren
            memcpy(d_real_buffer+dest_pos,d_real_buffer+src_pos, actual_copy_length*sizeof(float));
        }
        uint src_pos = std::max(noutput_items-80*d_sampels_per_symbol,0);
        uint dest_pos = (80*d_sampels_per_symbol-len);

        memcpy(d_rssi_buffer+ dest_pos,rssi + src_pos,len *sizeof(float));
        memcpy(d_real_buffer+ dest_pos,real + src_pos,len *sizeof(float));

        return noutput_items;
    }

    unsigned char*
    skynet_extended_analyze_impl::to_byte_array(const char* bitArray, const size_t& inLength, size_t& outputLength) {
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


