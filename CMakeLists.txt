cmake_minimum_required(VERSION 3.12)
project(itch_feed_handler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable aggressive optimizations for production/low-latency performance
if(MSVC)
    add_compile_options(/W4 /WX /O2 /Oi /Ot /Gy)
else()
    add_compile_options(-Wall -Wextra -Werror -O3 -march=native)
endif()

add_executable(itch_feed_handler main.cpp itch_utils.cpp)

