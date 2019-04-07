#include "hydra/hydra_resampler.h"


namespace hydra {




RxBuffer::RxBuffer(
   std::deque<std::complex<float>>* input_buffer,
   std::mutex* in_mtx,
   float sampling_rate,
   unsigned int fft_size,
   bool pad)
{
  // Time threshold - round the division to a long int
  l_threshold = llrint(fft_size * 1e6 / sampling_rate);

  // Number of IQ samples per FFT window
  u_fft_size = fft_size;
  // The input buffer
  p_input_buffer = input_buffer;
  // Get the mutex
  p_in_mtx = in_mtx;
  // Get the padding flag
  b_pad = pad;

  // Create a thread to receive the data
  buffer_thread = std::make_unique<std::thread>(&RxBuffer::run, this);
}


const window *
RxBuffer::consume()
{
   /* We do an ugly thing here: we have 'never_delete'
    * storing the data want to be consumed by the hypervisor
    */
   std::lock_guard<std::mutex> _l(out_mtx);
   if (output_buffer.size())
   {
      never_delete = output_buffer.front();
      output_buffer.pop_front();

      return &never_delete;
   }

   return nullptr;
}


void
RxBuffer::run()
{
  // Thread stop condition
  thr_stop = false;
  // Vector of IQ samples that comprise a FFT window
  std::vector<std::complex<float>> window;
  // Reserve number of elements
  window.reserve(u_fft_size);
  // Empty IQ sample, as zeroes
  std::complex<float> empty_iq = {0.0, 0.0};
  // Integer to hold the current size of the queue
  long long int ll_cur_size;

  // Consume indefinitely
  while(true)
  {
     // Wait for "threshold" nanoseconds
     //std::this_thread::sleep_for(std::chrono::microseconds(l_threshold));

     // If the destructor has been called
     if (thr_stop){ return; }
#if 0
     // Windows not being consumed
     if (output_buffer.size() > 100)
     {
       //std::lock_guard<std::mutex> _l(out_mtx);
       //output_buffer.pop_front();
       //std::cerr << "Too many windows!" << std::endl;
     }
#endif

     {
        std::lock_guard<std::mutex> _p(*p_in_mtx);
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
            std::lock_guard<std::mutex> _l(out_mtx);
            output_buffer.push_back(window);
          }

           ll_cur_size = p_input_buffer->size();
        }
#if 0
        // Without padding, just transmit an empty window and wait for the next one
        else
        {
           // Fill the window with complex zeroes
           window.assign(u_fft_size, empty_iq);

           // Add the window to the output buffer
           std::lock_guard<std::mutex> _l(out_mtx);
           output_buffer.push_back(window);
           out_mtx.unlock();
        }
#endif
     }
  }
}




} // namespace hydra
