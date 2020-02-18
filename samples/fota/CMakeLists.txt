cmake_minimum_required(VERSION 3.13.1)

if(NOT DEFINED BOARD)
  set(BOARD nrf9160_pca10090ns)
endif()

#Include nrf boilerplate
get_filename_component(NRF_BASE $ENV{ZEPHYR_BASE}/../nrf ABSOLUTE)
include(${NRF_BASE}/cmake/boilerplate.cmake)

# Include Zephyr boilerplate
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(telenor_fota)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/fota.c)
zephyr_include_directories(src)
