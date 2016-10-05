#include <svl/svl_virtual_radio.h>

#include "easylogging++.h"

namespace gr {
   namespace svl {

VirtualRadio::VirtualRadio(size_t _idx, size_t _fft_n_len):
        fft_n_len(_fft_n_len), g_idx(_idx)
{
   g_fft_complex = sfft_complex(new gr::svl::fft_complex(fft_n_len)) ;
   g_ifft_complex = sfft_complex(new gr::svl::fft_complex(fft_n_len, false));
}

/**
 * The number of IQ samples required to produced noutput_items output
 * One output is a buffer with fft_n_items
 *
 * @param noutput_items Total of noutput_items required
 * @return Total of IQ samples require to produce nouput_items
 */
int
VirtualRadio::forecast(int noutput_items)
{
   if (0 == g_tx_samples.size())
      return noutput_items * fft_n_len;

   int required = noutput_items - g_tx_samples.size();
   if (required <= 0) return 0;

   if (g_tx_samples.back().size() < fft_n_len)
   {
           required -= 1;
           return (required * fft_n_len) + (fft_n_len - g_tx_samples.back().size());
   }
   else return (required * fft_n_len);
}

/** Added the buff samples to the VR tx queue.
 * \param samples The samples  that must be added to the VR tx queue.
 * \param len samples lenght.
 */
void
VirtualRadio::add_iq_sample(const gr_complex *samples, size_t len)
{
   // the total of if itens we have transfered so far
   size_t consumed = 0;
   
   if (0 == g_tx_samples.size())
      g_tx_samples.push(samples_vec());
   
   // While we have samples to transfer
   while (consumed < len)
   {
      // If we filled the last samples_vec, create a new one
      if (g_tx_samples.back().size() == fft_n_len)
         g_tx_samples.push(samples_vec());
      
      size_t rest = std::min(len - consumed,
                      fft_n_len - g_tx_samples.back().size());
      
      // TRICKY: use std::copy instead of this loop.
      // Was seg faulting
      for (int i = 0; i < rest; ++i )
         g_tx_samples.back().push_back(samples[consumed+i]);
         consumed += rest;
   }

   LOG_IF(consumed > len, INFO) << "ERROR";
}

/**
 * @param iq_map
 */
void
VirtualRadio::set_iq_mapping(const iq_map_vec &iq_map)
{
	LOG_IF(iq_map.size() != fft_n_len, ERROR)  << "iq_map.size() != fft_n_len";
   g_iq_map = iq_map;
}

/**
 * @param samples_buf FREQ domain samples
 */
void
VirtualRadio::demap_iq_samples(const samples_vec &samples_buf)
{
	samples_vec rx_samples_freq(fft_n_len);
	
	// Copy the samples used by this radio
	size_t idx(0);
	for (iq_map_vec::iterator it = g_iq_map.begin();
			it != g_iq_map.end();
			++it, ++idx)
	{
		rx_samples_freq[idx] = samples_buf[*it];  
	}

	// Transfer samples to fft_complex buff and perform fft
	std::copy(rx_samples_freq.begin(), rx_samples_freq.end(),
			g_ifft_complex->get_inbuf());
	g_ifft_complex->execute();

	g_rx_samples.push(samples_vec(g_ifft_complex->get_outbuf(),
				  g_ifft_complex->get_outbuf() + fft_n_len)
	);
}

/**
 */
void
VirtualRadio::get_rx_samples(gr_complex *samples_buff, size_t len)
{
	LOG_IF(g_rx_samples.front().size() != fft_n_len, ERROR) << "wrong number of samples";

	std::copy(g_rx_samples.front().begin(), g_rx_samples.front().end(),
			samples_buff);

	LOG(INFO) << "get_rx_samples";
	for (int i =0; i < g_rx_samples.front().size(); i++)
		std::cout << g_rx_samples.front()[i] << ", ";
	std::cout << std::endl;

	g_rx_samples.pop();
}

/**
*/
bool const
VirtualRadio::ready_to_map_iq_samples()
{
   if (g_tx_samples.size() == 0 || (g_tx_samples.front().size() < fft_n_len))
   {
      return false;
   }

   return true;
}

/**
 * @param samples_buf
 */
bool
VirtualRadio::map_iq_samples(samples_vec &samples_buf)
{
   if (!ready_to_map_iq_samples()) return false;

   // Copy samples in TIME domain to FFT buffer, execute FFT
	samples_vec tx_time_samples = g_tx_samples.front();
		g_tx_samples.pop();

   std::copy(tx_time_samples.begin(), tx_time_samples.end(),
			g_fft_complex->get_inbuf());

   g_fft_complex->execute();

   samples_vec tx_samples_freq(g_fft_complex->get_outbuf(),
			g_fft_complex->get_outbuf() + fft_n_len);

   // map samples in FREQ domain to samples_buff
   size_t idx(0);
   for (iq_map_vec::iterator it = g_iq_map.begin();
			it != g_iq_map.end();
         ++it, ++idx)
   {
		samples_buf[*it] = tx_samples_freq[idx]; 
   }

   return true;
}

} /* namespace svl */

} /* namespace gr */
