#ifndef INCLUDED_HYDRA_UHD_INTERFACE_H
#define INCLUDED_HYDRA_UHD_INTERFACE_H

#include "hydra/hydra_fft.h"
#include "hydra/types.h"

#include <uhd/usrp/multi_usrp.hpp>
#include <zmq.hpp>
#include <memory>
#include <mutex>

#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>
#include <uhd/usrp/usrp.h>
#include <opencv2/opencv.hpp>


namespace hydra {

class abstract_device
{
 public:
 abstract_device(){}

  virtual void send(const window &buf, size_t len) { std::cerr << __PRETTY_FUNCTION__ << " not implemented" << std::endl;};
  virtual size_t receive(window &buf, size_t len) { std::cerr << __PRETTY_FUNCTION__ << " not implemented" << std::endl;};

  virtual void set_tx_config(double freq, double rate, double gain){ g_tx_freq = freq; g_tx_rate = rate; g_tx_gain = gain;};
  virtual void set_rx_config(double freq, double rate, double gain){ g_rx_freq = freq; g_rx_rate = rate; g_rx_gain = gain;};

  virtual void release() {};

 protected:
  double g_tx_freq;
  double g_tx_rate;
  double g_tx_gain;
  double g_rx_freq;
  double g_rx_rate;
  double g_rx_gain;
};


class device_uhd: public abstract_device
{
public:
  device_uhd(std::string device_args = "");
  ~device_uhd();

  void send(const window &buf, size_t len);
  size_t receive(window &buf, size_t len);

  void set_tx_config(double freq, double rate, double gain);
  void set_rx_config(double freq, double rate, double gain);

  virtual void release();

private:
  uhd::usrp::multi_usrp::sptr usrp;

  uhd::rx_streamer::sptr rx_stream;
  uhd::tx_streamer::sptr tx_stream;

};


class device_image_gen: public abstract_device
{
public:
   device_image_gen(std::string device_args = "");
   void send(const window &buf, size_t len);
   size_t receive(window &buf, size_t len);

private:
   samples_vec g_iq_samples;
   std::string file_read;
};


class device_loopback: public abstract_device
{
public:
   device_loopback(std::string device_args = "");
   void send(const window &buf, size_t len);
   size_t receive(window &buf, size_t len);

private:
   std::mutex g_mutex;
   window_stream g_windows_vec;
};

class device_network: public abstract_device
{
public:
   device_network(std::string host_add, std::string remote_addr);
   void send(const window &buf, size_t len);
   size_t receive(window &buf, size_t len);

private:
   bool init_tx, init_rx;

   std::string g_host_addr, g_remote_addr;
   std::unique_ptr<zmq::socket_t> socket_tx, socket_rx;
};


} // namespace hydra

#endif
