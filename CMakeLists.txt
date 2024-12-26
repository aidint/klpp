cmake_minimum_required(VERSION 3.20)
project(k++
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wpedantic)


find_package(LLVM REQUIRED CONFIG)
include_directories(include)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS lib/*.cpp)


# Create repl
add_executable(kpp src/repl.cpp ${sources})
llvm_config(kpp USE_SHARED orcjit native core)

# build compiler
add_executable(kppc src/compiler.cpp ${sources})
target_compile_definitions(kppc PUBLIC COMPILATION)
llvm_config(kppc USE_SHARED all)

# bring files
file(GLOB_RECURSE libfiles CONFIGURE_DEPENDS lib/std/*kl)

foreach(file_i ${libfiles})
  get_filename_component(file ${file_i} NAME)
  configure_file(${file_i} ${CMAKE_BINARY_DIR}/lib/${file} COPYONLY)
endforeach(file_i)

configure_file(scripts/kl++.in ${CMAKE_BINARY_DIR}/kl++ 
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE @ONLY)
configure_file(scripts/post_build.sh ${CMAKE_BINARY_DIR}/post_build.sh 
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
  GROUP_READ GROUP_EXECUTE 
  WORLD_READ WORLD_EXECUTE)

# building standard library
add_library(external OBJECT lib/external.cpp)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/lib/core.o ${CMAKE_BINARY_DIR}/lib/builtin.o
  COMMAND ${CMAKE_COMMAND} -E env CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} ${CMAKE_BINARY_DIR}/post_build.sh
  DEPENDS external kppc
  )
add_custom_target(post_build ALL DEPENDS ${CMAKE_BINARY_DIR}/lib/core.o ${CMAKE_BINARY_DIR}/lib/builtin.o)

add_library(kalpp STATIC ${CMAKE_BINARY_DIR}/lib/core.o ${CMAKE_BINARY_DIR}/lib/builtin.o $<TARGET_OBJECTS:external>)




  
