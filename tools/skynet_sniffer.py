#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: SkyNet Sniffer
# Author: Fabian Haerer
# Generated: Thu Mar 12 23:15:31 2015
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
from time import strftime
import bachelor
import math
import osmosdr

class skynet_sniffer(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "SkyNet Sniffer")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 1e6
        self.dec = dec = 4
        self.samp_rate_after_FIR_filter = samp_rate_after_FIR_filter = samp_rate/dec
        self.fsk_deviation_hz = fsk_deviation_hz = 20000
        self.trans = trans = 16
        self.samp_per_sym = samp_per_sym = samp_rate_after_FIR_filter/50000
        self.frequency = frequency = 867.908e6
        self.filename = filename = "./"+ strftime("SkyNet_Capture_%Y-%m-%d_%H:%M:%S")+".pcap"
        self.cutoff = cutoff = 2*fsk_deviation_hz/1e3

        ##################################################
        # Blocks
        ##################################################
        self.single_pole_iir_filter_xx_0 = filter.single_pole_iir_filter_ff(1.0, 1)
        self.rtlsdr_source_0 = osmosdr.source( args="numchan=" + str(1) + " " + "" )
        self.rtlsdr_source_0.set_sample_rate(samp_rate)
        self.rtlsdr_source_0.set_center_freq(frequency-100e3, 0)
        self.rtlsdr_source_0.set_freq_corr(0, 0)
        self.rtlsdr_source_0.set_dc_offset_mode(0, 0)
        self.rtlsdr_source_0.set_iq_balance_mode(0, 0)
        self.rtlsdr_source_0.set_gain_mode(False, 0)
        self.rtlsdr_source_0.set_gain(50, 0)
        self.rtlsdr_source_0.set_if_gain(20, 0)
        self.rtlsdr_source_0.set_bb_gain(20, 0)
        self.rtlsdr_source_0.set_antenna("", 0)
        self.rtlsdr_source_0.set_bandwidth(0, 0)
          
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(dec, (firdes.low_pass(1, samp_rate, cutoff*1000, trans*1000, firdes.WIN_BLACKMAN, 6.76)), 100e3 , samp_rate)
        self.blocks_threshold_ff_1 = blocks.threshold_ff(0.001, 0.05, 0)
        self.blocks_nlog10_ff_0 = blocks.nlog10_ff(10, 1, 0)
        self.blocks_file_sink_1_1 = blocks.file_sink(gr.sizeof_char*1, filename, False)
        self.blocks_file_sink_1_1.set_unbuffered(True)
        self.blocks_complex_to_mag_squared_1 = blocks.complex_to_mag_squared(1)
        self.bachelor_skynet_wireshark_sink_1 = bachelor.skynet_wireshark_sink()
        self.bachelor_skynet_analyze_0 = bachelor.skynet_analyze(int(samp_rate_after_FIR_filter),samp_per_sym)
        self.analog_simple_squelch_cc_0 = analog.simple_squelch_cc(-40, 1)
        self.analog_quadrature_demod_cf_1 = analog.quadrature_demod_cf(1)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_simple_squelch_cc_0, 0), (self.blocks_complex_to_mag_squared_1, 0))
        self.connect((self.analog_simple_squelch_cc_0, 0), (self.analog_quadrature_demod_cf_1, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_simple_squelch_cc_0, 0))
        self.connect((self.bachelor_skynet_wireshark_sink_1, 0), (self.blocks_file_sink_1_1, 0))
        self.connect((self.single_pole_iir_filter_xx_0, 0), (self.blocks_nlog10_ff_0, 0))
        self.connect((self.blocks_nlog10_ff_0, 0), (self.bachelor_skynet_analyze_0, 2))
        self.connect((self.blocks_complex_to_mag_squared_1, 0), (self.single_pole_iir_filter_xx_0, 0))
        self.connect((self.analog_quadrature_demod_cf_1, 0), (self.bachelor_skynet_analyze_0, 1))
        self.connect((self.blocks_threshold_ff_1, 0), (self.bachelor_skynet_analyze_0, 0))
        self.connect((self.rtlsdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))
        self.connect((self.blocks_complex_to_mag_squared_1, 0), (self.blocks_threshold_ff_1, 0))

        ##################################################
        # Asynch Message Connections
        ##################################################
        self.msg_connect(self.bachelor_skynet_analyze_0, "out", self.bachelor_skynet_wireshark_sink_1, "in")


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_samp_rate_after_FIR_filter(self.samp_rate/self.dec)
        self.rtlsdr_source_0.set_sample_rate(self.samp_rate)
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate, self.cutoff*1000, self.trans*1000, firdes.WIN_BLACKMAN, 6.76)))

    def get_dec(self):
        return self.dec

    def set_dec(self, dec):
        self.dec = dec
        self.set_samp_rate_after_FIR_filter(self.samp_rate/self.dec)

    def get_samp_rate_after_FIR_filter(self):
        return self.samp_rate_after_FIR_filter

    def set_samp_rate_after_FIR_filter(self, samp_rate_after_FIR_filter):
        self.samp_rate_after_FIR_filter = samp_rate_after_FIR_filter
        self.set_samp_per_sym(self.samp_rate_after_FIR_filter/50000)

    def get_fsk_deviation_hz(self):
        return self.fsk_deviation_hz

    def set_fsk_deviation_hz(self, fsk_deviation_hz):
        self.fsk_deviation_hz = fsk_deviation_hz
        self.set_cutoff(2*self.fsk_deviation_hz/1e3)

    def get_trans(self):
        return self.trans

    def set_trans(self, trans):
        self.trans = trans
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate, self.cutoff*1000, self.trans*1000, firdes.WIN_BLACKMAN, 6.76)))

    def get_samp_per_sym(self):
        return self.samp_per_sym

    def set_samp_per_sym(self, samp_per_sym):
        self.samp_per_sym = samp_per_sym

    def get_frequency(self):
        return self.frequency

    def set_frequency(self, frequency):
        self.frequency = frequency
        self.rtlsdr_source_0.set_center_freq(self.frequency-100e3, 0)

    def get_filename(self):
        return self.filename

    def set_filename(self, filename):
        self.filename = filename
        self.blocks_file_sink_1_1.open(self.filename)

    def get_cutoff(self):
        return self.cutoff

    def set_cutoff(self, cutoff):
        self.cutoff = cutoff
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate, self.cutoff*1000, self.trans*1000, firdes.WIN_BLACKMAN, 6.76)))

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = skynet_sniffer()
    tb.start()
    raw_input('Press Enter to quit: ')
    tb.stop()
    tb.wait()
