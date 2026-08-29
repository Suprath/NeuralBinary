cmake_minimum_required(VERSION 3.15)
project(ZCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Homebrew Z3 path on Apple Silicon macOS
include_directories(/opt/homebrew/include include)
link_directories(/opt/homebrew/lib)

add_executable(z_core src/main.cpp)
target_link_libraries(z_core PRIVATE z3)
