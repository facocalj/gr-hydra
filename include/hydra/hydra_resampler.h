#ifndef INCLUDED_HYDRA_RESAMPLER_H
#define INCLUDED_HYDRA_RESAMPLER_H

#include <thread>

#include "hydra/hydra_buffer.h"
#include "hydra/types.h"


namespace hydra {

class resampler
{
  private:
    // Pointer to the buffer object
    hydra_buffer<iq_sample>* p_input_buffer;
    // Inernal buffer object
    hydra_buffer<window> output_buffer;

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
        hydra_buffer<iq_sample>* input_buffer,
        double sampling_rate,
        size_t fft_size);

    // Consume a window
    window* consume();

   // Stop the thread
   void stop()
   {
      // Stop the thread
      stop_thread = true;
      // Stop the buffer thread
      run_thread->join();
   };

  // Return pointer to the internal buffer
  hydra_buffer<window>* buffer()
  {
    return &output_buffer;
  };

};

} // Namespace

#endif
