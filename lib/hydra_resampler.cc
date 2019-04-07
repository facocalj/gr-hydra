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


const window*
resampler::consume()
{
   /* We do an ugly thing here: we have 'never_delete'
    * storing the data want to be consumed by the hypervisor
    */
   if (output_buffer.size())
   {
      never_delete = output_buffer.read<u_fft_size>();
      output_buffer.pop_front();

      return &never_delete;
   }

   return nullptr;
};


void
resampler::run()
{
  // Vector of IQ samples that comprise a FFT window
  std::complex<float>> window(u_fft_size);
  // Empty IQ sample, as zeroes
  std::complex<float> empty_iq = {0.0, 0.0};
  // Integer to hold the current size of the queue
  long long int ll_cur_size;

  // If the destructor has been called
  while (not stop_thread)
  {
    // Get the current size of the queue
    ll_cur_size = p_input_buffer->size();
    // Check whether the buffer has enough IQ samples
    while (ll_cur_size > 0)
    {
      if (ll_cur_size >= u_fft_size){
        // Insert IQ samples from the input buffer into the window
        window.assign(p_input_buffer->begin(),
                      p_input_buffer->begin() + u_fft_size); // 0..C

        // Erase the beginning of the queue
        p_input_buffer->erase(p_input_buffer->begin(),
                              p_input_buffer->begin() + u_fft_size);

      }
      else
      {
        // Copy the current amount of samples from the buffer to the window
        window.assign(p_input_buffer->begin(),
                      p_input_buffer->begin() + ll_cur_size); // 0..C-1
        // Erase the beginning of the queue
        p_input_buffer->erase(p_input_buffer->begin(),
                              p_input_buffer->begin() + ll_cur_size);

        // Fill the remainder of the window with complex zeroes
        window.insert(window.begin() + ll_cur_size,
                      u_fft_size - ll_cur_size,
                      empty_iq); // C..F-1
      }

      {
        output_buffer.write(window);
      }

       ll_cur_size = p_input_buffer->size();
    }
  }
};




} // namespace hydra
