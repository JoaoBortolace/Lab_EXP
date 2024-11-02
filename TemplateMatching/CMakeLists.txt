cmake_minimum_required(VERSION 3.16)

set(ProjectName TemplateMatching)
project(${ProjectName} LANGUAGES CXX)

# Diretório atual
set(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# Obter o diretório pai do diretório atual
get_filename_component(PARENT_DIR "${CURRENT_DIR}" DIRECTORY)

# Diretório dos arquivos auxiliares
set(LIB_DIR ${PARENT_DIR}/lib)

# Configurações de compilação
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")

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

find_package(OpenMP REQUIRED COMPONENTS CXX)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found!")
endif()

if(NOT OpenMP_FOUND)
    message(FATAL_ERROR "OpenMP not found!")
endif()

# Adiciona a pasta dos arquivos auxiliares
include_directories(${LIB_DIR})
file(GLOB programa "${LIB_DIR}/*.cpp")

# Define o executável e as bibliotecas
add_executable(${ProjectName} main.cpp ${programa})

# Adiciona as bibliotecas necessárias ao projeto
target_link_libraries(${ProjectName} PUBLIC OpenMP::OpenMP_CXX ${OpenCV_LIBS})

# Define flags para o OpenMP
target_compile_options(${ProjectName} PUBLIC ${OpenMP_CXX_FLAGS})
target_link_options(${ProjectName} PUBLIC ${OpenMP_CXX_FLAGS})