cmake_minimum_required(VERSION 3.16)

set(ProjectName Client)
project(${ProjectName})

# Diretório atual
set(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# Obter o diretório pai do diretório atual
get_filename_component(PARENT_DIR "${CURRENT_DIR}" DIRECTORY)

# Diretório dos arquivos auxiliares
set(LIB_DIR ${PARENT_DIR}/lib)

# Configurações de compilação
if (NOT CMAKE_BUID_TYPE)
    set(CMAKE_BUILD_TYPE  Debug)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
CHECK_CXX_COMPILER_FLAG("-mtune=native" COMPILER_SUPPORTS_MTUNE_NATIVE)

if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  -march=native")
endif()

if(COMPILER_SUPPORTS_MTUNE_NATIVE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mtune=native")
endif()

# Encontre o pacote OpenCV
find_package(OpenCV REQUIRED)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found!")
endif()

# Adiciona a pasta dos arquivos auxiliares
include_directories(${LIB_DIR})
file(GLOB programa "${LIB_DIR}/*.cpp")

add_executable(${ProjectName} main.cpp ${programa})

target_link_libraries(${ProjectName} ${OpenCV_LIBS})