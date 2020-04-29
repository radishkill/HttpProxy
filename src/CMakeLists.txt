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
  epollserver.cc
  conf.cc
  connreceiver.cc
  connmanager.cc
  connhandler.cc
  conninfo.cc
  request.cc
  requestparser.cc
  requesthandler.cc
  filter.cc
  response.cc
  utils.cc
  htmlerror.cc
  )

target_link_libraries(${APP_NAME}
  pthread
  boost_program_options.a
  boost_thread.a
  )

