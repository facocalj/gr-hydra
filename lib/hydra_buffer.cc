#include "hydra/hydra_buffer.h"


namespace hydra {

// template class hydra_buffer<iq_sample>;
// template class hydra_buffer<window>;

TxBuffer::TxBuffer(window_stream* input_buffer,
                   std::mutex* in_mtx,
                   double sampling_rate,
                   unsigned int fft_size)
{
  // Time threshold - round the division to a long int
  threshold = fft_size * 1e9 / sampling_rate;

  // Number of IQ samples per FFT window
  u_fft_size = fft_size;
  // The input buffer
  p_input_buffer = input_buffer;
  // Get the input mutex
  p_in_mtx = in_mtx;

  // Create a thread to receive the data
  buffer_thread = std::make_unique<std::thread>(&TxBuffer::run, this);
}

void
TxBuffer::run()
{
#if 0

  // Thread stop condition
  thr_stop = false;
  // Empty IQ sample, as zeroes
  iq_sample empty_iq = {0.0, 0.0};
  // Temporary array to hold a window worth of IQ samples
  std::vector<iq_sample> window;
  window.reserve(u_fft_size);

  /* Produce indefinitely */
  while(not thr_stop)
  {
    // Check if there's a valid window ready for transmission
    if (not p_input_buffer->empty())
    {
      // Lock access to the buffer
      std::lock_guard<std::mutex> _inmtx(*p_in_mtx);

      // Copy the first window from the input buffer and pop it
      window = p_input_buffer->front();
      p_input_buffer->pop_front();

      // Lock access to the output buffer
      std::lock_guard<std::mutex> _omtx(out_mtx);
      output_buffer.insert(output_buffer.end(), window.begin(), window.end());
    }
    /* If the queue of windows is empty at the moment */
    else
    {
#if 0
      /* Stream empty IQ samples that comprise the window duration */
      std::lock_guard<std::mutex> _omtx(out_mtx);
      output_buffer.insert(output_buffer.end(), u_fft_size, empty_iq);
      /* Wait for "threshold" nanoseconds */
      std::this_thread::sleep_for(std::chrono::nanoseconds(threshold));
#endif
    } // End padding
  }
#endif
}

void
TxBuffer::produce(const gr_complex *buf, size_t len)
{
#if 0
  std::lock_guard<std::mutex> _l(*p_in_mtx);
  p_input_buffer->push_back(window(buf, buf + len));
#endif
  std::lock_guard<std::mutex> _l(out_mtx);
  output_buffer.insert(output_buffer.end(), buf, buf + len);
}

} // namespace hydra
