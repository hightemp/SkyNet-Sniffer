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

#ifndef INCLUDED_BACHELOR_SKYNET_ANALYZE_IMPL_H
#define INCLUDED_BACHELOR_SKYNET_ANALYZE_IMPL_H

#include <bachelor/skynet_analyze.h>

namespace gr {
  namespace bachelor {

    class skynet_analyze_impl : public skynet_analyze
    {
     private:
     void add_item_to_buffer(float);
     void add_item_to_buffer(float item, std::vector<float>* buffer);
     void handlePackage();
     void clear_buffer();
     std::vector<float>* rec_buf;
     std::vector<float>* rssi_buf;
     bool packageFound;
     int sample_rate;
     float symb_per_second;
     struct timeval packet_start_time;

     public:
      skynet_analyze_impl(int samp_rate, float symbols_per_second);
      ~skynet_analyze_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace bachelor
} // namespace gr

#endif /* INCLUDED_BACHELOR_SKYNET_ANALYZE_IMPL_H */

