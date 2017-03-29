/* -*- c++ -*- */
/* 
 * Copyright 2016 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_HYDRA_ASINK_IMPL_H
#define INCLUDED_HYDRA_ASINK_IMPL_H

#include <hydra/api.h>
#include <hydra/hydra_block.h>

namespace gr {
   namespace hydra {

class SVL_API hydra_async_sink: public hydra_block
{
   public:
      typedef boost::shared_ptr<hydra_async_sink> hydra_async_sink_ptr;

      /**
       * @param _n_inputs
       * @param _fft_m_len
       * @param vradios_config
       */
      static hydra_async_sink_ptr make(size_t _n_ports,
			size_t _fft_m_len,
			double central_frequency,
			double bandwidth,
         const std::vector< std::vector<double> > vradios_config);

      /** CTOR
       */
      hydra_async_sink(size_t _n_inputs,
            size_t _fft_m_len,
				double central_frequency,
				double bandwidth,
            const std::vector< std::vector<double> > vradio_conf);

      /** DTOR
       */
      ~hydra_async_sink();

		void handle_msg(pmt::pmt_t msg, size_t radio_id);

      // Where all the action really happens
		int work(int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items);

};

} // namespace hydra
} // namespace gr

#endif /* INCLUDED_SVL_SVL-SINK_IMPL_H */
