#!/usr/bin/env python2
##################################################
# GNU Radio Python Flow Graph
# Title: SkyNet Sniffer
# Author: Fabian Haerer
# Generated: Fri May 29 02:31:47 2015
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import bachelor
import math
import osmosdr
from time import strftime
import time

class skynet_sniffer(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "SkyNet Sniffer")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 1e6
        self.dec = dec = 10
        self.samp_rate_after_FIR_filter = samp_rate_after_FIR_filter = samp_rate/dec
        self.trans = trans = 5
        self.samp_per_sym = samp_per_sym = samp_rate_after_FIR_filter/50000
        self.fsk_deviation_hz = fsk_deviation_hz = 20000
        self.frequency = frequency = 868.0377e6
        self.freq_shift = freq_shift = 150e3
        self.filename_0 = filename_0 = "./"+ strftime("SkyNet_Capture_%Y-%m-%d_%H:%M:%S")+"_old.pcap"
        self.filename = filename = "./"+ strftime("SkyNet_Capture_%Y-%m-%d_%H:%M:%S")+"_new.pcap"
        self.cutoff = cutoff = 12.5

        ##################################################
        # Blocks
        ##################################################
        self.single_pole_iir_filter_xx_0 = filter.single_pole_iir_filter_ff(1.0, 1)
        self.rtlsdr_source_0_0 = osmosdr.source( args="numchan=" + str(1) + " " + "" )
        self.rtlsdr_source_0_0.set_sample_rate(samp_rate)
        self.rtlsdr_source_0_0.set_center_freq(frequency-freq_shift, 0)
        self.rtlsdr_source_0_0.set_freq_corr(30.5, 0)
        self.rtlsdr_source_0_0.set_dc_offset_mode(2, 0)
        self.rtlsdr_source_0_0.set_iq_balance_mode(2, 0)
        self.rtlsdr_source_0_0.set_gain_mode(False, 0)
        self.rtlsdr_source_0_0.set_gain(50, 0)
        self.rtlsdr_source_0_0.set_if_gain(20, 0)
        self.rtlsdr_source_0_0.set_bb_gain(30, 0)
        self.rtlsdr_source_0_0.set_antenna("", 0)
        self.rtlsdr_source_0_0.set_bandwidth(0, 0)
          
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccf(dec, (firdes.low_pass(1, samp_rate_after_FIR_filter, cutoff*1e3, trans*100, firdes.WIN_BLACKMAN, 6.76)), freq_shift, samp_rate)
        self.blocks_threshold_ff_1 = blocks.threshold_ff(0.00001, 0.00002, 0)
        self.blocks_nlog10_ff_0 = blocks.nlog10_ff(10, 1, 0)
        self.blocks_file_sink_1_1_0 = blocks.file_sink(gr.sizeof_char*1, filename_0, False)
        self.blocks_file_sink_1_1_0.set_unbuffered(True)
        self.blocks_file_sink_1_1 = blocks.file_sink(gr.sizeof_char*1, filename, False)
        self.blocks_file_sink_1_1.set_unbuffered(True)
        self.blocks_complex_to_mag_squared_1 = blocks.complex_to_mag_squared(1)
        self.bachelor_skynet_wireshark_sink_0_0 = bachelor.skynet_wireshark_sink()
        self.bachelor_skynet_wireshark_sink_0 = bachelor.skynet_wireshark_sink()
        self.bachelor_skynet_extended_analyze_0 = bachelor.skynet_extended_analyze()
        self.bachelor_skynet_analyze_0 = bachelor.skynet_analyze(int(samp_rate_after_FIR_filter),samp_per_sym, 3)
        self.analog_quadrature_demod_cf_1 = analog.quadrature_demod_cf(2)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.bachelor_skynet_analyze_0, 'out'), (self.bachelor_skynet_wireshark_sink_0_0, 'in'))    
        self.msg_connect((self.bachelor_skynet_extended_analyze_0, 'out'), (self.bachelor_skynet_wireshark_sink_0, 'in'))    
        self.connect((self.analog_quadrature_demod_cf_1, 0), (self.bachelor_skynet_analyze_0, 1))    
        self.connect((self.analog_quadrature_demod_cf_1, 0), (self.bachelor_skynet_extended_analyze_0, 0))    
        self.connect((self.bachelor_skynet_wireshark_sink_0, 0), (self.blocks_file_sink_1_1, 0))    
        self.connect((self.bachelor_skynet_wireshark_sink_0_0, 0), (self.blocks_file_sink_1_1_0, 0))    
        self.connect((self.blocks_complex_to_mag_squared_1, 0), (self.blocks_threshold_ff_1, 0))    
        self.connect((self.blocks_complex_to_mag_squared_1, 0), (self.single_pole_iir_filter_xx_0, 0))    
        self.connect((self.blocks_nlog10_ff_0, 0), (self.bachelor_skynet_analyze_0, 2))    
        self.connect((self.blocks_nlog10_ff_0, 0), (self.bachelor_skynet_extended_analyze_0, 2))    
        self.connect((self.blocks_threshold_ff_1, 0), (self.bachelor_skynet_analyze_0, 0))    
        self.connect((self.blocks_threshold_ff_1, 0), (self.bachelor_skynet_extended_analyze_0, 1))    
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_quadrature_demod_cf_1, 0))    
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.blocks_complex_to_mag_squared_1, 0))    
        self.connect((self.rtlsdr_source_0_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))    
        self.connect((self.single_pole_iir_filter_xx_0, 0), (self.blocks_nlog10_ff_0, 0))    


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_samp_rate_after_FIR_filter(self.samp_rate/self.dec)
        self.rtlsdr_source_0_0.set_sample_rate(self.samp_rate)

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
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate_after_FIR_filter, self.cutoff*1e3, self.trans*100, firdes.WIN_BLACKMAN, 6.76)))

    def get_trans(self):
        return self.trans

    def set_trans(self, trans):
        self.trans = trans
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate_after_FIR_filter, self.cutoff*1e3, self.trans*100, firdes.WIN_BLACKMAN, 6.76)))

    def get_samp_per_sym(self):
        return self.samp_per_sym

    def set_samp_per_sym(self, samp_per_sym):
        self.samp_per_sym = samp_per_sym

    def get_fsk_deviation_hz(self):
        return self.fsk_deviation_hz

    def set_fsk_deviation_hz(self, fsk_deviation_hz):
        self.fsk_deviation_hz = fsk_deviation_hz

    def get_frequency(self):
        return self.frequency

    def set_frequency(self, frequency):
        self.frequency = frequency
        self.rtlsdr_source_0_0.set_center_freq(self.frequency-self.freq_shift, 0)

    def get_freq_shift(self):
        return self.freq_shift

    def set_freq_shift(self, freq_shift):
        self.freq_shift = freq_shift
        self.rtlsdr_source_0_0.set_center_freq(self.frequency-self.freq_shift, 0)
        self.freq_xlating_fir_filter_xxx_0.set_center_freq(self.freq_shift)

    def get_filename_0(self):
        return self.filename_0

    def set_filename_0(self, filename_0):
        self.filename_0 = filename_0
        self.blocks_file_sink_1_1_0.open(self.filename_0)

    def get_filename(self):
        return self.filename

    def set_filename(self, filename):
        self.filename = filename
        self.blocks_file_sink_1_1.open(self.filename)

    def get_cutoff(self):
        return self.cutoff

    def set_cutoff(self, cutoff):
        self.cutoff = cutoff
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass(1, self.samp_rate_after_FIR_filter, self.cutoff*1e3, self.trans*100, firdes.WIN_BLACKMAN, 6.76)))


if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = skynet_sniffer()
    tb.start()
    tb.wait()
