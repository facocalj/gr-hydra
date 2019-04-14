#include "hydra/hydra_core.h"



namespace hydra {

// Real radio centre frequency, bandwidth; and the hypervisor's sampling rate, FFT size
HydraCore::HydraCore()
{
   // Initialise the resource manager
   p_resource_manager = std::make_unique<xvl_resource_manager>();
   // Initialize the hypervisor
   p_hypervisor = std::make_unique<Hypervisor>();
}

void
HydraCore::set_rx_resources(uhd_hydra_sptr usrp,
                            double d_centre_freq,
                            double d_bandwidth,
                            unsigned int u_fft_size)
{
  // Initialise the RX resources
  p_resource_manager->set_rx_resources(d_centre_freq, d_bandwidth);

  usrp->set_rx_config(d_centre_freq, d_bandwidth, 0);
  p_hypervisor->set_rx_resources(usrp, d_centre_freq, d_bandwidth, u_fft_size);

  // Toggle flag
  b_receiver = true;
}

void
HydraCore::set_tx_resources(uhd_hydra_sptr usrp,
                            double d_centre_freq,
                            double d_bandwidth,
                            unsigned int u_fft_size)
{
   // Initialise the RX resources
   p_resource_manager->set_tx_resources(d_centre_freq, d_bandwidth);

   usrp->set_tx_config(d_centre_freq, d_bandwidth, 0.6);
   p_hypervisor->set_tx_resources(usrp, d_centre_freq, d_bandwidth, u_fft_size);

   // Toggle flag
   b_transmitter = true;
}

int
HydraCore::request_rx_resources(unsigned int u_id,
                                double d_centre_freq,
                                double d_bandwidth,
                                const std::string &server_addr,
                                const std::string &remote_addr)
{
  // Lock for this operation
  std::lock_guard<std::mutex> lock(virtual_radio_mutex);

  // If not configured to receive
  if (not b_receiver)
  {
    // Return error -- zero is bad
    std::cout << "<core> RX Resources not configured." << std::endl;
    return 0;
  }

  // Try to get this virtual readio
  VirtualRadio* virtual_radio = get_virtual_radio(u_id);

  // If this is a new Virtual Radio
  if (virtual_radio == nullptr)
  {
    // Try to reserve the resource chunks
    if(not p_resource_manager->reserve_rx_resources(u_id, d_centre_freq, d_bandwidth))
    {
      // Create RX UDP port
      static size_t u_udp_port = 33000;

      // Creare a new virtual radio
      virtual_radio = new VirtualRadio(u_id, p_hypervisor);
      // Create the RX chain
      virtual_radio->set_rx_chain(u_udp_port, d_centre_freq, d_bandwidth, server_addr, remote_addr);
      // Add it to the list
      g_virtual_radios.push_back(virtual_radio);

       // If able to create all of it, return the port number
      std::cout << "<core> RX Resources allocated successfully." << std::endl;
      return u_udp_port++;
    }
  } // New VR
  // There is already a virtual radio
  else if(vr->get_rx_enabled())
  {
    // Request RX resources from an existing VR
    if (p_resource_manager->check_rx_free(d_centre_freq, d_bandwidth, u_id))
    {
      // Free the RX resources
      p_resource_manager->free_rx_resources(u_id);
      // Allocate new RX resources
      p_resource_manager->reserve_rx_resources(u_id, d_centre_freq, d_bandwidth);

      // Set the RX centre frequency and bandwidth
      virtual_radio->set_rx_freq(d_centre_freq);
      virtual_radio->set_rx_bandwidth(d_bandwidth);

      // If able to create all of it, return the port number
      std::cout << "<core> RX Resources allocated successfully." << std::endl;
      return virtual_radio->g_rx_udp_port;
    }
  } // Existing VR

  // Otherwise, the allocation failed
  return 0;
}

int
HydraCore::request_tx_resources(unsigned int u_id,
                                double d_centre_freq,
                                double d_bandwidth,
                                const std::string &server_addr,
                                const std::string &remote_addr)
{
  // Lock for this operation
  std::lock_guard<std::mutex> lock(virtual_radio_mutex);

  // If not configured to transmit
  if (not b_transmit)
  {
    // Return error -- zero is bad
    std::cout << "<core> TX Resources not configured." << std::endl;
    return 0;
  }

  // Try to get this virtual readio
  VirtualRadio* virtual_radio = get_virtual_radio(u_id);

  // If this is a new Virtual Radio
  if (virtual_radio == nullptr)
  {
    // Try to reserve the resource chunks
    if(not p_resource_manager->reserve_tx_resources(u_id, d_centre_freq, d_bandwidth))
    {
      // Create TX UDP port
      static size_t u_udp_port = 33500;

      // Creare a new virtual radio
      virtual_radio = new VirtualRadio(u_id, p_hypervisor);
      // Create the RX chain
      virtual_radio->set_tx_chain(u_udp_port, d_centre_freq, d_bandwidth, server_addr, remote_addr);
      // Add it to the list
      g_virtual_radios.push_back(virtual_radio);

       // If able to create all of it, return the port number
      std::cout << "<core> TX Resources allocated successfully." << std::endl;
      return u_udp_port++;
    }
  } // New VR
  // There is already a virtual radio
  else if(virtual_radio->get_tx_enabled())
  {
    // Request TX resources from an existing VR
    if (p_resource_manager->check_tx_free(d_centre_freq, d_bandwidth, u_id))
    {
      // Free the TX resources
      p_resource_manager->free_tx_resources(u_id);
      // Allocate new TX resources
      p_resource_manager->reserve_tx_resources(u_id, d_centre_freq, d_bandwidth);

      // Set the TX centre frequency and bandwidth
      virtual_radio->set_tx_freq(d_centre_freq);
      virtual_radio->set_tx_bandwidth(d_bandwidth);

      // Return sucessfull reallocation
      return virtual_radio->g_tx_udp_port;
    }
  } // Existing VR

  // Otherwise, the allocation failed
  return 0;
}

// Query the virtual radios (and add their UDP port)
boost::property_tree::ptree
HydraCore::query_resources()
{  // Get the query from the RM
  boost::property_tree::ptree query_message = \
    p_resource_manager->query_resources();

  if (b_receiver)
  {
    // Get the RX child
    auto rx_chunks = query_message.get_child("receiver");
    // TODO There must be an easier way to do this
    query_message.erase("receiver");

    // Iterate over the child
    for (auto p_chunk = rx_chunks.begin();
         p_chunk != rx_chunks.end(); p_chunk++)
    {
      // If the current entry is a valid type
      if (p_chunk->second.get<int>("id"))
      {
         VirtualRadioPtr vr = p_hypervisor->get_vradio(p_chunk->second.get<int>("id"));

        // If receiving
         if ((vr != nullptr) and vr->get_rx_enabled())
        {
          // Add this new entry
           p_chunk->second.put("rx_udp_port",vr->get_rx_udp_port());
        }
      }
    }
    // Update the original tree
    query_message.add_child("receiver", rx_chunks);
  }

  if (b_transmitter)
  {
    // Get the TX child
    auto tx_chunks = query_message.get_child("transmitter");
    // TODO There must be an easier way to do this
    query_message.erase("transmitter");

    // Iterate over the child
    for (auto p_chunk = tx_chunks.begin();
         p_chunk != tx_chunks.end(); p_chunk++)
    {
      // If the current entry is a valid type
      if (p_chunk->second.get<int>("id"))
      {
        auto vr = p_hypervisor->get_vradio(p_chunk->second.get<int>("id"));

        // If receiving
        if ((vr != nullptr) and vr->get_tx_enabled())
        {
          // Add this new entry
           p_chunk->second.put("tx_udp_port",vr->get_tx_udp_port());
        }
      }
    }
    // Update the original tree
    query_message.add_child("transmitter", tx_chunks);
  }

  // Return updated
  return query_message;
}

// Deletes a given virtual radio
int
HydraCore::free_resources(size_t radio_id)
{
  std::cout << "CORE: freeing resources for radio: " << radio_id << std::endl;
  p_resource_manager->free_rx_resources(radio_id);
  p_resource_manager->free_tx_resources(radio_id);
  p_hypervisor->detach_virtual_radio(radio_id);

  std::cout << "CORE: DONE freeing resources for radio: " << radio_id << std::endl;
  return 1;
}

VirtualRadioPtr
HydraCore::get_vradio(size_t id)
{
  // Lock asses to the virtual radio vector
  std::lock_guard<std::mutex> core_lock(core_mutex);

  // Assgign it to NULL by default
  VirtualRadioPtr vr = nullptr;

  // Iterate over the virtual radio vector
  for (auto & it : g_virtual_radios)
  {
    // If the ID matches
    if(it->get_id() == id)
    {
      // Overwrite it
      vr = it;
      // Exit the loop
      break;
    }
  }
  // Return the VR, or NULL
  return vr;
}

bool
Hypervisor::detach_virtual_radio(size_t radio_id)
{
  // Lock asses to the virtual radio vector
  std::lock_guard<std::mutex> core_lock(core_mutex);

  auto new_end = std::remove_if(g_vradios.begin(), g_vradios.end(),
                                [radio_id](const auto & vr) {
                                  return vr->get_id() == radio_id; });

  g_virtual_radios.erase(new_end, g_vradios.end());
  return true;
}





} // namespace hydra
