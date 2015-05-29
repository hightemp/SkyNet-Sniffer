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

#ifndef INCLUDED_BACHELOR_SKYNET_EXTENDED_ANALYZE_IMPL_H
#define INCLUDED_BACHELOR_SKYNET_EXTENDED_ANALYZE_IMPL_H

#include <bachelor/skynet_extended_analyze.h>

namespace gr {
  namespace bachelor {

    class skynet_extended_analyze_impl : public skynet_extended_analyze
    {
     private:
      const static int MORE_BITS_NEEDED = -2;
      const static int NO_SYNC_FOUND = -1234567890;
      uint copy_to_buffer(const float *real, const float *rssi, uint start, uint length);
      void clear_buffer();
      int handle_package();
      int search_preamble();
      int search_sync_word(int preamble_start);
      int d_sampels_per_symbol;
      int d_threshold;
      std::vector<float>* d_rssi_buf;
      std::vector<float>* d_real_buf;
      bool d_packageFound;
      u_int64_t d_package_start_pos;
      u_int64_t d_last_package_end_pos;
      uint d_min_buffer_size;
      uint64_t d_data;
      uint64_t d_mag_data;
      uint64_t d_preamble_word;
      uint64_t d_mask;
      struct timeval d_packet_start_time;
      int d_preamble_pos;
      int d_detected;
      char* d_data_buffer;
      float* d_rssi_buffer;
      float* d_real_buffer;
      float d_average;

      unsigned char* to_byte_array(const char* bitArray, const size_t& inLength, size_t& outputLength);
     public:
      skynet_extended_analyze_impl();

      ~skynet_extended_analyze_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };


  } // namespace bachelor
} // namespace gr

#endif /* INCLUDED_BACHELOR_SKYNET_EXTENDED_ANALYZE_IMPL_H */

