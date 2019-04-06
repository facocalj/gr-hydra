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

#ifndef INCLUDED_HYDRA_TYPES_H
#define INCLUDED_HYDRA_TYPES_H

#include <vector>
#include <queue>
#include <memory>
#include <complex>
#include <thread>

#define PRINT_DEBUG(txt) std::cout << #txt ": " << txt << std::endl;

namespace hydra {
  class abstract_device;
  class Hypervisor;
  class VirtualRadio;

  typedef enum {
    USRP,
    IMAGE_GEN,
  } uhd_mode;

  typedef std::complex<float> gr_complex;
  typedef std::complex<float> iq_sample;
  typedef std::deque<iq_sample> iq_stream;
  typedef std::vector<iq_sample> window;
  typedef std::deque<window> window_stream;

  typedef std::vector<std::complex<float> > iq_window;
  typedef std::shared_ptr<abstract_device> uhd_hydra_sptr;


  typedef std::shared_ptr<gr_complex[]> samples_ptr;
  typedef std::vector<gr_complex> samples_vec;
  typedef std::queue<samples_vec> samples_vec_vec;

  typedef std::vector<int> iq_map_vec;

  typedef std::shared_ptr<Hypervisor> HypervisorPtr;
  typedef std::shared_ptr<VirtualRadio> VirtualRadioPtr;
} /* namespace hydra */

class base_block
{
  std::unique_ptr<std::thread> run_thread;
  bool stop_thread;

  void run();

  void stop()
  {
    stop_thread = true;
    run_thread->join();
  };
};

class sink_block : public base_block
{
  // Input buffer pointer
};

class source_block: public base_block
{
  // Output buffer instance
};


class general_block: public sink_block, source_block{};







#endif /* ifndef INCLUDED_HYDRA_TYPES_H */
