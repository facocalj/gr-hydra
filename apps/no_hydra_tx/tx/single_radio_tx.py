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


# GNURadio blocks
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio import blocks
import hydra

# parse options
from optparse import OptionParser

# Server for remote commands
import SimpleXMLRPCServer
import threading

# from current dir
from transmit_path import TransmitPath, ReadThread, XMLRPCThread
from uhd_interface import uhd_transmitter

#hydra_center_frequency = 5.5e9 # XCVR2450
hydra_center_frequency = 2.45e9 # SBX
vr1_initial_shift = -500e3
vr2_initial_shift =  400e3

import numpy as np


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
            print("Using USRP")
            self.sink = uhd_transmitter(options.args,
                                        options_vr1.bandwidth if options.lte_radio else options_vr2.bandwidth,
                                        options_vr1.freq if options.lte_radio else options_vr2.freq,
                                        options.lo_offset, options.tx_gain,
                                        options.spec, options.antenna,
                                        options.clock_source, options.verbose)
        else:
            self.sink = blocks.null_sink(gr.sizeof_gr_complex)

        if not (options.lte_radio or options.nbiot_radio):
            print 'ERROR: Need to specify --lte-radio or --nbiot-radio'
            sys.exit(0)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath1 = TransmitPath(options_vr1 if options.lte_radio else options_vr2)

        self.connect(self.txpath1, self.sink)

	print 'Start XMLRPC Server ...'
        self.xmlrpc_server = SimpleXMLRPCServer.SimpleXMLRPCServer(("localhost", 12345), allow_none=True)
        self.xmlrpc_server.register_instance(self)
        threading.Thread(target=self.xmlrpc_server.serve_forever).start()

t1 = None

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
def main():
    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")

    global_options = parser.add_option_group("Host Options")
    global_options.add_option("", "--host-ip", type="string", default="localhost",
            help="This host command interface IP address [default=%default]")

    global_options = parser.add_option_group("Global Options")
    global_options.add_option("", "--lte-radio", action="store_true", default=False,
                              help="Run with VR1 (LTE) [default=%default]")
    global_options.add_option("", "--nbiot-radio", action="store_true", default=False,
                              help="Run with VR2 (NB-IoT) [default=%default]")
    global_options.add_option("", "--file-sink",
            action="store_true", default=False, help="Do not use USRP as sink. Use file instead [default=%default]")


    vr1_options = parser.add_option_group("VR 1 Options")
    vr1_options.add_option("", "--vr1-bandwidth", type="eng_float", default=1e6,
            help="set bandwidth for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-freq", type="eng_float", default=hydra_center_frequency+vr1_initial_shift,
            help="set central frequency for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-tx-amplitude", type="eng_float", default=0.1, metavar="AMPL",
            help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr1_options.add_option("", "--vr1-file", type="string", default='/home/ctvr/.wishful/radio/vr1fifo',
            help="set the file to obtain data [default=%default]")
    vr1_options.add_option("", "--vr1-buffersize", type="intx", default=3072,
            help="set number of bytes to read from buffer size for VR1 [default=%default]")
    vr1_options.add_option("-m", "--vr1-modulation", type="string", default="qpsk",
            help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr1_options.add_option("", "--vr1-fft-length", type="intx", default=512,
            help="set the number of FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-occupied-tones", type="intx", default=512,
            help="set the number of occupied FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-cp-length", type="intx", default=128,
            help="set the number of bits in the cyclic prefix [default=%default]")

    vr2_options = parser.add_option_group("VR 2 Options")
    vr2_options.add_option("", "--vr2-bandwidth", type="eng_float", default=200e3,
                           help="set bandwidth for VR 2 [default=%default]")
    vr2_options.add_option("", "--vr2-freq", type="eng_float", default=hydra_center_frequency + vr2_initial_shift,
                           help="set central frequency for VR 2 [default=%default]")
    vr2_options.add_option("", "--vr2-tx-amplitude", type="eng_float", default=0.125, metavar="AMPL",
                           help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr2_options.add_option("", "--vr2-file", type="string", default='/home/ctvr/.wishful/radio/vr2fifo',
                      help="set the file to obtain data [default=%default]")
    vr2_options.add_option("", "--vr2-buffersize", type="intx", default=64,
                           help="set number of bytes to read from buffer size for VR2 [default=%default]")
    vr2_options.add_option("-m", "--vr2-modulation", type="string", default="bpsk",
                           help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr2_options.add_option("", "--vr2-fft-length", type="intx", default=64,
                           help="set the number of FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-occupied-tones", type="intx", default=64,
                           help="set the number of occupied FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-cp-length", type="intx", default=2,
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
                            'sleep_time': 0,
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
                            'sleep_time': 0,
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

    tb = my_top_block(options, options_vr1, options_vr2)
    tb.start()                       # start flow graph


    if options.lte_radio:
        print 'Starting VR1 data thread...'
        t1 = ReadThread(options_vr1.file, options_vr1.buffersize, tb.txpath1, True, options_vr1.sleep_time)
        t1.start()
    else:
        print 'Starting VR2 data thread...'
        t2 = XMLRPCThread(options.host_ip, options_vr2.buffersize, tb.txpath1, options_vr2.sleep_time)
        t2.start()

    print 'Starting'
    return tb

if __name__ == '__main__':
    tb = None
    try:
       tb = main()
       tb.wait()
    except KeyboardInterrupt:
       print "Closing ..."
       tb.xmlrpc_server.stop()
       tb.stop()
