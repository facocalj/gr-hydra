# Include libraries and their implementation
include_directories(${HYDRA_SOURCE_DIR}/hydra/include)
link_directories(${HYDRA_SOURCE_DIR}/lib)

# Add the Server executable
add_executable(server server.cc)
target_link_libraries(server hydra)

# Add the Client executable
add_executable(client client.cc)
target_link_libraries(client hydra)

add_executable(tx_trace tx_trace.cc)
target_link_libraries(tx_trace hydra)

add_executable(tx_iqs tx_iqs.cc)
target_link_libraries(tx_iqs hydra)
