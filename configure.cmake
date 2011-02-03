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

# setup compiler
if(UNIX)
	set(CMAKE_CXX_FLAGS -std=c++0x)
endif(UNIX)


if(DOXYGEN)
	include(UseDoxygen.cmake)
else(DOXYGEN)
	message(WARNING "Doxygen was not found - documentation will not be built")
endif(DOXYGEN)

include(fun.cmake)


# CONFIGURE INCLUDE
####################
include_directories(src model SYSTEM)
configure_file(src/git/config.h.in src/git/config.h)

# CONFIGURE LIBRARIES
#####################
add_library(gitpp STATIC src/git/db/odb.cpp)

# CONFIGURE EXECUTABLES
########################


# TEST SETUP
############
enable_testing()
add_model_test_executable(model_odb_test model_odb 
					test/gitmodel/db/model_odb_test.cpp)
add_lib_test_executable(lib_odb_test lib_odb
					test/git/db/lib_odb_test.cpp)

