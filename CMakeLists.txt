cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(reshade-hdr-metadata
        VERSION 1.0.0
        LANGUAGES CXX)

add_library(reshade-hdr-metadata MODULE ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp)

target_include_directories(reshade-hdr-metadata SYSTEM PUBLIC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared")
target_include_directories(reshade-hdr-metadata SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/reshade")
target_include_directories(reshade-hdr-metadata SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/reshade/deps/vulkan/include")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set_target_properties(reshade-hdr-metadata PROPERTIES SUFFIX ".addon64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set_target_properties(reshade-hdr-metadata PROPERTIES SUFFIX ".addon32")
else()
  message(FATAL_ERROR "architecture could not be detected!")
endif()
