#include "hydra/hydra_main.h"

int main()
{
  // Receiver
  // Centre frequency in Hz
  double d_rx_centre_freq = 2e9;
  // Sampling rate in mega samples per second
  double d_rx_samp_rate = 20e6;
  // FFT size
  unsigned int u_rx_fft_size = 2000;

  // Transmitter
  // Centre frequency in Hz
  double d_tx_centre_freq = 2e9;
  // Sampling rate in mega samples per second
  double d_tx_samp_rate = 20e6;
  // FFT size
  unsigned int u_tx_fft_size = 2000;

  // Control port
  unsigned int u_port = 5000;

  // Instantiate XVL
  HydraMain main = HydraMain(u_port);

  // Configure the RX radio
  /*
  main.set_rx_config(d_rx_centre_freq,
                     d_rx_samp_rate,
                     u_rx_fft_size);
  */

  // Configure the TX radio
  main.set_tx_config(d_tx_centre_freq,
                     d_tx_samp_rate,
                     u_tx_fft_size);

  // Run server
  main.run();

  // Run server
  return 0;
}