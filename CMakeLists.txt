cmake_minimum_required(VERSION 3.0.0)
project(motoilet_whisper VERSION 0.1.0)
project(motoilet_whisper C)

set(CMAKE_C_STANDARD 90)

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

# data layer
add_executable(data_layer_test src/test/data_layer/data_layer_test.c src/main/data_layer/array_buffer.c src/main/data_layer/crc.c)
target_include_directories(data_layer_test PUBLIC include PRIVATE src/main/data_layer)
target_link_libraries(data_layer_test unity)
add_test(data_layer_test data_layer_test)

# data layer integration
find_package(Threads REQUIRED)
add_executable(data_layer_integration_test  src/test/data_layer/integration_test.c src/main/data_layer/array_buffer.c src/main/data_layer/crc.c)
target_include_directories(data_layer_integration_test PUBLIC include PRIVATE src/main/data_layer)
target_link_libraries(data_layer_integration_test unity pthread)
add_test(data_layer_integration_test data_layer_integration_test)

# array buffer
add_executable(array_buffer_test src/test/data_layer/array_buffer_test.c src/main/data_layer/array_buffer.c)
target_include_directories(array_buffer_test PRIVATE src/main/data_layer include)
target_link_libraries(array_buffer_test unity)
add_test(array_buffer_test array_buffer_test)
