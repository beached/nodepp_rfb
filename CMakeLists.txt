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

externalproject_add( 
	header_libraries_prj
	GIT_REPOSITORY "https://github.com/beached/header_libraries.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/header_libraries"
	GIT_TAG "master"
	PATCH_COMMAND ""
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	TEST_COMMAND ""
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	char_range_prj
	GIT_REPOSITORY "https://github.com/beached/char_range.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/char_range"
	GIT_TAG "master"
	PATCH_COMMAND ""
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	TEST_COMMAND ""
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	parse_json_prj
	GIT_REPOSITORY "https://github.com/beached/parse_json.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/parse_json"
	GIT_TAG "master"
	PATCH_COMMAND ""
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	TEST_COMMAND ""
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	parse_template_prj
	GIT_REPOSITORY "https://github.com/beached/parse_template.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/parse_template"
	GIT_TAG "master"
	PATCH_COMMAND ""
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	TEST_COMMAND ""
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
    lib_nodepp_prj
    GIT_REPOSITORY "https://github.com/beached/lib_nodepp.git"
    SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/lib_nodepp"
    GIT_TAG "master"
    PATCH_COMMAND ""
    INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
    TEST_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

include_directories( SYSTEM "${CMAKE_BINARY_DIR}/install/include" )
link_directories( "${CMAKE_BINARY_DIR}/install/lib" )

include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
link_directories( ${Boost_LIBRARY_DIRS} )
include_directories( SYSTEM ${OPENSSL_INCLUDE_DIR} )

set( NODEPPRFB_DEPS header_libraries_prj char_range_prj parse_json_prj lib_nodepp_prj )
set( NODEPPRFB_LIBS nodepp char_range parse_json  ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} )

set( SOURCE_FILES
		nodepp_rfb.h
		rfb_message.h
		nodepp_rfb.cpp
		nodepp_rfb_test.cpp
		)

add_executable( nodepp_rfb ${SOURCE_FILES} )
add_dependencies( nodepp_rfb ${NODEPPRFB_DEPS} )

target_link_libraries( nodepp_rfb ${NODEPPRFB_LIBS} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
