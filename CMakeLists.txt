cmake_minimum_required(VERSION 3.6)

if(APPLE)
  set(CMAKE_C_COMPILER gcc-9)
else()
  set(CMAKE_C_COMPILER gcc)
endif()

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

# **Source files** for the project
set(HAL_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/src/host/gecko_bglib.c
    ${CMAKE_CURRENT_LIST_DIR}/hal/common/uart/uart_posix.c
    ${CMAKE_CURRENT_LIST_DIR}/hal/bg_uart_cbs.c
    ${CMAKE_CURRENT_LIST_DIR}/hal/socket_handler.c)
set(CLI_SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/cli/cli.c
                 ${CMAKE_CURRENT_LIST_DIR}/cli/climng_startup.c)
set(MNG_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/mng/mng.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/dev_add.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/dev_config.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/dev_bl.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/bgevt_hdr.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/nwk.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_getdcd.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_addappkey.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_bindappkey.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_setpub.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_addsub.c
    # ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_setconfig.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_rm.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_rmend.c
    ${CMAKE_CURRENT_LIST_DIR}/mng/states/as_end.c)
set(CFG_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/cfg/cfg.c ${CMAKE_CURRENT_LIST_DIR}/cfg/cfgdb.c
    ${CMAKE_CURRENT_LIST_DIR}/cfg/parser/generic_parser.c
    ${CMAKE_CURRENT_LIST_DIR}/cfg/parser/json_parser.c)
set(UTILS_SRC_LIST
    ${CMAKE_CURRENT_LIST_DIR}/utils/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/err.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/logging.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/sock/sockcomn.c)

set(SRC_LIST
    ${CLI_SRC_LIST}
    ${MNG_SRC_LIST}
    ${UTILS_SRC_LIST}
    ${CFG_SRC_LIST}
    ${HAL_SRC_LIST}
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/utils/startup.c)

if(APPLE)
  set(CMAKE_C_LINK_FLAGS
      "-L/usr/local/Cellar/json-c/0.13.1/lib -L/usr/local/Cellar/readline/8.0.1/lib -L/usr/local/Cellar/glib/2.62.4/lib ${CMAKE_C_LINK_FLAGS}"
  )
  set(EXTRA_INC_PATH
      /usr/local/Cellar/glib/2.62.4/include/glib-2.0
      /usr/local/Cellar/glib/2.62.4/lib/glib-2.0/include
      /usr/local/Cellar/json-c/0.13.1/include/json-c)
  set(RL_LIB readline.dylib)
else()
  set(EXTRA_INC_PATH
      /usr/include/glib-2.0 /usr/lib/x86_64-linux-gnu/glib-2.0/include
      /usr/include/json-c)
  set(RL_LIB readline.so)
endif()

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/include/utils
  ${CMAKE_CURRENT_LIST_DIR}/include/cli
  ${CMAKE_CURRENT_LIST_DIR}/include/mng
  ${CMAKE_CURRENT_LIST_DIR}/include/cfg
  # ${CMAKE_CURRENT_LIST_DIR}/include/ipc
  ${CMAKE_CURRENT_LIST_DIR}/hal
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/inc/common
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/inc/host
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/common
  ${CMAKE_CURRENT_LIST_DIR}/hal/ble_stack/host
  ${CMAKE_CURRENT_LIST_DIR}/hal/common/uart
  ${RL_INC}
  ${EXTRA_INC_PATH})

add_executable(${CMAKE_PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${CMAKE_PROJECT_NAME} m glib-2.0 ${RL_LIB} pthread json-c)

if(0)
  add_executable(
    cli-mng
    ${HAL_SRC_LIST} ${CLI_SRC_LIST} ${MNG_SRC_LIST} ${UTILS_SRC_LIST}
    ${CMAKE_CURRENT_LIST_DIR}/main.c ${CMAKE_CURRENT_LIST_DIR}/utils/startup.c)
  target_link_libraries(cli-mng m ${RL_LIB} pthread)
  target_compile_definitions(cli-mng PUBLIC CLI_MNG)
  add_executable(
    cfg ${CFG_SRC_LIST} ${UTILS_SRC_LIST} ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/utils/startup.c)
  target_link_libraries(cfg m json-c glib-2.0)
  target_compile_definitions(cfg PUBLIC CFG)
endif()

add_custom_command(
  TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy compile_commands.json ${PROJECT_SOURCE_DIR}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT "COPYING the **compile_commands.json** file to root directory"
  VERBATIM)
