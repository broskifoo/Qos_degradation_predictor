cmake_minimum_required(VERSION 3.14)
project(QoS_Simulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Compile without warnings as requested
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Include directories
include_directories(src)

# Fetch Raylib
include(FetchContent)

# Set newer CMake policies to avoid errors with older raylib
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# Source files
set(SOURCES
    src/main.cpp
    src/utils/Config.cpp
    src/utils/CSVReader.cpp
    src/network/Packet.cpp
    src/network/Queue.cpp
    src/network/Link.cpp
    src/simulator/TCPFlow.cpp
    src/simulator/RealTimeFlow.cpp
    src/simulator/Simulator.cpp
    src/qos/Metrics.cpp
    src/qos/Logger.cpp
    src/ml/LogisticRegression.cpp
    src/visualization/Dashboard.cpp
)

# Executable
add_executable(QoS_Simulator ${SOURCES})

# Link libraries
target_link_libraries(QoS_Simulator PRIVATE raylib)
