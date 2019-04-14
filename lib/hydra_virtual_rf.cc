#include "hydra/hydra_virtual_rf.h"

namespace hydra {



virtual_rf::virtual_rf()
{
};



virtual_rf_sink::virtual_rf_sink(
    std::shared_ptr<hydra_buffer<iq_window>> input_buffer,
    unsigned int u_fft_size,
    const iq_map_vec &iq_map)
{
  // Number of IQ samples per FFT window
  g_fft_size = u_fft_size;
  // The FFT mapping
  g_fft_map = iq_map;

  // The input buffer
  p_input_buffer = input_buffer;

  // Create FFT object
  g_fft_complex  = sfft_complex(new fft_complex(g_fft_size));
}

void
virtual_rf_sink::map_to_freq(iq_sample* output_window)
{
  // Try to get a window from the resampler
  iq_window time_window = p_input_buffer->read_one();

  // Return false if the window is empty
  if (time_window.empty()){return false;}

  // Copy samples in time domain to FFT buffer, execute FFT
  g_fft_complex->set_data(&time_window[0], g_fft_size);
  // Execute the FFT
  g_fft_complex->execute();

  // Get the frequency domain window
  iq_sample* freq_window = g_fft_complex->get_outbuf();

  // Map samples in frequency domain to the output buffer, with an FFT shift
  size_t index = 0;
  for (auto it = g_fft_map.begin(); it != g_fft_map.end(); ++it, ++index)
  {
    // Very ugly, but this seems to be the only way to make the FFT shift
    output_window[*it]  = freq_window[index];
  }

  // Life is great, return true
  return true;
}

virtual_rf_source::virtual_rf_source(
    unsigned int u_fft_size,
    const iq_map_vec &iq_map)
{
  // Number of IQ samples per FFT window
  g_fft_size = u_fft_size;
  // The IFFT mapping
  g_fft_map = iq_map;

  // The output buffer
  p_output_buffer= std::make_shared<hydra_buffer<iq_window>>(1000);

  // create IFFT object
  g_fft_complex  = sfft_complex(new fft_complex(g_fft_size, false));
}

void
virtual_rf_source::map_to_time(const iq_sample *input_buffer)
{
  // Map samples in input buffer to the time domain, with an IFFT shift
  for (size_t index = 0; index < g_fft_size; ++index)
  {
    // Very ugly, but this seems to be the only way to make the IFFT shift
    g_fft_complex->get_inbuf()[index] = input_buffer[g_fft_map[index]];
  }
  // Perform the I<Up>FFT operation
  g_fft_complex->execute();

  // Write the time domain samples in the outputbuffer
  p_output_buffer->write(g_fft_complex->get_outbuf());
}



} // End namespace
