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

#include "hydra/hydra_hypervisor.h"

namespace hydra {


Hypervisor::Hypervisor()
{
};

void
Hypervisor::set_tx_resources(uhd_hydra_sptr tx_dev, double cf, double bw, size_t fft)
{
   g_tx_dev = tx_dev;
   g_tx_cf = cf;
   g_tx_bw = bw;
   g_tx_fft_size = fft;
   g_tx_subcarriers_map = iq_map_vec(fft, -1);
   g_ifft_complex = sfft_complex(new fft_complex(fft, false));

   // Thread stop flag
   thr_tx_stop = false;

   g_tx_thread = std::make_unique<std::thread>(&Hypervisor::tx_run, this);
}

void
Hypervisor::set_rx_resources(uhd_hydra_sptr rx_dev, double cf, double bw, size_t fft_len)
{
  g_rx_dev = rx_dev;
  g_rx_cf = cf;
  g_rx_bw = bw;
  g_rx_fft_size = fft_len;
  g_rx_subcarriers_map = iq_map_vec(fft_len, -1);
  g_fft_complex = sfft_complex(new fft_complex(fft_len));

  // Thread stop flag
  thr_rx_stop = false;

  g_rx_thread = std::make_unique<std::thread>(&Hypervisor::rx_run, this);
}

std::tuple<double,int>
Hypervisor::calculate_tx_offsets(const double &d_vr_tx_cf, const double &d_vr_tx_bw)
{
  // Frequency offset between VR and RR
  double freq_offset = (d_vr_tx_cf - d_vr_tx_bw/2.0) - (g_tx_cf - g_tx_bw/2.0);
  // FFT offset
  int carrier_offset = freq_offset / (g_tx_bw / g_tx_fft_size);

  // Return offsets
  return {freq_offset, carrier_offset}
}

std::tuple<size_t, double, double>
Hypervisor::calculate_tx_params(size_t u_vr_id, double d_vr_tx_cf, double d_vr_tx_bw)
{
  // Calculate the VR TX FFT size as a function of the allocated resources
  unsigned int u_vr_tx_fft_size = g_tx_fft_size * (d_vr_tx_bw / g_tx_bw);
  // Calculate the offsets
  auto [freq_offset, carrier_offset] = caltulate_tx_offsets(d_vr_tx_cf, d_vr_tx_bw)

  // TODO check with Maicon, shouldn't it be CO + FFT size?
  if ((carrier_offset < 0) or(carrier_offset > g_tx_fft_size)){return {0, 0, 0};}

  // Claculate the actual bandwidth and centre frequency
  double d_actual_vr_tx_bw = u_vr_tx_fft_size * g_tx_bw/g_tx_fft_size;
  // TODO the following equation can be simplified
  double d_actual_vr_tx_cf = g_rx_cf - (g_rx_bw/2.0) + (g_rx_bw/g_rx_fft_size) * (carrier_offset + (u_vr_tx_fft_size/2));

  // Output debug information
  std::cout << boost::format("<hypervisor> VR #%1%, Offset: %2%, First SC: %3%, Last SC: %4% ") % u_vr_id % freq_offset % carrier_offset % (carrier_offset + u_vr_tx_fft_size) << std::endl;
  std::cout << boost::format("<hypervisor> TX Request VR BW: %1%, CF: %2% ") % d_vr_tx_bw % d_vr_tx_cf << std::endl;
  std::cout << boost::format("<hypervisor> TX Actual  VR BW: %1%, CF: %2% ") % d_actual_vr_tx_bw % d_actual_vr_tx_cf << std::endl;

  // Return tuple with the new parameters
  return {u_vr_tx_fft_size, d_actual_vr_tx_cf, d_actual_vr_tx_bw};
}

Hypervisor::stuff_tx()
{
    // Copy the subcarriers map
    iq_map_vec subcarriers_map = g_tx_subcarriers_map;

    // Create a new FFT map
    iq_map_vec fft_map(u_vr_tx_fft_size);
    // Allocate subcarriers sequentially from sc
    for (int i = carrier_offset; i < carrier_offset + u_vr_tx_fft_size; i++)
    {
      //LOG_IF(subcarriers_map[i] != -1, INFO) << "Subcarrier @" <<  i << " already allocated";
      if (subcarriers_map[i] != -1){return {0, 0, 0};}

      // Write the FFT map and the subcarrier map
      fft_map[i - carrier_offset] = i;
      subcarriers_map[carrier_offset] = u_vr_id;
    }
    // Set the FFT size and FFT map
    vsink->set_fft_size(u_vr_tx_fft_size);
    vsink->set_fft_map(fft_map);

}

Hypervisor::stuff_rx()
{
   // Copy the subcarriers map
  iq_map_vec subcarriers_map = g_tx_subcarriers_map;

  // Create a new FFT map
  iq_map_vec fft_map(u_vr_rx_fft_size);
  // Allocate subcarriers sequentially from sc
  for (int i = carrier_offset; i < carrier_offset + u_vr_rx_fft_size; i++)
  {
    //LOG_IF(subcarriers_map[i] != -1, INFO) << "Subcarrier @" <<  i << " already allocated";
    if (subcarriers_map[i] != -1){return {0, 0, 0};}

    // Write the FFT map and the subcarrier map
    fft_map[i - carrier_offset] = i;
    subcarriers_map[carrier_offset] = u_vr_id;
  }
  // Set the FFT size and FFT map
  vsource->set_fft_size(u_vr_rx_fft_size);
  vsource->set_fft_map(fft_map);
}


std::tuple<double,int>
Hypervisor::calculate_rx_offsets(const double &d_vr_rx_cf, const double &d_vr_rx_bw)
{
  // Frequency offset between VR and RR
  double freq_offset = (d_vr_rx_cf - d_vr_rx_bw/2.0) - (g_rx_cf - g_rx_bw/2.0);
  // FFT offset
  int carrier_offset = freq_offset / (g_rx_bw / g_rx_fft_size);

  // Return offsets
  return {freq_offset, carrier_offset}
}

std::tuple<size_t, double, double>
Hypervisor::calculate_rx_params(size_t u_vr_id, double d_vr_rx_cf, double d_vr_rx_bw, VirtualRFSourcePtr vsource)
{
  // Calculate the VR RX FFT size as a function of the allocated resources
  unsigned int u_vr_rx_fft_size = g_rx_fft_size * (d_vr_rx_bw / g_rx_bw);
  // Frequency offset between VR and RR
  double freq_offset = (d_vr_rx_cf - d_vr_rx_bw/2.0) - (g_rx_cf - g_rx_bw/2.0);
  // FFT offset
  int carrier_offset = freq_offset / (g_rx_bw / g_rx_fft_size);

  // TODO check with Maicon, shouldn't it be CO + FFT size?
  if ((carrier_offset < 0) or(carrier_offset > g_rx_fft_size)){return {0, 0, 0};}

  // Claculate the actual bandwidth and centre frequency
  double d_actual_vr_rx_bw = u_vr_rx_fft_size * g_rx_bw/g_rx_fft_size;
  // TODO the following equation can be simplified
  double d_actual_vr_rx_cf = g_rx_cf - (g_rx_bw/2.0) + (g_rx_bw/g_rx_fft_size) * (carrier_offset + (u_vr_rx_fft_size/2));

  // Output debug information
  std::cout << boost::format("<hypervisor> VR #%1%, Offset: %2%, First SC: %3%, Last SC: %4% ") % u_vr_id % freq_offset % carrier_offset % (carrier_offset + u_vr_rx_fft_size) << std::endl;
  std::cout << boost::format("<hypervisor> RX Request VR BW: %1%, CF: %2% ") % d_vr_rx_bw % d_vr_rx_cf << std::endl;
  std::cout << boost::format("<hypervisor> RX Actual  VR BW: %1%, CF: %2% ") % d_actual_vr_rx_bw % d_actual_vr_rx_cf << std::endl;

  // Return tuple with the new parameters
  return {u_vr_rx_fft_size, d_actual_vr_rx_cf, d_actual_vr_rx_bw};
};

int
Hypervisor::attach_tx_vrf(int u_vr_id, VirtualRFSinkPtr vsink)
{
  // Copy the global TX subcarrier map
  iq_map_vec subcarriers_map = g_tx_subcarriers_map;

  // Replace instances of the given VR with -1
  std::replace(subcarriers_map.begin(), subcarriers_map.end(), u_vr_id, -1);

  // enter 'if' in case of success
  if (set_tx_mapping(vsink, subcarriers_map ) > 0)
  {
    // LOG(INFO) << "success";
    g_tx_subcarriers_map = subcarriers_map;
  }

  return 1;
};


int
Hypervisor::attach_rx_vrf(int u_vr_id, VirtualRFSourcePtr vsource)
{
  // Copy the global RX subcarrier map
  iq_map_vec subcarriers_map = g_rx_subcarriers_map;
  // Replace instances of the given VR with -1
  std::replace(subcarriers_map.begin(), subcarriers_map.end(), u_vr_id, -1);

  // enter 'if' in case of success
  if (set_rx_mapping(vsource, subcarriers_map ) > 0)
  {
    //LOG(INFO) << "success";
    g_rx_subcarriers_map = subcarriers_map;
  }

  return 1;
}

void
Hypervisor::tx_run()
{
  size_t g_tx_sleep_time = llrint(get_tx_fft() * 1e6 / get_tx_bandwidth());
  iq_window g_tx_window(get_tx_fft());

  // Create an empty IQ sample
  iq_sample empty_iq(0,0);

  // Even loop
  while (not thr_tx_stop)
  {
    // Populate TX window
    build_tx_window(&g_tx_window);
    // Send TX window
    g_tx_dev->send(&g_tx_window);
    // Fill TX window with empty samples
    std::fill(g_tx_window.begin(), g_tx_window.end(), empty_iq);
  }

  // Print debug message
  // std::cout << "Stopped hypervisor's transmitter chain" << std::endl;
}

void
Hypervisor::set_tx_mapping()
{
   iq_map_vec subcarriers_map(g_tx_fft_size, -1);

   std::lock_guard<std::mutex> _l(vradios_mtx);
   for (vradio_vec::iterator it = g_vradios.begin();
        it != g_vradios.end();
        ++it)
     {
        std::cout << "<hypervisor> TX setting map for VR " << (*it)->get_id() << std::endl;
        set_tx_mapping(*((*it).u_vr_id), subcarriers_map);
     }
     g_tx_subcarriers_map = subcarriers_map;
}

void
Hypervisor::build_tx_window(iq_window *tx_window)
{
  // If the list if empty, return false
  if (g_virtual_radios.empty()){return nullptr;}

  // Otherwise, lock this context
  std::lock_guard<std::mutex> hyper_tx_lock(tx_mutex);

  // Iterate over the list of TX front ends
  for (auto it = g_tx_rfs.begin(); it != g_tx_rfs.end(); it++)
  {
    (*it)->map_tx_samples(g_ifft_complex->get_inbuf());
  }

  // Map them to frequency domain
  g_ifft_complex->execute();

  // Map result to Tx window
  std::copy(
      g_ifft_complex->get_outbuf(),
      g_ifft_complex->get_outbuf() + len,
      g_tx_window->begin());
}

void
Hypervisor::rx_run()
{
  size_t g_rx_sleep_time = llrint(get_rx_fft() * 1e9 / get_rx_bandwidth());
  iq_window g_tx_window(get_tx_fft());

  while (not thr_rx_stop)
  {
    // Send TX window
    g_tx_dev->receive(&g_rx_window);
    // Populate TX window
    build_rx_window(&g_rx_window);
  }

  // Print debug message
  // std::cout << "<hypervisor> Stopped hypervisor's receiver chain" << std::endl;
}

void
Hypervisor::set_rx_mapping()
{
  iq_map_vec subcarriers_map(g_rx_fft_size, -1);

  // for each virtual radio, to its mapping to subcarriers
  // ::TRICKY:: we dont stop if a virtual radio cannot be allocated
  std::lock_guard<std::mutex> _l(vradios_mtx);
  for (vradio_vec::iterator it = g_vradios.begin();
       it != g_vradios.end();
       ++it)
    {
      std::cout << "<hypervisor> RX setting map for VR " << (*it)->get_id() << std::endl;
      set_rx_mapping(*((*it).get()), subcarriers_map);
    }
  g_rx_subcarriers_map = subcarriers_map;
}

void
Hypervisor::build_rx_window(iq_window *tx_window)
{
  // If the list if empty, return false
  if (g_virtual_radios.empty()){return nullptr;}

  // Otherwise, lock this context
  std::lock_guard<std::mutex> hyper_rx_lock(rx_mutex);

  g_fft_complex->set_data(&buf[0], len);
  g_fft_complex->execute();


  // Iterate over the list of TX front ends
  for (auto it = g_rx_rfs.begin(); it != g_rx_rfs.end(); it++)
  {
    (*it)->demaprtx_samples(g_ifft_complex->get_inbuf());
  }

          (*it)->demap_iq_samples(g_fft_complex->get_outbuf(), get_rx_fft());
}


void
virtual_rf_sink::set_tx_mapping(const iq_map_vec &iq_map)
{

  // TODO move to hypervisor
  g_tx_map = iq_map;
}


void
virtual_fr_source::set_rx_mapping(const iq_map_vec &iq_map)
{
  g_rx_map = iq_map;
}

} /* namespace hydra */
