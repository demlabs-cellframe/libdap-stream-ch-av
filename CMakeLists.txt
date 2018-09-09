cmake_minimum_required(VERSION 3.0)
project (dap_stream_ch_av)
  
set(DAP_STREAM_CH_AV_SRCS dap_stream_ch_av.c)

add_library(${PROJECT_NAME} STATIC ${DAP_STREAM_CH_AV_SRCS})

target_link_libraries(dap_stream_ch_av dap_core dap_crypto dap_core_server dap_stream dap_stream_ch)

target_include_directories(dap_stream_ch_av INTERFACE .)
