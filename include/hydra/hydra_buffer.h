#ifndef INCLUDED_HYDRA_BUFFER_H
#define INCLUDED_HYDRA_BUFFER_H

#include <queue>
#include <vector>
#include <complex>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <math.h>

#include <boost/circular_buffer.hpp>
#include "hydra/types.h"


namespace hydra {

// Template to allow different data types and container types
template <typename data_type, template<typename, typename> class container_type = boost::circular_buffer>
class hydra_buffer
{
  private:
    // The actual buffer, a container of the given type of data
    container_type<data_type, std::allocator<data_type>> buffer;
    // Output mutex
    std::mutex buffer_mutex;

  public:
    // Default constructor
    hydra_buffer();

    // Class Constructor
    hydra_buffer(unsigned int size);

    // Return the current size of the buffer
    unsigned int size()
    {
      // Lock access to the inner buffer structure
      std::lock_guard<std::mutex> lock(buffer_mutex);

      return buffer.size();
    };

    // Return the capacity of the buffer
    unsigned int capacity(){ return buffer.capacity();};

    // Read a number of elements
    std::vector<data_type> read(unsigned int num_elements = 1);

    // Write a number of the same element in the buffer
    void write(data_type element, unsigned int num_elements = 1);

    // Write a number of elements to the buffer
    template <typename iterator>
    void write(iterator begin_it, unsigned int num_elements = 1);

    // Write a range of elements to the buffer
    template <typename iterator>
    void write(iterator begin_it, iterator end_it);

    // Access operator
    data_type operator[](unsigned int position);
};


class TxBuffer
{
public:
   // Constructor
   TxBuffer(window_stream* input_buffer,
            std::mutex* in_mtx,
            double sampling_rate,
            unsigned int fft_size);

   // Destructor
   ~TxBuffer()
   {
     /* Stop the thread */
     thr_stop = true;
     /* Stop the buffer thread */
     buffer_thread->join();
   };

   void produce(const gr_complex *buf, size_t len);

   // Method to transmit UDP data from a buffer
   void run();

   // Returns pointer to the output buffer, a stream of IQ samples
   iq_stream *stream(){return &output_buffer;};

   // Returns pointer to the mutex
   std::mutex* mutex() {return &out_mtx;};

private:
   // Pointer to the input FFT
   window_stream* p_input_buffer;
   // Hold the FFT size
   unsigned int u_fft_size;
   // Time threshold for padding/empty frames
   std::size_t threshold;
   // Output deque

   iq_stream output_buffer;

   // Pointer to the buffer thread
   std::unique_ptr<std::thread> buffer_thread;
   // Pointer to the mutex
   std::mutex* p_in_mtx;
   // Output mutex
   std::mutex out_mtx;
   // Thread stop condition
   bool thr_stop;
};

typedef std::shared_ptr<TxBuffer> TxBufferPtr;

} // namespace hydra

#endif
