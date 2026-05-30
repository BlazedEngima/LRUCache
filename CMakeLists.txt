cmake_minimum_required(VERSION 4.0)

project(LRUCache VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

set(WARNING_FLAGS
  -pedantic
  -Wall
  -Wextra
  -Wcast-align
  -Wcast-qual
  -Wctor-dtor-privacy
  -Wdisabled-optimization
  -Wformat=2
  -Winit-self
  -Wlogical-op
  -Wmissing-declarations
  -Wmissing-include-dirs
  -Wnoexcept
  -Wold-style-cast
  -Woverloaded-virtual
  -Wredundant-decls
  -Wshadow
  -Wsign-conversion
  -Wsign-promo
  -Wstrict-null-sentinel
  -Wstrict-overflow=5
  -Wswitch-default
  -Wundef
  -Werror
  -Wno-unused
)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_executable(LRUTests src/lru_tests.cpp)
  target_link_libraries(LRUTests LRUCache)

  add_executable(LRUExample src/lru_example.cpp)
  target_link_libraries(LRUExample LRUCache)
endif()
