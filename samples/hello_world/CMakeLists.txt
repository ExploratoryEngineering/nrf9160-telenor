#
# Copyright (c) 2019 Exploratory Engineering
#

cmake_minimum_required(VERSION 3.13.1)

if(NOT DEFINED BOARD)
  set(BOARD nrf9160_pca10090ns)
endif()

#Include nrf boilerplate
get_filename_component(NRF_BASE $ENV{ZEPHYR_BASE}/../nrf ABSOLUTE)
include(${NRF_BASE}/cmake/boilerplate.cmake)

# Include Zephyr boilerplate
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(telenor_hello_world)

zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})
target_sources(app PRIVATE src/main.c)
zephyr_include_directories(src)
