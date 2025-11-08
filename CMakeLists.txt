cmake_minimum_required (VERSION 3.8)
project ("Netprobe")

# Set C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Find Boost (required for networking)
find_package(Boost REQUIRED COMPONENTS system)

# Add source to this project's executable.
add_executable (Netprobe   "src/main.cpp" "src/port_status.cpp" "src/reachability.cpp" "src/local_ip.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Netprobe PROPERTY CXX_STANDARD 20)
endif()

# Link Boost library
target_link_libraries(Netprobe PRIVATE Boost::system)
