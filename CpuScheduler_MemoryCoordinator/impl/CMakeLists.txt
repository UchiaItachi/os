cmake_minimum_required(VERSION 3.8)
project(impl)

set(CMAKE_CXX_STANDARD 11)
set(GCC_COVERAGE_LINK_FLAGS  "-lvirt")
set(GCC_COVERAGE_COMPILE_FLAGS  "-I/usr/include/libvirt/")
set(SOURCE_FILES main.cpp CpuScheduler.cpp CpuScheduler.h MemoryCoordinator.h MemoryCoordinator.cpp HypervisorConnection.h HypervisorConnection.cpp Domain.h Host.h VCpuInfo.h Domain.cpp Host.cpp PCpuInfo.h)
include_directories(/usr/include)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )
add_executable(impl ${SOURCE_FILES})