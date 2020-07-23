cmake_minimum_required(VERSION 3.6)

if(APPLE)
  # set(CMAKE_C_COMPILER gcc-9)
  set(CMAKE_C_COMPILER clang)
else()
  set(CMAKE_C_COMPILER gcc)
endif()

project(nwmng C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CWD)
  set(CWD "${CMAKE_SOURCE_DIR}/")
endif()

message(STATUS "************************System Status************************")
message(STATUS "Working Directory : ${CWD}")
message(STATUS "System Name : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "************************System Status************************")

find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_search_module(JSONC REQUIRED json-c)
  pkg_search_module(GLIB REQUIRED glib-2.0)
else()
  message(
    ERROR
    "\n"
    " >***************************************************************************\n"
    " > pkg-config is not available in the platform                               \n"
    " > THE NECESSARY PATHES NEED TO BE SPECIFIED EXPLICITLY                      \n"
    " > More Specifically they are:                                               \n"
    " > <GLIB_INCLUDE_DIRS>  <JSONC_INCLUDE_DIRS> <READLINE_INC_DIRS.             \n"
    " > <JSONC_LIBRARY_DIRS> <GLIB_LIBRARY_DIRS>  <READLINE_LIB_DIRS>             \n"
    " >***************************************************************************\n"
  )
endif()

# readline doesn't work with pkg-config, so specify explicitly.
if(APPLE)
  set(READLINE_INC_DIRS "/usr/local/Cellar/readline/8.0.4/include")
  set(READLINE_LIB_DIRS "/usr/local/Cellar/readline/8.0.4/lib")
endif()

if(GCOV EQUAL 1)
  set(GCOV_FLAGS "-fprofile-arcs -ftest-coverage")
else()
  set(GCOV_FLAGS "")
endif()

set(CMAKE_C_FLAGS
    "${GCOV_FLAGS} -D_DEFAULT_SOURCE -D_BSD_SOURCE -DSRC_ROOT_DIR='\"${CWD}\"' -Wall -Wno-missing-braces -std=c99 -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'"
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

# **Source files** for the project
set(HAL_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/src/host/gecko_bglib.c
    ${CMAKE_CURRENT_LIST_DIR}/hal/common/uart/uart_posix.c
    ${CMAKE_CURRENT_LIST_DIR}/hal/bg_uart_cbs.c
    ${CMAKE_CURRENT_LIST_DIR}/hal/socket_handler.c)
set(CLI_SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/cli/cli.c
                 ${CMAKE_CURRENT_LIST_DIR}/cli/cli_print.c)
set(MNG_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/mng/mng.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/models.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/demo.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/dev_add.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/dev_config.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/dev_bl.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/bgevt_hdr.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/nwk.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/stat.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_getdcd.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_addappkey.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_bindappkey.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_setpub.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_addsub.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_setconfig.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_rm.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_rmend.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_end.c)
set(CFG_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/cfg/cfg.c ${CMAKE_CURRENT_LIST_DIR}/cfg/cfgdb.c
    ${CMAKE_CURRENT_LIST_DIR}/cfg/parser/generic_parser.c
    ${CMAKE_CURRENT_LIST_DIR}/cfg/parser/json_parser.c)
set(UTILS_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/utils/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/utils_print.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/err.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/logging.c)

set(SRC_LIST
    ${CLI_SRC_LIST}
    ${MNG_SRC_LIST}
    ${UTILS_SRC_LIST}
    ${CFG_SRC_LIST}
    ${HAL_SRC_LIST}
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/startup.c)

link_directories(${JSONC_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS}
                 ${READLINE_LIB_DIRS})

if(APPLE)
  set(RL_LIB readline.dylib)
else()
  set(RL_LIB readline.so)
endif()

set(EXTRA_INC_PATH ${GLIB_INCLUDE_DIRS} ${JSONC_INCLUDE_DIRS})

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/include/utils
  ${CMAKE_CURRENT_LIST_DIR}/include/cli
  ${CMAKE_CURRENT_LIST_DIR}/include/mng
  ${CMAKE_CURRENT_LIST_DIR}/include/cfg
  ${CMAKE_CURRENT_LIST_DIR}/hal
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/inc/common
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/inc/host
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/common
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/host
  ${CMAKE_CURRENT_LIST_DIR}/hal/common/uart
  ${READLINE_INC_DIRS}
  ${EXTRA_INC_PATH})

add_executable(${CMAKE_PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${CMAKE_PROJECT_NAME} m glib-2.0 ${RL_LIB} pthread json-c)

add_custom_command(
  TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy compile_commands.json ${PROJECT_SOURCE_DIR}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT "COPYING the **compile_commands.json** file to root directory"
  VERBATIM)
