cmake_minimum_required(VERSION 3.13)

project(waveshare-epaper C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
	src/Debug.h
        src/dev_spi.h
        src/dev_spi.cpp
	src/EPD_2in13b_V3.h
        src/EPD_2in13b_V3.cpp
        src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
        gpiod
        freeimage
)
