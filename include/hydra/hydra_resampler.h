#ifndef INCLUDED_HYDRA_RESAMPLER_H
#define INCLUDED_HYDRA_RESAMPLER_H


#include <mutex>
#include <thread>

#include "hydra/types.h"

namespace hydra {

template <typename data_type>
class resampler
{
  public:
  resampler();

  resampler(hydra_buffer<data_type>* a, double b, size_t c);

  window* consume();
};


class RxBuffer
{
private:
   // Pointer to the buffer object
   iq_stream * p_input_buffer;
   // Hold the FFT size
   unsigned int u_fft_size;
   // Flag to indicate use of padding or empty frame
   bool b_pad;
   // Time threshold for padding/empty frames
   long long int l_threshold;

   // Queue of arrays of IQ samples to be used for the FFT
   window_stream output_buffer;

   // Pointer to the buffer thread
   std::unique_ptr<std::thread> buffer_thread;
   // Pointer to the input mutex
   std::mutex* p_in_mtx;
   // Output mutex
   std::mutex out_mtx;
   // Thread stop condition
   bool thr_stop;

public:
   // CTOR
   RxBuffer(iq_stream* input_buffer,
            std::mutex* in_mtx,
            float sampling_rate,
            unsigned int fft_size,
            bool pad);

   // DTOR
   ~RxBuffer()
   {
      // Stop the thread
      thr_stop = true;
      // Stop the buffer thread
      buffer_thread->join();
   };

   // Method to receive UDP data and put it into a buffer
   void run();

   window never_delete;
   const iq_window * consume();

   // Returns pointer to the output buffer of windows
   window_stream* windows(){return &output_buffer;};

   // Returns pointer to the mutex
   std::mutex* mutex() {return &out_mtx;};
};


} // namespace hydra

#endif
