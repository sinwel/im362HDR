# vim: syntax=cmake
if(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()
message(STATUS "cmake version ${CMAKE_VERSION}")
if(NOT CMAKE_VERSION VERSION_LESS "2.8.12.20131121")
    cmake_policy(SET CMP0025 OLD) # report Apple's Clang as just Clang
    cmake_policy(SET CMP0042 OLD) # do not require MACOSX_RPATH
endif()

project (imx362_xm4)
cmake_minimum_required (VERSION 2.8.8) # OBJECT libraries require 2.8.8
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)


#set(CMAKE_C_STANDARD_LIBRARIES_INIT "kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib Ws2_32.lib" )
#message(${CMAKE_C_STANDARD_LIBRARIES_INIT})			   
			   
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

option(CHECKED_BUILD "Enable run-time sanity checks (debugging)" OFF)
if(CHECKED_BUILD)
    add_definitions(-DCHECKED_BUILD=1)
endif()
option(CEVA_XM4_BUILD "Enable CEVA_XM4_BUILD option " ON)
if(CEVA_XM4_BUILD)
    add_definitions(-DCEVA_XM4_BUILD=1)
endif()

# System architecture detection
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSPROC)
set(X86_ALIASES x64 x86 i386 i686 x86_64 amd64)
list(FIND X86_ALIASES "${SYSPROC}" X86MATCH)
if("${SYSPROC}" STREQUAL "" OR X86MATCH GREATER "-1")
    message(STATUS "Detected x86 system processor")
    set(X86 1)
    add_definitions(-DPREISP_ARCH_X86=1)
    #if("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
    #    set(X64 1)
    #    add_definitions(-DX86_64=1)
    #    message(STATUS "Define X86_6 to 1")
    #endif()
elseif(${SYSPROC} STREQUAL "armv6l")
    message(STATUS "Detected ARM system processor")
    set(ARM 1)
    add_definitions(-DPREISP_ARCH_X86=1 -DHAVE_ARMV6=1)
else()
    message(STATUS "CMAKE_SYSTEM_PROCESSOR value `${CMAKE_SYSTEM_PROCESSOR}` is unknown")
    message(STATUS "Please add this value near ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}")
endif()

if(UNIX)
    SET(PLATFORM_LIBS pthread)
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} rt)
    endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(UNIX)

# Compiler detection
if(CMAKE_GENERATOR STREQUAL "Xcode")
  set(XCODE 1)
endif()
if (APPLE)
  add_definitions(-DMACOS)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CLANG 1)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(INTEL_CXX 1)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(GCC 1)
endif()

if(INTEL_CXX AND WIN32)
    # treat icl roughly like MSVC
    set(MSVC 1)
endif()
if(MSVC)
    option(STATIC_LINK_CRT "Statically link C runtime for release builds" OFF)
    if (STATIC_LINK_CRT)
        set(CompilerFlags CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS_RELEASE)
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        endforeach()
    endif (STATIC_LINK_CRT)
    add_definitions(/W3)  # Full warnings
    add_definitions(/Ob2) # always inline
    add_definitions(/MP)  # multithreaded build

    # disable Microsofts suggestions for proprietary secure APIs
    add_definitions(/D_CRT_SECURE_NO_WARNINGS)

	
    check_include_files(stdint.h HAVE_STDINT_H)
    if(NOT HAVE_STDINT_H)
        include_directories(compat/msvc)
    endif()
endif(MSVC)

if(INTEL_CXX AND UNIX)
    # treat icpc roughly like gcc
    set(GCC 1)
    add_definitions(-Wall -Wextra -Wshadow)
elseif(CLANG)
    # treat clang roughly like gcc
    set(GCC 1)
    add_definitions(-Wall -Wextra -Wshadow -ffast-math)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall -Wextra -Wshadow -ffast-math)
    check_cxx_compiler_flag(-Wno-narrowing GCC_HAS_NO_NARROWING) 
    check_cxx_compiler_flag(-mstackrealign GCC_HAS_STACK_REALIGN) 
    if (GCC_HAS_STACK_REALIGN)
        add_definitions(-mstackrealign)
    endif()
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
endif()
if(GCC)
    if(X64 AND NOT WIN32)
        add_definitions(-fPIC)
    endif(X64 AND NOT WIN32)
    if(X86 AND NOT X64)
        add_definitions(-march=i686)
    endif()
    if(ARM)
        add_definitions(-march=armv6 -mfloat-abi=hard -mfpu=vfp)
    endif()
endif(GCC)

find_package(Yasm)
if(YASM_FOUND AND X86)
    if (YASM_VERSION_STRING VERSION_LESS "1.2.0")
        message(STATUS "Yasm version ${YASM_VERSION_STRING} is too old. 1.2.0 or later required")
        option(ENABLE_ASSEMBLY "Enable use of assembly coded primitives" OFF)
    else()
        message(STATUS "Found Yasm ${YASM_VERSION_STRING} to build assembly primitives")
        option(ENABLE_ASSEMBLY "Enable use of assembly coded primitives" ON)
    endif()
endif()

# Build options
set(LIB_INSTALL_DIR lib CACHE STRING "Install location of libraries")
set(BIN_INSTALL_DIR bin CACHE STRING "Install location of executables")


