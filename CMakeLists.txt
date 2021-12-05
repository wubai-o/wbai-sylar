cmake_minimum_required(VERSION 2.8)

project(wubai)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror  -Wno-unused-function")

set(LIB_SRC
    wubai/log.cc
    wubai/utils.cc
    wubai/config.cc 
    wubai/thread.cc
    wubai/fiber.cc
    wubai/scheduler.cc
    wubai/iomanager.cc
    wubai/timer.cc
    wubai/hook.cc
    wubai/fd_manager.cc
    wubai/address.cc
    wubai/socket.cc
    wubai/bytearray.cc
    wubai/http/http.cc
    wubai/http/http11_parser.rl.cc
    wubai/http/httpclient_parser.rl.cc
    wubai/http/http_parser.cc
    wubai/tcp_server.cc
    wubai/stream.cc
    wubai/socket_stream.cc
    wubai/http/http_session.cc
    wubai/http/http_server.cc
    wubai/http/servlet.cc
    wubai/http/http_connection.cc
    wubai/uri.cc
    wubai/keshe/local_server.cc
    wubai/keshe/mymysql.cc
    wubai/daemon.cc
    wubai/env.cc
    wubai/application.cc
    wubai/hashutils.cc
    wubai/http/ws_session.cc
    wubai/http/ws_servlet.cc
    wubai/http/ws_server.cc
    wubai/http/ws_connection.cc
    )

add_library(wubai SHARED ${LIB_SRC})
#add_library(wubai_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (wubai_static PROPERTIES OUTPUT_NAME "wubai"

set(LIB_LIB
    wubai
    pthread
    dl
    yaml-cpp
    mysqlclient
    ssl
    )

add_executable(test tests/test.cc)
add_dependencies(test wubai)
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config wubai)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread wubai)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util wubai)
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber wubai)
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler wubai)
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager wubai)
target_link_libraries(test_iomanager ${LIB_LIB})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook wubai)
target_link_libraries(test_hook ${LIB_LIB})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address wubai)
target_link_libraries(test_address ${LIB_LIB})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket wubai)
target_link_libraries(test_socket ${LIB_LIB})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray wubai)
target_link_libraries(test_bytearray ${LIB_LIB})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http wubai)
target_link_libraries(test_http ${LIB_LIB})

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser wubai)
target_link_libraries(test_http_parser ${LIB_LIB})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server wubai)
target_link_libraries(test_tcp_server ${LIB_LIB})

add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server wubai)
target_link_libraries(echo_server ${LIB_LIB})

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server wubai)
target_link_libraries(test_http_server ${LIB_LIB})

add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection wubai)
target_link_libraries(test_http_connection ${LIB_LIB})

add_executable(test_uri tests/test_uri.cc)
add_dependencies(test_uri wubai)
target_link_libraries(test_uri ${LIB_LIB})

add_executable(test_my_fiber tests/test_my_fiber.cc)
add_dependencies(test_my_fiber wubai)
target_link_libraries(test_my_fiber ${LIB_LIB})

add_executable(App keshe/App.cc)
add_dependencies(App wubai)
target_link_libraries(App ${LIB_LIB})

add_executable(my_http_server samples/my_http_server.cc)
add_dependencies(my_http_server wubai)
target_link_libraries(my_http_server ${LIB_LIB})

add_executable(test_daemon tests/test_daemon.cc)
add_dependencies(test_daemon wubai)
target_link_libraries(test_daemon ${LIB_LIB})

add_executable(test_env tests/test_env.cc)
add_dependencies(test_env wubai)
target_link_libraries(test_env ${LIB_LIB})

add_executable(test_application tests/test_application.cc)
add_dependencies(test_application wubai)
target_link_libraries(test_application ${LIB_LIB})

add_executable(test_hashutils tests/test_hashutils.cc)
add_dependencies(test_hashutils wubai)
target_link_libraries(test_hashutils ${LIB_LIB})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
