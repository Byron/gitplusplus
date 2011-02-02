project(git++ CXX)

# Create a test executable, and setup a test
# add the source files as last arguments
function(add_test_executable name testname)
	add_executable(${name} ${ARGN})
	set_target_properties(${name} 
		PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test)
	get_target_property(execpath ${name} RUNTIME_OUTPUT_DIRECTORY)
	add_test(${testname} ${execpath}/${name})
endfunction(add_test_executable)


set(${PROJECT_NAME}_VERSION_MAJOR 1)
set(${PROJECT_NAME}_VERSION_MINOR 0)

# CONFIGURE INCLUDE
include_directories(include SYSTEM)

# CONFIGURE EXECUTABLES


# TEST SETUP
enable_testing()
add_test_executable(odb_test model_odb 
					test/git/model/db/odb_test.cpp)

