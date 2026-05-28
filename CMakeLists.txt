cmake_minimum_required(VERSION 3.10)
project(wifi_mac_teaching C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(${PROJECT_SOURCE_DIR}/include)

# --- Core MAC library: the four coordination functions --------------------
add_library(wifimac STATIC
  src/dcf.c
  src/dcf_sched.c
  src/pcf.c
  src/edca.c
  src/edca_sched.c
  src/hcca.c
  src/rts_cts.c
  src/aggregation.c
  src/block_ack.c
  src/seq_num.c
  src/rate_control.c
  src/tx_constraints.c
)
target_include_directories(wifimac PUBLIC ${PROJECT_SOURCE_DIR}/include)

# --- Demo executable: verbose side-by-side traces -------------------------
add_executable(demo src/demo.c)
target_link_libraries(demo wifimac)

# --- Tests ----------------------------------------------------------------
enable_testing()

function(add_mac_test name src)
  add_executable(${name} tests/${src})
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/tests)
  target_link_libraries(${name} wifimac)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_mac_test(test_dcf  test_dcf.c)
add_mac_test(test_pcf  test_pcf.c)
add_mac_test(test_edca test_edca.c)
add_mac_test(test_hcca test_hcca.c)
add_mac_test(test_edca_triggers test_edca_triggers.c)
add_mac_test(test_rts_cts        test_rts_cts.c)
add_mac_test(test_aggregation    test_aggregation.c)
add_mac_test(test_block_ack      test_block_ack.c)
add_mac_test(test_seq_num        test_seq_num.c)
add_mac_test(test_rate_control   test_rate_control.c)
add_mac_test(test_tx_constraints test_tx_constraints.c)
