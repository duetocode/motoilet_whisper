cmake_minimum_required(VERSION 3.0.0)
project(motoilet_whisper VERSION 0.1.0)
project(motoilet_whisper C)

set(CMAKE_C_STANDARD 99)

###################
# motoilet whisper 
###################

file(GLOB MAIN_SRC src/main/**/*.c)
add_library(motoilet_whisper STATIC ${MAIN_SRC})
target_include_directories(motoilet_whisper PUBLIC include)

############
# Unit Test
############
include(CTest)
enable_testing()

add_library(unity STATIC components/Unity/src/unity.c)
target_include_directories(unity PUBLIC components/Unity/src)

add_executable(data_layer_test src/test/data_layer/data_layer_test.c src/main/data_layer/ring_buffer.c src/main/data_layer/crc.c)
target_include_directories(data_layer_test PRIVATE src/main/data_layer)
target_link_libraries(data_layer_test unity)
add_test(data_layer_test data_layer_test)

add_executable(ring_buffer_test src/test/data_layer/ring_buffer_test.c)
target_include_directories(ring_buffer_test PRIVATE src/main/data_layer)
target_link_libraries(ring_buffer_test unity)
add_test(ring_buffer_test ring_buffer_test)
