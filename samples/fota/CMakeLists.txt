cmake_minimum_required(VERSION 3.13.1)

if(NOT DEFINED BOARD)
  set(BOARD nrf9160_pca10090ns)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include(../../CMakeLists.txt)

project(app)
zephyr_include_directories(src)
zephyr_library_sources(src/main.c)
