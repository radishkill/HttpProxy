cmake_minimum_required(VERSION 3.5)

set(APP_NAME HttpProxy)

project(${APP_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_BUILD_TYPE "Debug")

include_directories(
  ${PROJECT_SOURCE_DIR}/
  ${PROJECT_SOURCE_DIR}/utils
  )

add_executable(${APP_NAME} main.cc
  epollrepertory.cc
  epollrepertory.h
  epollserver.cc
  epollserver.h
  conf.cc
  conf.h
  proxyserver.cc
  proxyserver.h
  proxyconnection.cc
  proxyconnection.h
  httpparser.cc
  httpparser.h
  requesthandler.cc
  requesthandler.h
  filter.cc
  filter.h
  response.cc
  response.h
  utils.cc
  utils.h
  htmlerror.cc
  common.h
  )

target_link_libraries(${APP_NAME}
  pthread
  boost_program_options.a
  boost_thread.a
  )

