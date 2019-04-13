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
    size_t g_fft_size; // Subcarriers used by this VRadio
    double g_centre_freq;      // Central frequency
    double g_bandwidth;      // Bandwidth

    iq_map_vec g_iq_map;

    // pointer to this VR hypervisor
    Hypervisor *p_hypervisor; // temporary

  public:

  virtual void set_fft_size(unsigned int size) = 0;
  virtual void set_cenfre_freq(double bw) = 0;

  virtual void set_bandwidth(double bw) = 0;

};


class virtual_rf_sink : public virtual_rf
{

  private:

    iq_map_vec g_tx_map;
    sfft_complex g_fft_complex;

  public:
    virtual_rf_sink();

    ~virtual_rf_sink();

    virtual_rf_sink(
      std::shared_ptr<hydra_buffer<iq_window>> input_buffer,
      double d_bandwidth,
      double d_centre_freq,
      unsigned int u_fft_size);

    void set_fft_size(unsigned int size);
    void set_cenfre_freq(double bw);
    void set_bandwidth(double bw);

    int set_tx_freq(double cf);
    void set_tx_bandwidth(double bw);
    void set_tx_mapping(const iq_map_vec &iq_map);

    size_t const set_tx_fft(size_t n) {return g_fft_size = n;}

    bool map_tx_samples(iq_sample *samples_buf); // called by the hypervisor

};

class virtual_rf_source: public virtual_rf
{

  private:

    iq_map_vec g_tx_map;
    sfft_complex g_fft_complex;

    std::shared_ptr<hydra_buffer<iq_window>> p_output_buffer;

  public:
    virtual_rf_source();

    ~virtual_rf_source();

    virtual_rf_source(
      double d_bandwidth,
      double d_centre_freq,
      unsigned int u_fft_size);

    // Return pointer to the internal buffer
    std::shared_ptr<hydra_buffer<iq_window>> buffer()
    {
      return p_output_buffer;
    };

    void set_fft_size(unsigned int size);
    void set_cenfre_freq(double bw);
    void set_bandwidth(double bw);

    int set_tx_freq(double cf);
    void set_tx_bandwidth(double bw);
    void set_tx_mapping(const iq_map_vec &iq_map);

    size_t const set_tx_fft(size_t n) {return g_fft_size = n;}

    bool map_tx_samples(iq_sample *samples_buf); // called by the hypervisor

};




} // Namespace

#endif
