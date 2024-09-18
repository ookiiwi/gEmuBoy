# Most of the code here is taken or inspired by https://github.com/cpp-best-practices/cmake_template/blob/main/cmake/Sanitizers.cmake

function(enable_sanitizers project_name)
	set(SANITIZERS "address")

	if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL ".*Clang")
		list(APPEND SANITIZERS "leak" "undefined" )
	# MSVC only supports address sanitizer
	endif()

	list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)

	if (NOT MSVC)
		target_compile_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
		target_link_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
	else()
		message(SEND_ERROR "MSVC not supported by this script")
	endif()
endfunction()
