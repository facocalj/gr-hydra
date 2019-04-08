#ifndef INCLUDED_HYDRA_RESAMPLER_H
#define INCLUDED_HYDRA_RESAMPLER_H

#include <thread>

#include "hydra/hydra_buffer.h"
#include "hydra/types.h"


namespace hydra {

template <typename input_data_type = iq_sample, typename output_data_type = iq_window>
class resampler
{
  private:
    // Pointer to the buffer object
    std::shared_ptr<hydra_buffer<input_data_type>> p_input_buffer;
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
        std::shared_ptr<hydra_buffer<input_data_type>> input_buffer,
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
    std::shared_ptr<hydra_buffer<input_data_type>> input_buffer,
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

template <>
void
inline resampler<iq_sample, iq_window>::run()
{
  // Vector of IQ samples that comprise a FFT window
  iq_window temp_object(u_fft_size);

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

template <>
void
inline resampler<iq_window, iq_sample>::run()
{
  // Vector of IQ samples that comprise a FFT window
  iq_window temp_object(u_fft_size);

  // Only a single window per time, for resampling

  // If the destructor has been called
  while (not stop_thread)
  {
    // Check whether the buffer has windows
    if (p_input_buffer->size())
    {
      // Insert IQ samples from the input buffer into the window
      temp_object = p_input_buffer->read(1)[0];
    }

    //TODO Check the need for resampling here

    // Write all the IQ sample in this window
    output_buffer.write(temp_object.begin(), temp_object.end());
  }
};



} // Namespace

#endif
