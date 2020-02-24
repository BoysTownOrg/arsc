cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(ARSC VERSION 0.57 LANGUAGES C CXX)

add_library(arsc_lib 
	arsc_asio/arsc_asio.c
	arsc_asio/arsc_asio_wrappers.cpp
	arsc_asio/asio.cpp
	arsc_asio/asiodrivers.cpp
	arsc_asio/asiolist.cpp
	arsc_api.c
	arsc_helper.c
	arsc_win.c
	arsc_xfer.c
	sio_arsc.c
)
target_compile_definitions(arsc_lib PRIVATE ASIO)
target_include_directories(arsc_lib PUBLIC arsc_asio .)

include(FetchContent)

FetchContent_Declare(
	check
	URL https://github.com/libcheck/check/releases/download/0.14.0/check-0.14.0.tar.gz
)
FetchContent_MakeAvailable(check)

add_executable(check-tests
	check-tests/arsc_asio_bind_tests.c
	check-tests/arsc_asio_combined_tests.c
	check-tests/arsc_asio_io_prepare_tests.c
	check-tests/arsc_asio_open_device_tests.c
	check-tests/arsc_asio_read_device_buffer_tests.c
	check-tests/arsc_asio_tests_common.c
	check-tests/arsc_asio_write_device_buffer_tests.c
	check-tests/main.c
)
target_link_libraries(check-tests arsc_lib check winmm ksuser)