#!/usr/bin/python2
#!/usr/bin/env python
#
# Copyright 2005,2006,2011,2013 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import svl

from gnuradio import blocks

# from current dir
from transmit_path import TransmitPath, ReadThread
from uhd_interface import uhd_transmitter


# For a nested dict, you need to recursively update __dict__
def dict2obj(d):
        if isinstance(d, list):
            d = [dict2obj(x) for x in d]
        if not isinstance(d, dict):
            return d

        class C(object):
            pass

        o = C()
        for k in d:
            o.__dict__[k] = dict2obj(d[k])
        return o


class my_top_block(gr.top_block):
    def __init__(self, options, options_vr1, options_vr2):
        gr.top_block.__init__(self)

        if(options.file_sink is False):
            self.sink = uhd_transmitter(options.args,
                                        options.bandwidth, options.tx_freq,
                                        options.lo_offset, options.tx_gain,
                                        options.spec, options.antenna,
                                        options.clock_source, options.verbose)
        else:
            self.sink = blocks.null_sink(gr.sizeof_gr_complex)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath1 = TransmitPath(options_vr1)
        self.txpath2 = TransmitPath(options_vr2)

        vr_configs = []
        vr_configs.append([options_vr1.freq, options_vr1.bandwidth])
        vr_configs.append([options_vr2.freq, options_vr2.bandwidth])

        if not options.two_virtual_radios:
            svl_sink = svl.svl_sink(1,
                                    options.fft_length,
                                    int(options.tx_freq),
                                    int(options.bandwidth),
                                    vr_configs)
            self.connect(self.txpath1, svl_sink, self.sink)
            self.svl = svl_sink
        else:
            svl_sink = svl.svl_sink(2,
                                    options.fft_length,
                                    int(options.tx_freq),
                                    int(options.bandwidth),
                                    vr_configs)
            self.connect(self.txpath1, (svl_sink, 0), self.sink)
            self.connect(self.txpath2, (svl_sink, 1))
            self.svl = svl_sink


# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
def main():

    def send_pkt(payload='', eof=False):
        return tb.txpath1.send_pkt(payload, eof)

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")

    svl_centerfrequency = 5.5e9
    svl_options = parser.add_option_group("HyDRA Options")
    svl_options.add_option("-2", "--two-virtual-radios",
            action="store_true", default=False, help="Run with TWO virtual radios [default=%default]")
    svl_options.add_option("", "--file-sink",
            action="store_true", default=False, help="Do not use USRP as sink. Use file instead [default=%default]")
    svl_options.add_option("", "--fft-length", type="intx", default=2048,
            help="HyDRA FFT M size [default=%default]")
    parser.add_option("", "--tx-freq", type="eng_float", default=svl_centerfrequency,
            help="Hydra transmit frequency [default=%default]", metavar="FREQ")
    parser.add_option("-W", "--bandwidth", type="eng_float", default=2e6,
            help="Hydra sample_rate [default=%default]")

    vr1_options = parser.add_option_group("VR 1 Options")
    vr1_options.add_option("", "--vr1-bandwidth", type="eng_float", default=1e6,
            help="set bandwidth for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-freq", type="eng_float", default=svl_centerfrequency-500e3,
            help="set central frequency for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-tx-amplitude", type="eng_float", default=0.1, metavar="AMPL",
            help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr1_options.add_option("", "--vr1-file", type="string", default=None,
            help="set the file to obtain data [default=%default]")
    vr1_options.add_option("", "--vr1-buffersize", type="intx", default=3072,
            help="set number of bytes to read from buffer size for VR1 [default=%default]")
    vr1_options.add_option("-m", "--vr1-modulation", type="string", default="bpsk",
            help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr1_options.add_option("", "--vr1-fft-length", type="intx", default=512,
            help="set the number of FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-occupied-tones", type="intx", default=200,
            help="set the number of occupied FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-cp-length", type="intx", default=4,
            help="set the number of bits in the cyclic prefix [default=%default]")

    vr2_options = parser.add_option_group("VR 2 Options")
    vr2_options.add_option("", "--vr2-bandwidth", type="eng_float", default=200e3,
                           help="set bandwidth for VR 2 [default=%default]")  
    vr2_options.add_option("", "--vr2-freq", type="eng_float", default=svl_centerfrequency+350e3,
                           help="set central frequency for VR 2 [default=%default]")  
    vr2_options.add_option("", "--vr2-tx-amplitude", type="eng_float", default=0.1, metavar="AMPL",
                           help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr2_options.add_option("", "--vr2-file", type="string", default=None,
                      help="set the file to obtain data [default=%default]")
    vr2_options.add_option("", "--vr2-buffersize", type="intx", default=16,
                           help="set number of bytes to read from buffer size for VR2 [default=%default]")
    vr2_options.add_option("-m", "--vr2-modulation", type="string", default="bpsk",
                           help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr2_options.add_option("", "--vr2-fft-length", type="intx", default=128,
                           help="set the number of FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-occupied-tones", type="intx", default=100,
                           help="set the number of occupied FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-cp-length", type="intx", default=16,
                           help="set the number of bits in the cyclic prefix [default=%default]")

    expert_grp = parser.add_option_group("Expert")
    expert_grp.add_option("-s", "--size", type="eng_float", default=400,
                          help="set packet size [default=%default]")
    expert_grp.add_option("-M", "--megabytes", type="eng_float", default=1.0,
                          help="set megabytes to transmit [default=%default]")
    expert_grp.add_option("", "--to-file", default=None,
                          help="Output file for modulated samples")
    uhd_transmitter.add_options(parser)

    (options, args) = parser.parse_args()

    # build the graph
    options_vr1 = dict2obj({'tx_amplitude': options.vr1_tx_amplitude,
                            'freq': options.vr1_freq,
                            'bandwidth': options.vr1_bandwidth,
                            'file': options.vr1_file,
                            'buffersize': options.vr1_buffersize,
                            'modulation': options.vr1_modulation,
                            'fft_length': options.vr1_fft_length,
                            'occupied_tones': options.vr1_occupied_tones,
                            'cp_length': options.vr1_cp_length,
                            'modulation': options.vr1_modulation,
                            'verbose': False,
                            'log': False})
    options_vr2 = dict2obj({'tx_amplitude': options.vr2_tx_amplitude,
                            'freq': options.vr2_freq,
                            'bandwidth': options.vr2_bandwidth,
                            'file': options.vr2_file,
                            'buffersize': options.vr2_buffersize,
                            'modulation': options.vr2_modulation,
                            'fft_length': options.vr2_fft_length,
                            'occupied_tones': options.vr2_occupied_tones,
                            'cp_length': options.vr2_cp_length,
                            'modulation': options.vr2_modulation,
                            'verbose': False,
                            'log': False})

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    tb = my_top_block(options, options_vr1, options_vr2)

    t1 = ReadThread(options_vr1.file, options_vr1.buffersize, tb.txpath1)
    t2 = ReadThread(options_vr2.file, options_vr2.buffersize, tb.txpath2)

    tb.start()                       # start flow graph
    t1.start()
    t2.start()

    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass