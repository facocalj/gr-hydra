#include "hydra/hydra_client.h"

#include <iostream>
#include <signal.h>
#include <boost/format.hpp>



using namespace std;

int main()
{
  // Hypervisor's CF
  double cf = 2e9;

  // VR 1 specs
  double vr_1_cf = cf + 200e3;
  double vr_1_bw = 200e3;

  // Request resources
  std::cout << "------------- Requesting" << std::endl;
  std::cout << "CF: "  << vr_1_cf << "\tBW: " <<  vr_1_bw << std::endl;

  hydra::hydra_client s1 = hydra::hydra_client("127.0.0.1", 5000, 91, true);

  std::cout<< s1.check_connection() << std::endl;
  std::cout<< s1.query_resources() << std::endl;

  hydra::rx_configuration tx1{vr_1_cf, vr_1_bw, false};
  std::cout << s1.request_tx_resources(tx1) << std::endl;

#if 0
  // VR 1 specs
  double vr_2_cf = cf - 200e3;
  double vr_2_bw = 200e3;

  // Request resources
  std::cout << "------------- Requesting" << std::endl;
  std::cout << "CF: "  << vr_2_cf << "\tBW: " <<  vr_2_bw << std::endl;

  hydra::hydra_client s2 = hydra::hydra_client("127.0.0.1", 5000, 92, true);

  std::cout<< s2.check_connection() << std::endl;
  std::cout<< s2.query_resources() << std::endl;

  hydra::rx_configuration tx2{vr_2_cf, vr_2_bw, false};
  std::cout << s2.request_tx_resources(tx2) << std::endl;
#endif

  // sleep(1);
  // Free resources from a given service
  // std::cout << s1.free_resources() << std::endl;
  // Query available resources
  // std::cout << s1.query_resources() << std::endl;

#if 0
  sleep(1);
  // Free resources from a given service
  std::cout << s2.free_resources() << std::endl;
  // Query available resources
  std::cout << s2.query_resources() << std::endl;
#endif


  std::cout << "Press CTRL-C to quit" << std::endl;
  while (1) usleep(1000);

  return 0;
}
