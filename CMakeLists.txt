cmake_minimum_required(VERSION 3.6)
project(ISU)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

set(SOURCE_FILES util/buffer.c util/buffer.h util/misc.h util/dictionary.c util/dictionary.h util/misc.c util/regex.c util/regex.h util/linked_list.c util/linked_list.h util/delegate.c util/delegate.h generator/code_graph.c generator/code_graph.h util/io.c util/io.h)

set(SOURCE_LAUNCHER launchers/launchers.h main.c)

set(SOURCE_LINUX_X86 generator/linux-x86/generator.c generator/linux-x86/generator.h)

set(SOURCE_CRE compiler/cre/parser.c compiler/cre/parser.h compiler/cre/grapher.c compiler/cre/grapher.h launchers/launchers.c)

add_executable(mcc-cre-linux-x86 ${SOURCE_FILES} ${SOURCE_LAUNCHER} ${SOURCE_LINUX_X86} ${SOURCE_CRE} launchers/cre-linux-x86.c)
add_executable(mcc-regex-test ${SOURCE_FILES} test/regex-test.c)
add_executable(mcc-linux-x86-test ${SOURCE_FILES} ${SOURCE_LINUX_X86} test/linux-x86-test.c)
