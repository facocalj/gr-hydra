#include <iostream>
#include <mutex>
#include <queue>
#include <boost/circular_buffer.hpp>

// typedef std::complex<float> gr_complex;
// typedef std::complex<float> iq_sample;
// typedef std::deque<iq_sample> iq_stream;
// typedef std::vector<iq_sample> window;
// typedef std::deque<window> window_stream;


// Template to allow different data types and container types
template <typename data_type, template<typename, typename> class container_type = boost::circular_buffer>
class xvl_buffer
{
  private:
    // The actual buffer, a container of the given type of data
    container_type<data_type, std::allocator<data_type>> buffer;
    // Output mutex
    std::mutex buffer_mutex;

  public:
    // Default constructor
    xvl_buffer();

    // Class Constructor
    xvl_buffer(unsigned int size = 1);

    // Read a number of elements
    template <unsigned int num_elements = 1>
    std::array<data_type, num_elements> read();

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

template <typename data_type, template<typename, typename> class container_type>
xvl_buffer<data_type, container_type>::xvl_buffer(unsigned int size)
{
  // Allocate a given size for the buffer
  buffer = container_type<data_type, std::allocator<data_type>> (size);
};

// Read a number of elements
template <typename data_type, template<typename, typename> class container_type>
template <unsigned int num_elements>
std::array<data_type, num_elements>
xvl_buffer<data_type, container_type>::read()
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Create ana rray to hold the elements
  std::array<data_type, num_elements> elements;

  // Copy the given number of elements to the temp array
  std::copy(buffer.begin(), buffer.begin()+num_elements, elements.begin());
  // Remove the given numbert of elements from the buffer
  buffer.erase(buffer.begin(), buffer.begin()+num_elements);

  std::cout << buffer.size() << std::endl;

  // Return the array of elements
  return elements;
};

// Write a number of the same element in the buffe:
template <typename data_type, template<typename, typename> class container_type>
void
xvl_buffer<data_type, container_type>::write(data_type element, unsigned int num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Insert N elements at the end
  buffer.insert(buffer.end(), num_elements, element);

  std::cout << buffer.size() << std::endl;
};
// Write a number of elements to the buffer
template <typename data_type, template<typename, typename> class container_type>
template <typename iterator>
void
xvl_buffer<data_type, container_type>::write(iterator begin_it, unsigned int num_elements)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Assign N elements at the end, starting from the begin iterator
  buffer.insert(buffer.end(), begin_it, begin_it+num_elements);

  std::cout << buffer.size() << std::endl;
};

// Write a range of elements to the buffer
template <typename data_type, template<typename, typename> class container_type>
template <typename iterator>
void
xvl_buffer<data_type, container_type>::write(iterator begin_it, iterator end_it)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Assign a range of elements at the end, between the begin and end iterators
  buffer.insert(buffer.end(), begin_it, end_it);

  std::cout << buffer.size() << std::endl;
};

// Access operator
template <typename data_type, template<typename, typename> class container_type>
data_type
xvl_buffer<data_type, container_type>::operator[](unsigned int position)
{
  // Lock access to the inner buffer structure
  std::lock_guard<std::mutex> lock(buffer_mutex);

  // Return element at a given position
  return buffer[position];
};





int
main(int argc, const char *argv[])
{
  // Create a circular buffer with a capacity for 3 integers.
  xvl_buffer<int, boost::circular_buffer> cb(9);

  // Insert some elements into the buffer.
  cb.write(1);
  cb.write(2);
  cb.write(3);
  cb.write(4);
  cb.write(5);
  cb.write(6);

  int a = cb[0];  // a == 1
  int b = cb[1];  // b == 2
  int c = cb[2];  // c == 3

  // The buffer is full now, pushing subsequent
  // elements will overwrite the front-most elements.
  cb.read<2>();  // 5 is removed.
  cb.read<1>(); // 3 is removed.

  // The buffer now contains 3, 4 and 5.

  a = cb[0];  // a == 3
  b = cb[1];  // b == 4
  c = cb[2];  // c == 5

  // Elements can be popped from either the front or the back.


  int d = cb[0];  // d == 4

  std::cout << a << std::endl;
  std::cout << b << std::endl;
  std::cout << c << std::endl;

  return 0;
 }
