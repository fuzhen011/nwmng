cmake_minimum_required(VERSION 3.6)

set(CMAKE_C_COMPILER gcc-9)
project(nwmng C)

if(NOT CWD)
  set(CWD "${CMAKE_SOURCE_DIR}/")
endif()

# TODO: Use find_package
set(RL_INC "/usr/local/Cellar/readline/8.0.1/include")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "************************System Status************************")
message(STATUS "Working Directory : ${CWD}")
message(STATUS "SYSTEM_NAME : ${CMAKE_SYSTEM_NAME}")
message(STATUS "************************System Status************************")

if(GCOV EQUAL 1)
  set(GCOV_FLAGS "-fprofile-arcs -ftest-coverage")
else()
  set(GCOV_FLAGS "")
endif()

set(CMAKE_C_FLAGS
    "${GCOV_FLAGS} -D_DEFAULT_SOURCE -D_BSD_SOURCE -DSRC_ROOT_DIR='\"${CWD}\"' -Wall -std=c99 -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'"
)
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "-Os -DDEBUG=0")

add_definitions(-D_DEFAULT_SOURCE -D_BSD_SOURCE)

if(OS AND (OS MATCHES "Windows*" OR WIN32))
  set(SRC_LIST ${SRC_LIST} ${CMAKE_SOURCE_DIR}/common/uart/uart_win.c)
else()
  set(SRC_LIST ${SRC_LIST} ${CMAKE_SOURCE_DIR}/common/uart/uart_posix.c)
endif()

# if(JSON_SUP EQUAL 1) add_definitions(-DJSON_SUP=1) set(SRC_LIST ${SRC_LIST}
# ${CMAKE_SOURCE_DIR}/src/common/json_parser.c) endif()

set(CLI_SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/cli/cli.c)

set(MNG_SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/mng/mng.c)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/utils UTILS_SRC_LIST)

set(SRC_LIST ${CLI_SRC_LIST} ${MNG_SRC_LIST} ${UTILS_SRC_LIST}
             ${CMAKE_CURRENT_LIST_DIR}/main.c)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/include/utils
  ${CMAKE_CURRENT_LIST_DIR}/include/cli
  ${CMAKE_CURRENT_LIST_DIR}/include/mng
  ${CMAKE_CURRENT_LIST_DIR}/include/cfg
  ${RL_INC})

add_executable(${CMAKE_PROJECT_NAME} ${SRC_LIST})

if(WIN32)
  target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    m
    pthread
    ${CMAKE_SOURCE_DIR}/lib/libxml2.dll
    ${CMAKE_SOURCE_DIR}/lib/json-c.dll
    ${CMAKE_SOURCE_DIR}/lib/WSock32.lib
    ${CMAKE_SOURCE_DIR}/lib/WS2_32.lib)
endif()

if(APPLE)
  # set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS}
  # -L/usr/local/Cellar/json-c/0.13.1/lib")
endif()

if(UNIX)
  if(APPLE)
    target_link_libraries(
      ${CMAKE_PROJECT_NAME} m readline
      # pthread json-c
    )
  else()
    target_link_libraries(${CMAKE_PROJECT_NAME} m rt pthread
                          ${CMAKE_SOURCE_DIR}/lib/libjson-c.so)
  endif()
endif()

add_custom_command(
  TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy compile_commands.json ${PROJECT_SOURCE_DIR}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT "COPYING the **compile_commands.json** file to root directory"
  VERBATIM)
