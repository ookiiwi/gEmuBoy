function(set_compiler_warnings project_name)
	set(CLANG_WARNINGS
		-Wall
        -Wextra # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        # catch hard to track down memory errors
        -Wunused # warn on anything being unused
        -Wpedantic # warn if non-standard C is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
	)

    set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
        -Wduplicated-cond # warn if if / else chain has duplicated conditions
        -Wduplicated-branches # warn if if / else branches have duplicated code
        -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
        -Wuseless-cast # warn if you perform a cast to the same type
        -Wsuggest-override # warn if an overridden member function is not marked 'override' or 'final'
    )

	message(TRACE "Warnings are treated as errors")
    list(APPEND CLANG_WARNINGS -Werror)
    list(APPEND GCC_WARNINGS -Werror)

	if(MSVC)
    	set(PROJECT_WARNINGS_C ${MSVC_WARNINGS})
	elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    	set(PROJECT_WARNINGS_C ${CLANG_WARNINGS})
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    	set(PROJECT_WARNINGS_C ${GCC_WARNINGS})
	endif()

	target_compile_options( ${project_name}	INTERFACE $<$<COMPILE_LANGUAGE:C>:${PROJECT_WARNINGS_C}> )
endfunction()
