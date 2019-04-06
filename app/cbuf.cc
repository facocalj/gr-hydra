#include <iostream>
#include <boost/circular_buffer.hpp>


// typedef std::complex<float> gr_complex;
// typedef std::complex<float> iq_sample;
// typedef std::deque<iq_sample> iq_stream;
// typedef std::vector<iq_sample> window;
// typedef std::deque<window> window_stream;




// template <class data_type,  class container_type>
class xvl_buffer
{

  // data_type element;
  // container_type buffer;

   // Output mutex
   std::mutex buffer_mutex;

  public:
    boost::circular_buffer<int> buffer;

    xvl_buffer(int size)
    {
      buffer = boost::circular_buffer<int> (size);
    };


    template<class type>
    void push_back(type element)
    {
      std::mutex buffer_mutex;

      buffer.push_back(element);

      std::cout << buffer.size() << std::endl;
    };


    void read(unsigned int num_element = 1)
    {
      // Lock access to the inner buffer structure
      std::lock_guard<std::mutex< lock(buffer_mutex);

      buffer.pop_front();
      std::cout << buffer.size() << std::endl;
    };


    void write(unsigned int num_element = 1)
    {
      // Lock access to the inner buffer structure
      std::lock_guard<std::mutex< lock(buffer_mutex);

      buffer.pop_back();
      std::cout << buffer.size() << std::endl;
    };

    int operator[](int n)
    {
      return buffer[n];
    };

};




int
main(int argc, const char *argv[])
{
  // Create a circular buffer with a capacity for 3 integers.

  xvl_buffer cb(3*10);

  // Insert some elements into the buffer.
  cb.push_back(1);
  cb.push_back(2);
  cb.push_back(3);

  int a = cb[0];  // a == 1
  int b = cb[1];  // b == 2
  int c = cb[2];  // c == 3

  // The buffer is full now, pushing subsequent
  // elements will overwrite the front-most elements.
  cb.pop_back();  // 5 is removed.
  cb.pop_front(); // 3 is removed.

  cb.push_back(4);  // Overwrite 1 with 4.
  cb.push_back(5);  // Overwrite 2 with 5.

  // The buffer now contains 3, 4 and 5.

  a = cb[0];  // a == 3
  b = cb[1];  // b == 4
  c = cb[2];  // c == 5

  // Elements can be popped from either the front or the back.


  int d = cb[0];  // d == 4

  return 0;
 }
