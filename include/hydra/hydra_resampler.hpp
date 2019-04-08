#ifndef INCLUDED_HYDRA_RESAMPLER_H
#define INCLUDED_HYDRA_RESAMPLER_H

#include <thread>

#include "hydra/hydra_buffer.h"
#include "hydra/types.h"


namespace hydra {

template <typename input_data_type = iq_sample, typename output_data_type = window>
class resampler
{
  private:
    // Pointer to the buffer object
    hydra_buffer<input_data_type>* p_input_buffer;
    // Inernal buffer object
    hydra_buffer<output_data_type> output_buffer;

    // Event loop thread
    std::unique_ptr<std::thread> run_thread;
    // Thread stop condition
    bool stop_thread;

    // Event loop method
    void run();

    // Hold the FFT size
    unsigned int u_fft_size;

  public:
    // Default constructor
    resampler();

    // Consturctor
    resampler(
        hydra_buffer<input_data_type>* input_buffer,
        double sampling_rate,
        size_t fft_size);

    // Consume a window
    output_data_type* consume();

   // Stop the thread
   void stop()
   {
      // Stop the thread
      stop_thread = true;
      // Stop the buffer thread
      run_thread->join();
   };

  // Return pointer to the internal buffer
  hydra_buffer<output_data_type>* buffer()
  {
    return &output_buffer;
  };

};


template <typename input_data_type, typename output_data_type>
resampler<input_data_type, output_data_type>::resampler()
{
};

template <typename input_data_type, typename output_data_type>
resampler<input_data_type, output_data_type>::resampler(
    hydra_buffer<input_data_type>* input_buffer,
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


template <typename input_data_type, typename output_data_type>
output_data_type*
resampler<input_data_type, output_data_type>::consume()
{
  // If there is at least a single window in the buffer
  if (output_buffer.size())
  {
    // Return an array with the element
    output_data_type* k = new window(output_buffer.read().front());
    return k;
  }
  // Otherwise, return null pointer
  else
  {
    return nullptr;
  }
};


template <typename input_data_type, typename output_data_type>
void
resampler<input_data_type, output_data_type>::run()
{
  // Vector of IQ samples that comprise a FFT window
  output_data_type temp_object(u_fft_size);
  // Integer to hold the current size of the queue
  long long int ll_cur_size;

  // If the destructor has been called
  while (not stop_thread)
  {
    // Check whether the buffer has enough IQ samples
    if (p_input_buffer->size() >= u_fft_size)
    {
      // Insert IQ samples from the input buffer into the window
      temp_object = p_input_buffer->read(u_fft_size);
    }

    //TODO Check the need for resampling here

    output_buffer.write(temp_object);
  }
};

} // Namespace

#endif
