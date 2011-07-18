project(git++ CXX)
set(${PROJECT_NAME}_VERSION_MAJOR 1)
set(${PROJECT_NAME}_VERSION_MINOR 0)

# general path configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)

# add the profiling configuration. Its essentially the release config, but
# compiles with profiling instructions, enabling gprof
if(CMAKE_CONFIGURATION_TYPES)
	list(APPEND CMAKE_CONFIGURATION_TYPES Profile)
	list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
		"Add the configurations that we need"
		FORCE)
endif()

set( CMAKE_CXX_FLAGS_PROFILE "-O3 -DNDEBUG -pg" CACHE STRING
    "Flags used by the C++ compiler during PROFILE builds."
    FORCE )
set( CMAKE_EXE_LINKER_FLAGS_PROFILE
    "-pg" CACHE STRING
    "Flags used for linking binaries during PROFILE builds."
    FORCE )
set( CMAKE_SHARED_LINKER_FLAGS_PROFILE
    "-pg" CACHE STRING
    "Flags used by the shared libraries linker during PROFILE builds."
    FORCE )
mark_as_advanced(
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE
    CMAKE_SHARED_LINKER_FLAGS_PROFILE )

#CMAKE SETUP AND CONFIGURATION
###############################
# setup modules
include(FindDoxygen)

set(Boost_USE_STATIC_LIBS        ON)

find_package(Boost 1.45.0 COMPONENTS date_time filesystem system unit_test_framework) 

if(NOT Boost_FOUND)
	message(SEND_ERROR "Require boost libraries")
endif()

# setup compiler
if(UNIX)
	# show all warnings
	# use c++0x features
	# make throw() semantically equivalent to noexcept (std::exception uses throw() for instance)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}\ -Wall -std=c++0x -fnothrow-opt")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile")
	add_definitions(-DDEBUG)
endif()

if(DOXYGEN)
	include(UseDoxygen.cmake)
else(DOXYGEN)
	message(WARNING "Doxygen was not found - documentation will not be built")
endif()

include(fun.cmake)


# CONFIGURE INCLUDE
###################
include_directories(${CMAKE_BINARY_DIR}/src src test ${Boost_INCLUDE_DIR} SYSTEM)
link_directories(${Boost_LIBRARY_DIRS})
configure_file(src/git/config.h.in src/git/config.h)

# CONFIGURE LIBRARIES
#####################
add_library(gitpp STATIC
			src/git/obj/object.cpp
			src/git/obj/stream.cpp
			src/git/obj/tree.cpp
			src/git/obj/commit.cpp
			src/git/obj/tag.cpp
			src/git/obj/blob.cpp
			src/git/obj/multiobj.cpp
			src/git/db/odb_mem.cpp
			src/git/db/odb_loose.cpp
			src/git/db/odb_pack.cpp
			src/git/db/pack_file.cpp
			src/git/db/pack_stream.cpp
			src/git/db/util.cpp
			src/git/db/sha1_gen.cpp)

# CONFIGURE EXECUTABLES
########################


# TEST SETUP
############
enable_testing()
add_model_test_executable(model_odb_test model_odb 
					test/gtl/db/model_odb_test.cpp)
add_lib_test_executable(lib_odb_test lib_odb
					test/git/db/lib_odb_test.cpp)
add_lib_test_executable(lib_sha1_performance_test lib_sha1_perf
					test/git/db/sha1_performance_test.cpp)
add_lib_test_executable(lib_looseodb_performance_test lib_looseodb_perf
					test/git/db/looseodb_performance_test.cpp)
add_lib_test_executable(lib_packodb_performance_test lib_packodb_perf
					test/git/db/packodb_performance_test.cpp)

