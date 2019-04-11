#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Client 1Rx
# Generated: Thu Apr 11 13:50:12 2019
##################################################

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from PyQt4 import Qt
from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import hydra
import sys
from gnuradio import qtgui


class ansible_hydra_gr_client_1rx(gr.top_block, Qt.QWidget):

    def __init__(self, ansibleIP='127.0.0.1', freqrx=2e9, freqtx=2e9, mul=0.01, samp_rate=200e3, vr1offset=-300e3, vr2offset=700e3):
        gr.top_block.__init__(self, "Ansible Hydra Gr Client 1Rx")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Ansible Hydra Gr Client 1Rx")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "ansible_hydra_gr_client_1rx")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Parameters
        ##################################################
        self.ansibleIP = ansibleIP
        self.freqrx = freqrx
        self.freqtx = freqtx
        self.mul = mul
        self.samp_rate = samp_rate
        self.vr1offset = vr1offset
        self.vr2offset = vr2offset

        ##################################################
        # Blocks
        ##################################################
        self.hydra_gr__source_0_0 = hydra.hydra_gr_client_source(1, ansibleIP, ansibleIP, 5000)
        self.hydra_gr__source_0_0.start_client(freqrx + vr1offset, samp_rate * 2, 10000)

        self.digital_ofdm_rx_0 = digital.ofdm_rx(
        	  fft_len=64, cp_len=16,
        	  frame_length_tag_key='frame_'+"len",
        	  packet_length_tag_key="len",
        	  bps_header=1,
        	  bps_payload=1,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.blocks_tuntap_pdu_1 = blocks.tuntap_pdu('tap0', 1500, False)
        (self.blocks_tuntap_pdu_1).set_max_output_buffer(100000)
        self.blocks_tagged_stream_to_pdu_0 = blocks.tagged_stream_to_pdu(blocks.byte_t, "len")
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, 'VR1 RX', ""); self.blocks_tag_debug_0.set_display(True)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.blocks_tagged_stream_to_pdu_0, 'pdus'), (self.blocks_tuntap_pdu_1, 'pdus'))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tagged_stream_to_pdu_0, 0))
        self.connect((self.hydra_gr__source_0_0, 0), (self.digital_ofdm_rx_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "ansible_hydra_gr_client_1rx")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_ansibleIP(self):
        return self.ansibleIP

    def set_ansibleIP(self, ansibleIP):
        self.ansibleIP = ansibleIP

    def get_freqrx(self):
        return self.freqrx

    def set_freqrx(self, freqrx):
        self.freqrx = freqrx

    def get_freqtx(self):
        return self.freqtx

    def set_freqtx(self, freqtx):
        self.freqtx = freqtx

    def get_mul(self):
        return self.mul

    def set_mul(self, mul):
        self.mul = mul

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

    def get_vr1offset(self):
        return self.vr1offset

    def set_vr1offset(self, vr1offset):
        self.vr1offset = vr1offset

    def get_vr2offset(self):
        return self.vr2offset

    def set_vr2offset(self, vr2offset):
        self.vr2offset = vr2offset


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    parser.add_option(
        "", "--ansibleIP", dest="ansibleIP", type="string", default='127.0.0.1',
        help="Set ansibleIP [default=%default]")
    return parser


def main(top_block_cls=ansible_hydra_gr_client_1rx, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    from distutils.version import StrictVersion
    if StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls(ansibleIP=options.ansibleIP)
    tb.start()
    tb.show()

    def quitting():
        tb.stop()
        tb.wait()
    qapp.connect(qapp, Qt.SIGNAL("aboutToQuit()"), quitting)
    qapp.exec_()


if __name__ == '__main__':
    main()
