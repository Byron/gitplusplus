
# Create a test executable, and setup a test
# add the source files as last arguments
function(add_model_test_executable name testname)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} boost_unit_test_framework.a boost_filesystem.a boost_system.a)
	set_target_properties(${name} 
		PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test)
	get_target_property(execpath ${name} RUNTIME_OUTPUT_DIRECTORY)
	add_test(${testname} ${execpath}/${name})
endfunction(add_model_test_executable)

function(add_lib_test_executable name testname)
	add_model_test_executable(${name} ${testname} ${ARGN})
	target_link_libraries(${name} gitpp)
endfunction(add_lib_test_executable)
