#include "hydra/hydra_resampler.h"


namespace hydra {

resampler::resampler()
{
};

resampler::resampler(
    hydra_buffer<iq_sample>* input_buffer,
    double sampling_rate,
    size_t fft_size)
{
  // Thread stio flag
  stop_thread = false;

  // Number of IQ samples per FFT window
  u_fft_size = fft_size;
  // The input buffer
  p_input_buffer = input_buffer;

  // Create a thread to receive the data
  run_thread = std::make_unique<std::thread>(&resampler::run, this);
};


window*
resampler::consume()
{
  // If there is at least a single window in the buffer
  if (output_buffer.size())
  {
    // Return an array with the element
    window* k = new window(output_buffer.read().front());
    return k;
  }
  // Otherwise, return null pointer
  else
  {
    return nullptr;
  }
};


void
resampler::run()
{
  // Vector of IQ samples that comprise a FFT window
  window temp_window(u_fft_size);
  // Integer to hold the current size of the queue
  long long int ll_cur_size;

  // If the destructor has been called
  while (not stop_thread)
  {
    // Check whether the buffer has enough IQ samples
    if (p_input_buffer->size() >= u_fft_size)
    {
      // Insert IQ samples from the input buffer into the window
      temp_window = p_input_buffer->read(u_fft_size);
    }

    //TODO Check the need for resampling here

    output_buffer.write(temp_window);
  }
};



} // namespace hydra
