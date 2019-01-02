# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)

project(dlite-example-1
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(Finddlite)
find_package(dlite)

add_custom_command(
OUTPUT chemistry.h
COMMAND ${DLITE_ROOT}/bin/dlite-codegen
  --template ${DLITE_TEMPLATE_DIR}/c-header.txt
  --uri ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  --output chemistry.h
MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
COMMENT "Generating C header for Chemistry-0.1.json"
)

link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-1 main.c chemistry.h)
target_link_libraries(dlite-example-1 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-1 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )