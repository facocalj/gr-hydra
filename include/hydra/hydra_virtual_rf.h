#ifndef HYDRA_VIRTUAL_RF_INCLUDE_H
#define HYDRA_VIRTUAL_RF_INCLUDE_H

#include <hydra/types.h>
#include <hydra/hydra_buffer.hpp>
#include <hydra/hydra_hypervisor.h>
#include <hydra/hydra_fft.h>

namespace hydra
{

class virtual_rf
{
  protected:
    // FFT size of the virtual RF front-end
    size_t g_fft_size;
    // FFT map of the virtual RF front-end
    iq_map_vec g_fft_map;
    // FFT object of the virtual RF front-end
    sfft_complex g_fft_complex;

  public:
    // Update the virtual RF front-end's FFT size
    void set_fft_size(size_t new_size)
    {
      // Uptete the FFT size
      g_fft_size = new_size;
    };
    void set_fft_map(const iq_map_vec &iq_map)
    {
      // Update the FFT mapping
      g_fft_map = iq_map;
    };

};


class virtual_rf_sink : public virtual_rf
{
  public:
    virtual_rf_sink();

    ~virtual_rf_sink();

    virtual_rf_sink(
      std::shared_ptr<hydra_buffer<iq_window>> input_buffer,
      unsigned int u_fft_size,
      const iq_map_vec &iq_map);

    // Map window of time domain samples to frequency domain
    bool map_to_freq(iq_sample *samples_buf);

};

class virtual_rf_source: public virtual_rf
{
  private:
    // Pointer to the output buffer
    std::shared_ptr<hydra_buffer<iq_window>> p_output_buffer;

  public:
    virtual_rf_source();

    ~virtual_rf_source();

    virtual_rf_source(
      unsigned int u_fft_size,
      const iq_map_vec &iq_map);

    // Return pointer to the internal buffer
    std::shared_ptr<hydra_buffer<iq_window>> buffer()
    {
      return p_output_buffer;
    };

    // Map window of frequency domain samples to time domain
    bool map_to_time(iq_sample* input_buffer);
};

} // Namespace

#endif