option(WARNINGS_AS_ERRORS "Stop compiles on first warning" OFF)
if(WARNINGS_AS_ERRORS)
    if(GCC)
        add_definitions(-Werror)
    elseif(MSVC)
        add_definitions(/WX)
    endif()
endif(WARNINGS_AS_ERRORS)


if (WIN32)
    # Visual leak detector
    find_package(VLD QUIET)
    if(VLD_FOUND)
        add_definitions(-DHAVE_VLD)
        include_directories(${VLD_INCLUDE_DIRS})
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${VLD_LIBRARIES})
        link_directories(${VLD_LIBRARY_DIRS})
    endif()
    option(WINXP_SUPPORT "Make binaries compatible with Windows XP" OFF)
    if(WINXP_SUPPORT)
        # force use of workarounds for CONDITION_VARIABLE and atomic
        # intrinsics introduced after XP
        add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WINXP)
    endif()
endif()
# Dependency libs from opencv.
if (CEVA_XM4_BUILD)
	set(PLATFORM_LIBS ${PLATFORM_LIBS}
		"C:/CEVA-ToolBox/V16/XM4/CEVA-XM4/Vec-C/xm4/lib/vs2010/libarchitectured.lib"   )
	
endif()
# opencv include files.
include_directories(. C:/CEVA-ToolBox/V16/XM4/CEVA-XM4/Vec-C/xm4/include/include  "${PROJECT_BINARY_DIR}")
#add_subdirectory(lencod)
include_directories(. ./include "${PROJECT_BINARY_DIR}")

if((MSVC_IDE OR XCODE) AND ENABLE_ASSEMBLY)
    # this is required because of this cmake bug
    # http://www.cmake.org/Bug/print_bug_page.php?bug_id=8170
    if(WIN32)
        set(SUFFIX obj)
    else()
        set(SUFFIX o)
    endif()
    foreach(ASM ${MSVC_ASMS})
        set(YASM_SRC ${CMAKE_CURRENT_SOURCE_DIR}/common/x86/${ASM})
        list(APPEND YASM_SRCS ${YASM_SRC})
        list(APPEND YASM_OBJS ${ASM}.${SUFFIX})
        add_custom_command(
            OUTPUT ${ASM}.${SUFFIX}
            COMMAND ${YASM_EXECUTABLE} ARGS ${YASM_FLAGS} ${YASM_SRC} -o ${ASM}.${SUFFIX}
            DEPENDS ${YASM_SRC})
    endforeach()
endif()

source_group(ASM FILES ${YASM_SRCS})

# for static
#add_library(modual-static STATIC ${YASM_OBJS} ${YASM_SRCS}) #$<TARGET_OBJECTS:TLibCommon>
 
#if(NOT MSVC)
#    set_target_properties(modual-static PROPERTIES OUTPUT_NAME modual)
#endif()
#install(TARGETS modual-static
#    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
#    ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
	
install(FILES DESTINATION include)


if(NOT (MSVC_IDE OR XCODE))
    add_custom_target(clean-generated COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/clean-generated.cmake)
endif()

option(ENABLE_SHARED "Build shared library" OFF)

if(NOT WIN32)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
                   IMMEDIATE @ONLY)
    add_custom_target(uninstall
                      "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")
endif()

# Main imx362_xm4 application
option(ENABLE_APP "Build standalone imx362_xm4 application" ON)
if(ENABLE_APP)
   
	file(GLOB preIspFiles   src/main.cpp
							src/zigZagBayerInterpo.cpp
							src/hdr_process.cpp
							include/XM4_defines.h
							include/rk_bayerwdr.h						
							include/rk_global.h	
							include/rk_typedef.h)
	source_group(PreISP FILES ${preIspFiles})
	
    check_include_files(getopt.h HAVE_GETOPT_H)
    if(NOT HAVE_GETOPT_H)
        if(MSVC)
            set_source_files_properties(compat/getopt/getopt.c PROPERTIES COMPILE_FLAGS "/wd4100 /wd4131 -DHAVE_STRING_H=1")
        endif(MSVC)
        include_directories(compat/getopt)
        set(GETOPT compat/getopt/getopt.c compat/getopt/getopt.h)
    endif(NOT HAVE_GETOPT_H)

    if(XCODE)
        # Xcode seems unable to link the imx362_xm4 with libs, so link as one targget
        add_executable(imx362_xm4 ${preIspFiles} ${YASM_OBJS} ${YASM_SRCS} ) #$<TARGET_OBJECTS:TLibCommon>
    else()
		add_executable(imx362_xm4 ${preIspFiles} ${YASM_OBJS} ${YASM_SRCS} ) #$<TARGET_OBJECTS:TLibCommon>
		     
        if(WIN32 OR NOT ENABLE_SHARED OR INTEL_CXX)
            # The imx362_xm4 cannot link to the shared library on Windows, it
            # requires internal APIs not exported from the DLL
            target_link_libraries(imx362_xm4 ${PLATFORM_LIBS})
        else()
            target_link_libraries(imx362_xm4 ${PLATFORM_LIBS} )
        endif()
    endif()
    set_target_properties(imx362_xm4 PROPERTIES OUTPUT_NAME PreISP_exe)

    install(TARGETS imx362_xm4 DESTINATION ${BIN_INSTALL_DIR})
endif(ENABLE_APP)
