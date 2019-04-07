#include "hydra/hydra_buffer.h"

#include <numeric>
#include <boost/format.hpp>

namespace hydra {

template <typename data_type, template<typename, typename> class container_type>
hydra_buffer<data_type, container_type>::hydra_buffer(unsigned int size)
{
  // Allocate a given size for the buffer
  buffer = container_type<data_type, std::allocator<data_type>> (size);
};

// Read a number of elements
template <typename data_type, template<typename, typename> class container_type>
std::vector<data_type>
hydra_buffer<data_type, container_type>::read(unsigned int num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Create an array to hold the elements
  std::vector<data_type> elements(num_elements);

  // Copy the given number of elements to the temp array
  std::copy(buffer.begin(), buffer.begin()+num_elements, elements.begin());
  // Remove the given numbert of elements from the buffer
  buffer.erase(buffer.begin(), buffer.begin()+num_elements);

  // Return the array of elements
  return elements;
};

// Write a number of the same element in the buffe:
template <typename data_type, template<typename, typename> class container_type>
void
hydra_buffer<data_type, container_type>::write(data_type element, unsigned int num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If the writting process will not overflow/overwrite the buffer
  if (buffer.size() + num_elements <= buffer.capacity())
  {
    // Insert N elements at the end
    buffer.insert(buffer.end(), num_elements, element);
  }
};
// Write a number of elements to the buffer
template <typename data_type, template<typename, typename> class container_type>
template <typename iterator>
void
hydra_buffer<data_type, container_type>::write(iterator begin_it, unsigned int num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If the writting process will not overflow/overwrite the buffer
  if (buffer.size() + num_elements <= buffer.capacity())
  {
    // Assign N elements at the end, starting from the begin iterator
    buffer.insert(buffer.end(), begin_it, begin_it+num_elements);
  }
};

// Write a range of elements to the buffer
template <typename data_type, template<typename, typename> class container_type>
template <typename iterator>
void
hydra_buffer<data_type, container_type>::write(iterator begin_it, iterator end_it)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // If the writting process will not overflow/overwrite the buffer
  if (buffer.size() + std::distance(begin_it, end_it) <= buffer.capacity())
  {
    // Assign a range of elements at the end, between the begin and end iterators
    buffer.insert(buffer.end(), begin_it, end_it);
  }
};

// Access operator
template <typename data_type, template<typename, typename> class container_type>
data_type
hydra_buffer<data_type, container_type>::operator[](unsigned int position)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Return element at a given position
  return buffer[position];
};





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
