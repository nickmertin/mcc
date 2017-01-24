cmake_minimum_required(VERSION 3.6)
project(ISU)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

set(SOURCE_FILES util/buffer.c util/buffer.h util/misc.h util/dictionary.c util/dictionary.h util/misc.c util/regex.c util/regex.h util/linked_list.c util/linked_list.h util/delegate.c util/delegate.h generator/code_graph.c generator/code_graph.h)
add_executable(mcc ${SOURCE_FILES} main.c)
add_executable(mcc-c ${SOURCE_FILES} compiler/c/preprocessor.c compiler/c/preprocessor.h)
add_executable(mcc-cre ${SOURCE_FILES} compiler/cre/parser.c compiler/cre/parser.h)
add_executable(mcc-regex-test ${SOURCE_FILES} test/regex-test.c)
