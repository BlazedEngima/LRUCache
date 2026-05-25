cmake_minimum_required(VERSION 4.0)

project(DataStructs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(TEST_NAME lru_tests)
add_executable(${TEST_NAME} src/lru_tests.cpp)

target_include_directories(${TEST_NAME} PUBLIC include/)
target_compile_options(${TEST_NAME} PUBLIC "${WARNING_FLAGS} -fno-exceptions")
