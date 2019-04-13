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
}

Hypervisor::Hypervisor(size_t _fft_m_len,
      double central_frequency,
      double bandwidth):
        tx_fft_len(_fft_m_len),
        g_tx_cf(central_frequency),
        g_tx_bw(bandwidth),
        g_tx_subcarriers_map(_fft_m_len, -1)
{
   g_fft_complex  = sfft_complex(new fft_complex(rx_fft_len));
   g_ifft_complex = sfft_complex(new fft_complex(tx_fft_len, false));
};

int
Hypervisor::notify(virtual_rf &vr, Hypervisor::Notify set_maps)
{
  if (vr.get_tx_enabled() || (set_maps & Hypervisor::SET_TX_MAP))
  {
    iq_map_vec subcarriers_map = g_tx_subcarriers_map;
    std::replace(subcarriers_map.begin(),
                 subcarriers_map.end(),
                 vr.get_id(),
                 -1);

    // enter 'if' in case of success
    if (set_tx_mapping(vr, subcarriers_map ) > 0)
    {
      // LOG(INFO) << "success";
      g_tx_subcarriers_map = subcarriers_map;
    }
  }

  if (vr.get_rx_enabled() || (set_maps & Hypervisor::SET_RX_MAP))
  {
    iq_map_vec subcarriers_map = g_rx_subcarriers_map;
    std::replace(subcarriers_map.begin(),
                 subcarriers_map.end(),
                 vr.get_id(),
                 -1);

    // enter 'if' in case of success
    if (set_rx_mapping(vr, subcarriers_map ) > 0)
    {
      //LOG(INFO) << "success";
      g_rx_subcarriers_map = subcarriers_map;
    }
  }

  return 1;
}

void
Hypervisor::set_tx_resources(uhd_hydra_sptr tx_dev, double cf, double bw, size_t fft)
{
   g_tx_dev = tx_dev;
   g_tx_cf = cf;
   g_tx_bw = bw;
   tx_fft_len = fft;
   g_tx_subcarriers_map = iq_map_vec(fft, -1);
   g_ifft_complex = sfft_complex(new fft_complex(fft, false));

   // Thread stop flag
   thr_tx_stop = false;

   g_tx_thread = std::make_unique<std::thread>(&Hypervisor::tx_run, this);
}

void
Hypervisor::tx_run()
{
  size_t g_tx_sleep_time = llrint(get_tx_fft() * 1e6 / get_tx_bandwidth());
  iq_window g_tx_window(get_tx_fft());

  // Even loop
  while (not thr_tx_stop)
  {
    // Populate TX window
    build_tx_window(&g_tx_window);
    // Send TX window
    g_tx_dev->send(&g_tx_window);
    // Fill TX window with empty samples
    std::fill(g_tx_window.begin(), g_tx_window.end(), std::complex<float>(0,0));
  }

  // Print debug message
  // std::cout << "Stopped hypervisor's transmitter chain" << std::endl;
}

void
Hypervisor::set_tx_mapping()
{
   iq_map_vec subcarriers_map(tx_fft_len, -1);

   std::lock_guard<std::mutex> _l(vradios_mtx);
   for (vradio_vec::iterator it = g_vradios.begin();
        it != g_vradios.end();
        ++it)
     {
        std::cout << "<hypervisor> TX setting map for VR " << (*it)->get_id() << std::endl;
        set_tx_mapping(*((*it).get()), subcarriers_map);
     }
     g_tx_subcarriers_map = subcarriers_map;
}

int
Hypervisor::set_tx_mapping(virtual_rf_sink &vr, iq_map_vec &subcarriers_map)
{
   double vr_bw = vr.get_tx_bandwidth();
   double vr_cf = vr.get_tx_freq();
   double offset = (vr_cf - vr_bw/2.0) - (g_tx_cf - g_tx_bw/2.0) ;

   // First VR subcarrier
   int sc = offset / (g_tx_bw / tx_fft_len);
   size_t fft_n = vr_bw /(g_tx_bw /tx_fft_len);

   if (sc < 0 || sc > tx_fft_len)
   {
     return -1;
   }

   std::cout << "<hypervisor> Created vRF for #" << vr.get_id() << ": CF @" << vr_cf << ", BW @" << vr_bw << ", Offset @" << offset << ", First SC @ " << sc << ". Last SC @" << sc + fft_n << std::endl;

   // Allocate subcarriers sequentially from sc
   iq_map_vec the_map;
   for (; sc < tx_fft_len; sc++)
   {
      //LOG_IF(subcarriers_map[sc] != -1, INFO) << "Subcarrier @" <<  sc << " already allocated";
      if (subcarriers_map[sc] != -1)
         return -1;

      the_map.push_back(sc);
      subcarriers_map[sc] = vr.get_id();

      // break when we allocated enough subcarriers
      if (the_map.size() == fft_n)
         break;
   }

   vr.set_tx_fft(fft_n);
   vr.set_tx_mapping(the_map);

   return 1;
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
Hypervisor::set_rx_resources(uhd_hydra_sptr rx_dev, double cf, double bw, size_t fft_len)
{
  g_rx_dev = rx_dev;
  g_rx_cf = cf;
  g_rx_bw = bw;
  rx_fft_len = fft_len;
  g_rx_subcarriers_map = iq_map_vec(fft_len, -1);
  g_fft_complex = sfft_complex(new fft_complex(fft_len));

  // Thread stop flag
  thr_rx_stop = false;

  g_rx_thread = std::make_unique<std::thread>(&Hypervisor::rx_run, this);
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
  iq_map_vec subcarriers_map(rx_fft_len, -1);

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

int
Hypervisor::set_rx_mapping(vrtual_rf_source &vr, iq_map_vec &subcarriers_map)
{
   double vr_bw = vr.get_rx_bandwidth();
   double vr_cf = vr.get_rx_freq();
   double offset = (vr_cf - vr_bw/2.0) - (g_rx_cf - g_rx_bw/2.0) ;

   // First VR subcarrier
   int sc = offset / (g_rx_bw / rx_fft_len);
   size_t fft_n = vr_bw /(g_rx_bw /rx_fft_len);


   if (sc < 0 || sc > rx_fft_len)
   {
      return -1;
   }

   // TODO what this means?
   double c_bw = fft_n*g_rx_bw/rx_fft_len;
   double c_cf = g_rx_cf - g_rx_bw/2 + (g_rx_bw/rx_fft_len) * (sc + (fft_n/2));

   std::cout << boost::format("<hypervisor> RX Request VR BW: %1%, CF: %2% ") % vr_bw % vr_cf << std::endl;
   std::cout << boost::format("<hypervisor> RX Actual  VR BW: %1%, CF: %2% ") % c_bw % c_cf << std::endl;

   // Allocate subcarriers sequentially from sc
   iq_map_vec the_map;
   for (; sc < rx_fft_len; sc++)
   {
      //LOG_IF(subcarriers_map[sc] != -1, INFO) << "Subcarrier @" <<  sc << " already allocated";
      if (subcarriers_map[sc] != -1)
         return -1;

      the_map.push_back(sc);
      subcarriers_map[sc] = vr.get_id();

      // break when we allocated enough subcarriers
      if (the_map.size() == fft_n)
         break;
   }

   vr.set_rx_fft(fft_n);
   vr.set_rx_mapping(the_map);

   return 1;
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

} /* namespace hydra */
