/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
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
#include "skynet_wireshark_sink_impl.h"
#include "bachelor/constants.h"

namespace gr {
  namespace bachelor {

    skynet_wireshark_sink::sptr
    skynet_wireshark_sink::make()
    {
      return gnuradio::get_initial_sptr
        (new skynet_wireshark_sink_impl());
    }

    /*
     * The private constructor
     */
    skynet_wireshark_sink_impl::skynet_wireshark_sink_impl()
        : gr::sync_block("SKYNET Wireshark Sink",
                 gr::io_signature::make(0, 0, 0),
                 gr::io_signature::make(1, 1, sizeof(uint8_t)))
    {
        message_port_register_in(PMT_STRING_IN);
        p_msg_len = sizeof(pcap_hdr_s);
        p_msg_offset = 0;
        p_msg = (char*) malloc(sizeof(pcap_hdr_s));
        pcap_hdr_s* hdr = (pcap_hdr_s*) p_msg;
        hdr->magic_number = 0xa1b2c3d4;
        hdr->version_major = 2;
        hdr->version_minor = 4;
        hdr->thiszone=0;
        hdr->sigfigs=0;
        hdr->snaplen=65536;
        hdr->network=148;
    }

    /*
     * Our virtual destructor.
     */
    skynet_wireshark_sink_impl::~skynet_wireshark_sink_impl()
    {
        if(p_msg != NULL)
            free(p_msg);
    }

    uint8_t *
    skynet_wireshark_sink_impl::toByteArray(const uint8_t* bitArray, const size_t& inLength, size_t& outputLength) {
        outputLength = inLength/8;
        uint8_t* array = (uint8_t*) new uint8_t[outputLength];
        uint bitIndex;
        for(int i = 0;i<outputLength;i+=1) {
            bitIndex = i*8;
            array[i] = bitArray[bitIndex]<<7 | bitArray[bitIndex+1]<< 6| bitArray[bitIndex+2]<< 5| bitArray[bitIndex+3]<< 4| bitArray[bitIndex+4]<< 3| bitArray[bitIndex+5]<<2 | bitArray[bitIndex+6]<<1 | bitArray[bitIndex+7];
        }
        return array;
    }

    int
    skynet_wireshark_sink_impl::work(int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items)
    {
        uint8_t *out = (uint8_t *) output_items[0];
        if(p_msg_len == 0) {  // Wenn der Nachrichtenbuffer leer ist, sonst wird zuerst der verbleibende Puffer gesendet
            pmt::pmt_t msg = delete_head_blocking(PMT_STRING_IN);

            struct timeval time,endTime ;
            uint uebertragungsDauer;
            gettimeofday(&time,NULL);

            if(!pmt::is_pair(msg)) {
                std::cerr << "Wrong Message Type received"<<std::endl;
                return 0;
            }
            pmt::pmt_t header = pmt::car(msg);
            if(pmt::dict_has_key(header,PMT_STRING_TIME_START_SEC)) {
                pmt::pmt_t sec = pmt::dict_ref(header,PMT_STRING_TIME_START_SEC,PMT_CONST_INT_0);
                time.tv_sec = pmt::to_long(sec);
                pmt::pmt_t usec = pmt::dict_ref(header,PMT_STRING_TIME_START_USEC,PMT_CONST_INT_0);
                time.tv_usec = pmt::to_long(usec);

                sec = pmt::dict_ref(header,PMT_STRING_TIME_STOP_SEC,PMT_CONST_INT_0);
                usec = pmt::dict_ref(header,PMT_STRING_TIME_STOP_USEC,PMT_CONST_INT_0);
                endTime.tv_sec = pmt::to_long(sec);
                endTime.tv_usec = pmt::to_long(usec);

                __suseconds_t seconds = endTime.tv_sec - time.tv_sec;


                __suseconds_t microSeconds = endTime.tv_usec;
                if (pmt::to_long(usec)< time.tv_usec)
                    microSeconds+=1e6;
                microSeconds-= time.tv_usec;
                uebertragungsDauer = microSeconds+seconds*1e6;
            }


            pmt::pmt_t vec = pmt::cdr(msg);
            size_t vec_size, byteVec_size;
            const uint8_t* data =toByteArray( pmt::u8vector_elements(vec,vec_size),vec_size,byteVec_size);
            float rssi = pmt::to_double(pmt::dict_ref(header,PMT_STRING_RSSI,PMT_CONST_INT_MINUS_1));
            p_msg_len = sizeof(pcaprec_hdr_s) + sizeof(float)  + sizeof(uint) + sizeof(uint8_t)*byteVec_size;
            p_msg_offset = 0;
            p_msg = (char*) malloc(p_msg_len);
            pcaprec_hdr_s* hdr = (pcaprec_hdr_s*) p_msg;
            hdr->ts_sec = time.tv_sec;
            hdr->ts_usec = time.tv_usec;
            hdr->incl_len= byteVec_size + sizeof(float) + sizeof(uint);
            hdr->orig_len= byteVec_size + sizeof(float) + sizeof(uint);

            memcpy(p_msg+sizeof(pcaprec_hdr_s),&rssi,sizeof(float));
            memcpy(p_msg+sizeof(pcaprec_hdr_s) + sizeof(float), &uebertragungsDauer,sizeof(uint));
            //memcpy(p_msg+sizeof(pcaprec_hdr_s) + sizeof(float) + sizeof(long), &endTime.tv_usec,sizeof(long));
            memcpy(p_msg+sizeof(pcaprec_hdr_s) + sizeof(float)+ sizeof(uint),data,byteVec_size);

            delete[] data;
        }

        if(out != NULL && p_msg != NULL ) {
            uint writeable_items = std::min(noutput_items,(int) (p_msg_len -p_msg_offset));
            memcpy(out,p_msg+p_msg_offset,writeable_items);
            p_msg_offset += writeable_items;

            if(writeable_items == p_msg_len){
                p_msg_len = 0;
                p_msg_offset = 0;
                free(p_msg);
                p_msg = NULL;
            }
            return writeable_items;
        }
        return 0;
    }

  } /* namespace bachelor */
} /* namespace gr */

