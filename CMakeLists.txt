cmake_minimum_required( VERSION 2.8.4 )
include( ExternalProject )
project( nodepp_rfb )

set( Boost_USE_STATIC_LIBS OFF )
set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_RUNTIME OFF )
find_package( Boost 1.59.0 REQUIRED COMPONENTS system filesystem regex unit_test_framework program_options iostreams )

find_package(OpenSSL REQUIRED)

IF( ${CMAKE_CXX_COMPILER_ID} STREQUAL 'MSVC' )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601" )
ELSEIF( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" )
	find_package( Threads REQUIRED )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++ -g -Weverything -Wno-c++98-compat -Wfatal-errors -Wno-covered-switch-default -Wno-padded -Wno-exit-time-destructors -Wno-c++98-compat-pedantic -Wno-unused-parameter -Wno-missing-noreturn -Wno-missing-prototypes -Wno-disabled-macro-expansion" )
ELSEIF( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" )
	find_package( Threads REQUIRED )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall -Wno-deprecated-declarations" )
ENDIF()

if( THREADS_HAVE_PTHREAD_ARG )
	set( COMPILER_SPECIFIC_LIBS, "-lpthread" )
endif()

enable_testing()

ExternalProject_Add(
		lib_nodepp_prj
		GIT_REPOSITORY "https://github.com/beached/lib_nodepp.git"
		GIT_TAG "master"
		PATCH_COMMAND ""
		INSTALL_COMMAND ""
		SOURCE_DIR "${CMAKE_BINARY_DIR}/3rdparty/lib_nodepp"
		TEST_COMMAND ""
)

ExternalProject_Add(
		header_libraries_prj
		GIT_REPOSITORY "https://github.com/beached/header_libraries.git"
		GIT_TAG "master"
		PATCH_COMMAND ""
		INSTALL_COMMAND ""
		SOURCE_DIR "${CMAKE_BINARY_DIR}/3rdparty/header_libraries"
		TEST_COMMAND ""
)

set( header_libraries_include "${CMAKE_BINARY_DIR}/3rdparty/header_libraries" )
set_property( GLOBAL PROPERTY header_libraries_include ${header_libraries_include} )
include_directories( SYSTEM ${header_libraries_include} )

set( SOURCE_FILES
		nodepp_rfb.cpp
		nodepp_rfb.h
		nodepp_rfb_test.cpp
		)

ExternalProject_Get_Property( lib_nodepp_prj source_dir )

include_directories( "${source_dir}/nodepp" )
include_directories( "${source_dir}/char_range" )

include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )

include_directories( SYSTEM ${OPENSSL_INCLUDE_DIR} )


link_directories( ${Boost_LIBRARY_DIRS} )

ExternalProject_Get_Property( lib_nodepp_prj binary_dir )
link_directories( "${binary_dir}/nodepp" )
link_directories( "${binary_dir}/char_range" )


add_executable( nodepp_rfb ${SOURCE_FILES} )
add_dependencies( nodepp_rfb lib_nodepp_prj )

target_link_libraries( nodepp_rfb nodepp char_range ${CMAKE_DL_LIBS} ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
