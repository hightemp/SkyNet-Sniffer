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

#ifndef INCLUDED_BACHELOR_CONSTANTS_H
#define INCLUDED_BACHELOR_CONSTANTS_H

namespace gr {
namespace bachelor {
        const static pmt::pmt_t PMT_STRING_TIME_START_SEC = pmt::mp("time_start_sec");
        const static pmt::pmt_t PMT_STRING_TIME_START_USEC = pmt::mp("time_start_usec");
        const static pmt::pmt_t PMT_STRING_TIME_STOP_SEC =  pmt::mp("time_stop_sec");
        const static pmt::pmt_t PMT_STRING_TIME_STOP_USEC =  pmt::mp("time_stop_usec");
        const static pmt::pmt_t PMT_STRING_IN = pmt::mp("in");
        const static pmt::pmt_t PMT_STRING_OUT = pmt::mp("out");
        const static pmt::pmt_t PMT_STRING_RSSI = pmt::mp("rssi");
        const static pmt::pmt_t PMT_CONST_INT_0 = pmt::mp(0);
        const static pmt::pmt_t PMT_CONST_INT_MINUS_1 =  pmt::mp(-1);
    } // namespace bachelor
} // namespace gr

#endif /* INCLUDED_BACHELOR_CONSTANTS_H */
