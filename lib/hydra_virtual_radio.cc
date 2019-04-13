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

#include "hydra/hydra_virtual_radio.h"

namespace hydra {

VirtualRadio::VirtualRadio(size_t _idx, Hypervisor *hypervisor):
   g_idx(_idx),
   b_receiver(false),
   b_transmitter(false),
   g_rx_udp_port(0),
   g_tx_udp_port(0)
{
}

int
VirtualRadio::set_rx_chain(unsigned int u_rx_udp,
                           double d_rx_freq,
                           double d_rx_bw,
                           const std::string &server_addr,
                           const std::string &remote_addr)
{
  // If already receiving
  if (b_receiver) { return 1; }

  // Set the VR RX UDP port
  g_rx_udp_port = u_rx_udp;
  g_rx_cf = d_rx_freq;
  g_rx_bw = d_rx_bw;

  g_rx_fft_size = p_hypervisor->get_rx_fft() * (d_rx_bw / p_hypervisor->get_rx_bandwidth());

  // Create a new virtual RF front-end
  rx_virtual_rf = std::make_unique<virtual_rf_source>(
      g_tx_bw,
      g_tx_cf,
      g_tx_fft_size);

  // Create new resampler
  rx_resampler = std::make_unique<resampler<iq_window, iq_sample>>(
      rx_virtual_rf->buffer(),
      d_rx_bw,
      g_rx_fft_size);

  /* Create ZMQ transmitter */
  rx_socket = zmq_sink::make(
      rx_resampler->buffer(),
      server_addr,
      remote_addr,
      std::to_string(u_rx_udp));

  /* Toggle receiving flag */
  b_receiver = true;

  // Create reports object
  //rx_report = std::make_unique<xvl_report>(g_idx, rx_socket->buffer());

  // Successful return
  return 0;
}

int
VirtualRadio::set_tx_chain(unsigned int u_tx_udp,
                           double d_tx_cf,
                           double d_tx_bw,
                           const std::string &server_addr,
                           const std::string &remote_addr)
{
  // If already transmitting
  if (b_transmitter)
    // Return error
    return 1;

  // Set the VR TX UDP port
  g_tx_udp_port = u_tx_udp;
  g_tx_bw = d_tx_bw;
  g_tx_cf = d_tx_cf;

  g_tx_fft_size = p_hypervisor->get_tx_fft() * (d_tx_bw / p_hypervisor->get_tx_bandwidth());

  // Create ZMQ receiver
  tx_socket = zmq_source::make(
      server_addr,
      remote_addr,
      std::to_string(u_tx_udp),
      1000*g_tx_fft_size);

  // Create new resampler
  tx_resampler = std::make_unique<resampler<iq_sample, iq_window>>(
      tx_socket->buffer(),
      d_tx_bw,
      g_tx_fft_size);


  // Create a new virtual RF front-end
  tx_virtual_rf = std::make_unique<virtual_rf_sink>(
      tx_resampler->buffer(),
      g_tx_bw,
      g_tx_cf,
      g_tx_fft_size);

  // Toggle transmitting flag
  b_transmitter = true;

  // TODO Create TX reports object
  // tx_report = std::make_unique<xvl_report>(u_id, tx_socket->windows());

  // Successful return
  return 0;
}



} /* namespace hydra */
