#include "hydra/hydra_virtual_rf.h"

namespace hydra {



virtual_rf::virtual_rf()
{
};

int
virtual_rf::set_freq(double cf)
{
  if (cf == g_centre_freq) return 0;

   double old_cf = g_centre_freq;
   g_centre_freq = cf;

   int err = p_hypervisor->notify(*this, Hypervisor::SET_TX_MAP);
   if (err < 0)
      g_centre_freq = old_cf;

   return err;
}

void
virtual_rf::set_bandwidth(double bw)
{
  if (bw == g_bandwidth) return;


   double old_bw = g_bandwidth;
   g_bandwidth = bw;

   int err = p_hypervisor->notify(*this, Hypervisor::SET_TX_MAP);
   if (err < 0)
      g_bandwidth = old_bw;
}


virtual_rf_sink::virtual_rf_sink(
    std::shared_ptr<hydra_buffer<iq_window>> input_buffer,
    double d_bandwidth,
    double d_centre_freq,
    unsigned int u_fft_size,
    Hypervisor* hypervisor)
{
  // Number of IQ samples per FFT window
  g_fft_size = u_fft_size;
  // Front-end BW
  g_bandwidth = d_bandwidth;
  // Front-end CF
  g_centre_freq = d_centre_freq;

  // The input buffer
  p_input_buffer = input_buffer;

  p_hypervisor = hypervisor; // temporary

  // create fft object
  g_fft_complex  = sfft_complex(new fft_complex(g_fft_size));

  p_hypervisor->notify(*this, Hypervisor::SET_TX_MAP);
}


void
virtual_rf_sink::set_tx_mapping(const iq_map_vec &iq_map)
{
  g_tx_map = iq_map;
}

bool
virtual_rf_sink::map_tx_samples(iq_sample *samples_buf)
{

  // Try to get a window from the resampler
  auto buf  = p_input_buffer->read(1);

  // Return false if the window is empty
  if (buf.empty()){return false;}

  // Copy samples in TIME domain to FFT buffer, execute FFT
  g_fft_complex->set_data(&buf[0][0], g_tx_fft_size);
  g_fft_complex->execute();
  iq_sample *outbuf = g_fft_complex->get_outbuf();

  // map samples in FREQ domain to samples_buff
  // perfors fft shift
  std::copy(g_tx_map.begin(), g_tx_map.end(), samples_buf);

  return true;
}

virtual_rf_source::virtual_rf_source(
    double d_bandwidth,
    double d_centre_freq,
    unsigned int u_fft_size,
    Hypervisor* hypervisor)
{
  // Number of IQ samples per FFT window
  g_fft_size = u_fft_size;
  // Front-end BW
  g_bandwidth = d_bandwidth;
  // Front-end CF
  g_centre_freq = d_centre_freq;

  // The input buffer

  p_output_buffer= std::make_shared<hydra_buffer<iq_window>>(1000);

  p_hypervisor = hypervisor; // temporary

  // create fft object
  g_ifft_complex  = sfft_complex(new fft_complex(g_rx_fft_size, false));

  p_hypervisor->notify(*this, Hypervisor::SET_RX_MAP);
}


void
virtual_fr_source::set_rx_mapping(const iq_map_vec &iq_map)
{
  g_rx_map = iq_map;
}

void
virtual_rf_source::demap_iq_samples(const iq_sample *samples_buf, size_t len)
{
  // If the receiver chain was not defined
  if (not b_receiver){ return;}

  /* Copy the samples used by this radio */
  for (size_t idx = 0; idx < g_rx_fft_size; ++idx)
    g_ifft_complex->get_inbuf()[idx] = samples_buf[g_rx_map[idx]];

  // Perform the FFT operation
  g_ifft_complex->execute();

  /* Append new samples */
  rx_resampler->buffer()->write(g_ifft_complex->get_outbuf(), g_rx_fft_size);
}



} // End namespace
