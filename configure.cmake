project(git++ CXX)
set(${PROJECT_NAME}_VERSION_MAJOR 1)
set(${PROJECT_NAME}_VERSION_MINOR 0)

# general path configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)

#CMAKE SETUP AND CONFIGURATION
###############################
# setup modules
include(FindDoxygen DOXYGEN_SKIP_DOT)

set(Boost_USE_STATIC_LIBS        ON)

find_package(Boost 1.36.0 COMPONENTS date_time filesystem system unit_test_framework) 

if(NOT Boost_FOUND)
	message(SEND_FAILURE "Require boost libraryies")
endif(NOT Boost_FOUND)

# setup compiler
if(UNIX)
	set(CMAKE_CXX_FLAGS "-Wall -std=c++0x")
endif(UNIX)


if(DOXYGEN)
	include(UseDoxygen.cmake)
else(DOXYGEN)
	message(WARNING "Doxygen was not found - documentation will not be built")
endif(DOXYGEN)

include(fun.cmake)


# CONFIGURE INCLUDE
####################
include_directories(src test SYSTEM)
configure_file(src/git/config.h.in src/git/config.h)

# CONFIGURE LIBRARIES
#####################
add_library(gitpp STATIC
			src/git/obj/tree.cpp
			src/git/obj/commit.cpp
			src/git/obj/tag.cpp
			src/git/obj/blob.cpp
			src/git/db/odb.cpp 
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

