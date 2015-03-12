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

#ifndef INCLUDED_BACHELOR_SKYNET_WIRESHARK_SINK_IMPL_H
#define INCLUDED_BACHELOR_SKYNET_WIRESHARK_SINK_IMPL_H

#include <bachelor/skynet_wireshark_sink.h>

namespace gr {
  namespace bachelor {

    class skynet_wireshark_sink_impl : public skynet_wireshark_sink
    {
     private:
        char* p_msg;
        uint p_msg_len;
        uint p_msg_offset;


        uint8_t* toByteArray(const uint8_t* bitArray, const size_t& inLength, size_t& outputLength);

     public:



      skynet_wireshark_sink_impl();
      ~skynet_wireshark_sink_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace bachelor
} // namespace gr

struct pcap_hdr_s {
        uint32_t magic_number;   /* magic number */
        uint16_t version_major;  /* major version number */
        uint16_t version_minor;  /* minor version number */
        int32_t  thiszone;       /* GMT to local correction */
        uint32_t sigfigs;        /* accuracy of timestamps */
        uint32_t snaplen;        /* max length of captured packets, in octets */
        uint32_t network;        /* data link type */
};

struct pcaprec_hdr_s {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
};

#endif /* INCLUDED_BACHELOR_SKYNET_WIRESHARK_SINK_IMPL_H */

