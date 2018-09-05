if (MSVC)
    # Match defaults for debugging and pdb creation settings in Visual Studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_UNICODE /DUNICODE /sdl")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /Gy /Oi /Oy- /GL")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:FULL /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /LTCG:incremental /ENTRY:mainCRTStartup")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK /SUBSYSTEM:WINDOWS /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG:FULL /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /LTCG:incremental")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK /SUBSYSTEM:WINDOWS /SAFESEH:NO")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /DEBUG:FULL /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /LTCG:incremental")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} /LTCG")
endif()

# NO_FORCE_INCLUDE to not force include the precompiled header
macro(enable_precompiled_header target header source)
    set (extra_macro_args ${ARGN})
    if (MSVC)
        if (extra_macro_args STREQUAL "NO_FORCE_INCLUDE")
            set_target_properties(${target} PROPERTIES COMPILE_FLAGS "/Yu\"${header}\"")
        else()
            set_target_properties(${target} PROPERTIES COMPILE_FLAGS "/Yu\"${header}\" /FI\"${header}\"")
        endif()
        set_source_files_properties(${source} PROPERTIES COMPILE_FLAGS "/Yc\"${header}\"")
    endif()
endmacro(enable_precompiled_header)

# Useful binaries
set(CYGPATH_BIN "${CMAKE_CURRENT_LIST_DIR}/bin/cygpath.exe")
set(RSYNC_BIN "${CMAKE_CURRENT_LIST_DIR}/bin/rsync.exe")
set(WGET_BIN "${CMAKE_CURRENT_LIST_DIR}/bin/wget.exe")
set(SED_BIN "${CMAKE_CURRENT_LIST_DIR}/bin/sed.exe")
set(JUNCTION_BIN "${CMAKE_CURRENT_LIST_DIR}/bin/junction.exe")

macro(link_dir target link)
    if (WIN32)
        # https://gitlab.kitware.com/cmake/cmake/issues/17412
        # execute_process(COMMAND cmd /C rmdir /SQ "${link}"")
        # execute_process(COMMAND cmd /C mklink /J "${link}" "${target}")
        execute_process(COMMAND ${JUNCTION_BIN} -accepteula "${link}" "${target}")
    else()
        execute_process(COMMAND rm -fr "${target}")
        execute_process(COMMAND ln -s "${target}" "${link}")
    endif()
endmacro(link_dir)

macro(set_output_directory output_directory)
    # First for the generic no-config case (e.g. with mingw)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${output_directory})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${output_directory})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${output_directory})

    # Second, for multi-config builds (e.g. msvc)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${output_directory})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${output_directory})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${output_directory})
    endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
endmacro(set_output_directory)

macro(set_target_output_directory target output_directory)
    # First for the generic no-config case (e.g. with mingw)
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${output_directory}")
    set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${output_directory}")
    set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${output_directory}")

    # Second, for multi-config builds (e.g. msvc)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${output_directory}")
        set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${output_directory}")
        set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${output_directory}")
    endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
endmacro(set_target_output_directory)

macro(subdirlist result curdir)
  file(GLOB children RELATIVE "${curdir}" "${curdir}/*")
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY "${curdir}/${child}")
      list(APPEND dirlist "${child}")
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

# INCLUDE_CURRENT to add source in the current directory
macro(glob_source_subdirs sources curdir)
    set(ret "")
	subdirlist(SUBDIRS ${curdir})
	foreach(subdir ${SUBDIRS})
		file(GLOB_RECURSE SOURCE_FILES "${curdir}/${subdir}/*.cpp")
		file(GLOB_RECURSE HEADER_FILES "${curdir}/${subdir}/*.h")
		source_group("${subdir}\\Source Files" FILES ${SOURCE_FILES})
		source_group("${subdir}\\Header Files" FILES ${HEADER_FILES})
		set(ret ${ret} ${HEADER_FILES} ${SOURCE_FILES})
	endforeach()

	set (extra_macro_args ${ARGN})
    if (extra_macro_args STREQUAL "INCLUDE_CURRENT")
        file(GLOB SOURCE_FILES "${curdir}/${subdir}/*.cpp")
        file(GLOB HEADER_FILES "${curdir}/${subdir}/*.h")
		set(ret ${ret} ${HEADER_FILES} ${SOURCE_FILES})
    endif()

	set(${sources} ${ret})
endmacro()

macro(get_unix_path outputvar path)
    if (WIN32)
        execute_process(
            COMMAND ${CYGPATH_BIN} "${path}"
            OUTPUT_VARIABLE ${outputvar}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        set(outputvar ${path})
    endif()
endmacro()

# More sources to be added on the target can be appended to the call
function(add_script_target target script)
    get_filename_component(filename ${script} NAME_WE)
    get_filename_component(scriptdir ${script} DIRECTORY)
    get_filename_component(extension ${script} EXT)
    string(SUBSTRING ${extension} 1 -1 extension)
    get_filename_component(realext ${extension} NAME_WE)
    if (scriptdir STREQUAL "")
        set(scriptdir "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    set(scriptfilein "${scriptdir}/${filename}.${realext}")
    if(EXISTS "${scriptfilein}.in")
        set(scriptfilein "${scriptfilein}.in")
    elseif(NOT EXISTS "${scriptfilein}")
        message(FATAL_ERROR "Missing script file")
    endif()
    
    set(scriptfileout "${CMAKE_CURRENT_BINARY_DIR}/${filename}.${realext}")
    configure_file(${scriptfilein} ${scriptfileout} @ONLY)

	# Append sources
	set(resources ${ARGN})
	set(sources ${script} ${resources})
    add_custom_target(${target} ALL
        COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=$<CONFIG> -P ${scriptfileout}
        SOURCES ${sources}
        VERBATIM
    )
endfunction()
