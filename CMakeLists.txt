cmake_minimum_required(VERSION 3.6)
project(ISU)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

set(SOURCE_FILES main.c parser/load.c parser/load.h preprocessor/controller.c preprocessor/controller.h util/buffer.c util/buffer.h)
add_executable(mcc ${SOURCE_FILES})