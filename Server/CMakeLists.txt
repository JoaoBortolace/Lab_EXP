cmake_minimum_required(VERSION 3.16)

set(ProjectName Server)
project(${ProjectName})

if (NOT CMAKE_BUID_TYPE)
    set(CMAKE_BUILD_TYPE  Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Obter o diretório pai do diretório atual
get_filename_component(PARENT_DIR "${CURRENT_DIR}" DIRECTORY)

# Encontre o pacote OpenCV
find_package(OpenCV REQUIRED)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found!")
endif()

include_directories(${PARENT_DIR}/lib)

add_executable(${ProjectName} main.cpp ${PARENT_DIR}/lib/Server.cpp ${PARENT_DIR}/lib/Client.cpp ${PARENT_DIR}/lib/Device.cpp)

target_link_libraries(${ProjectName} ${OpenCV_LIBS})

