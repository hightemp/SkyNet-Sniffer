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


#ifndef INCLUDED_BACHELOR_SKYNET_ANALYZE_H
#define INCLUDED_BACHELOR_SKYNET_ANALYZE_H

#include <bachelor/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace bachelor {

    /*!
     * \brief <+description of block+>
     * \ingroup bachelor
     *
     */
    class BACHELOR_API skynet_analyze : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<skynet_analyze> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of bachelor::skynet_analyze.
       *
       * \param samp_rate Sample Rate of the incomming stream
       * \param sampels_per_symbol sps of the signal
       * \param threshold number of bits that can be wrong in the syncword, to be detected right
       */
      static sptr make(int samp_rate, float sampels_per_symbol, int threshold);
    };

  } // namespace bachelor
} // namespace gr

#endif /* INCLUDED_BACHELOR_SKYNET_ANALYZE_H */

