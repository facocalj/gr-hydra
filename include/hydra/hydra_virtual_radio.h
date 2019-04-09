/* -*- c++ -*- */
/*
 * Copyright 2016 Trinity Connect Centre.
 *
 * HyDRA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * HyDRA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_HYDRA_VIRTUAL_RADIO_H
#define INCLUDED_HYDRA_VIRTUAL_RADIO_H

#include <hydra/types.h>
#include <hydra/hydra_buffer.hpp>
#include <hydra/hydra_stats.h>

#include <hydra/hydra_socket.h>
#include <hydra/hydra_resampler.hpp>
#include <hydra/hydra_hypervisor.h>
#include <hydra/hydra_virtual_rf.h>


#include <vector>
#include <mutex>
// #include <boost/format.hpp>

namespace hydra {

class VirtualRadio
{
  public:
    /** CTOR
     */
    VirtualRadio(size_t _idx, Hypervisor *hypervisor);

    int set_rx_chain(unsigned int u_rx_udp,
                    double d_rx_centre_freq,
                    double d_rx_bw,
                    const std::string &server_addr,
                    const std::string &remote_addr);

    int set_tx_chain(unsigned int u_tx_udp,
                    double d_tx_centre_freq,
                    double d_tx_bw,
                    const std::string &server_addr,
                    const std::string &remote_addr);

    /** Return VRadio unique ID
     * @return VRadio ID
     */
    int const get_id() {return g_idx;}

    bool const get_tx_enabled(){ return g_tx_udp_port; };
    size_t const get_tx_udp_port(){ return g_tx_udp_port; }
    size_t const get_tx_fft() {return g_rx_fft_size;}
    double const get_tx_freq() { return g_tx_cf; }
    double const get_tx_bandwidth() {return g_tx_bw;}


    bool const get_rx_enabled(){ return g_rx_udp_port; };
    size_t const get_rx_udp_port(){ return g_rx_udp_port; }
    size_t const get_rx_fft() {return g_rx_fft_size;}
    double const get_rx_freq() { return g_rx_cf; }
    double const get_rx_bandwidth() {return g_rx_bw;}
    int set_rx_freq(double cf);
    void set_rx_bandwidth(double bw);
    void set_rx_mapping(const iq_map_vec &iq_map);
    size_t const set_rx_fft(size_t n) {return g_rx_fft_size = n;}
    void demap_iq_samples(const iq_sample *samples_buf, size_t len); // called by the hypervisor

    /**
     */
    bool const ready_to_demap_iq_samples();

  private:
    size_t g_rx_fft_size; // Subcarriers used by this VRadio
    size_t g_rx_udp_port;

    bool b_receiver;
    double g_rx_cf;      // Central frequency
    double g_rx_bw;      // Bandwidth

    ReportPtr rx_report;
    std::unique_ptr<virtual_rf_source> rx_virtual_rf;
    std::unique_ptr<resampler<iq_window, iq_sample>> rx_resampler;
    zmq_sink_ptr rx_socket;

    size_t g_tx_fft_size; // Subcarriers used by this VRadio
    size_t g_tx_udp_port;

    bool b_transmitter;
    double g_tx_cf;      // Central frequency
    double g_tx_bw;      // Bandwidth

    ReportPtr tx_report;
    zmq_source_ptr tx_socket;
    std::unique_ptr<resampler<iq_sample, iq_window>> tx_resampler;
    std::unique_ptr<virtual_rf_sink> tx_virtual_rf;

    int g_idx;        // Radio unique ID

    // pointer to this VR hypervisor
    Hypervisor *p_hypervisor;
};


} /* namespace hydra */


#endif /* ifndef INCLUDED_HYDRA_VIRTUAL_RADIO_H */
