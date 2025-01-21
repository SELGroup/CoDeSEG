cmake_minimum_required(VERSION 3.29.3)
project(CoDeSEG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

include_directories(${PROJECT_SOURCE_DIR}/lib)



add_executable(CoDeSEG ${PROJECT_SOURCE_DIR}/main.cpp
${PROJECT_SOURCE_DIR}/lib/CoDeSEG.cpp
${PROJECT_SOURCE_DIR}/lib/CoDeSEG.h
${PROJECT_SOURCE_DIR}/lib/DynamicArray.cpp
${PROJECT_SOURCE_DIR}/lib/DynamicArray.h
${PROJECT_SOURCE_DIR}/lib/CxxOpts.h
${PROJECT_SOURCE_DIR}/lib/Utility.h
${PROJECT_SOURCE_DIR}/lib/Utility.cpp
${PROJECT_SOURCE_DIR}/lib/ThreadPool.h)

find_package(Threads REQUIRED)
target_link_libraries(CoDeSEG PRIVATE Threads::Threads)