if(CMAKE_VERSION VERSION_LESS "2.8.10")
    find_program(HG_EXECUTABLE hg)
else()
    find_package(Hg QUIET)
endif()
find_package(Git QUIET) # present in 2.8.8

# defaults, in case everything below fails
set(JM_VERSION "unknown")
set(JM_LATEST_TAG "0.0")
set(JM_TAG_DISTANCE "0")

if(EXISTS ${CMAKE_SOURCE_DIR}/../.hg_archival.txt)
    # read the lines of the archive summary file to extract the version
    file(READ ${CMAKE_SOURCE_DIR}/../.hg_archival.txt archive)
    STRING(REGEX REPLACE "\n" ";" archive "${archive}")
    foreach(f ${archive})
        string(FIND "${f}" ": " pos)
        string(SUBSTRING "${f}" 0 ${pos} key)
        string(SUBSTRING "${f}" ${pos} -1 value)
        string(SUBSTRING "${value}" 2 -1 value)
        set(hg_${key} ${value})
    endforeach()
    if(DEFINED hg_tag)
        set(JM_VERSION ${hg_tag} CACHE STRING "jm version string.")
        set(JM_LATEST_TAG ${hg_tag})
        set(JM_TAG_DISTANCE "0")
    elseif(DEFINED hg_node)
        string(SUBSTRING "${hg_node}" 0 16 hg_id)
        set(JM_VERSION "${hg_latesttag}+${hg_latesttagdistance}-${hg_id}")
    endif()
elseif(HG_EXECUTABLE AND EXISTS ${CMAKE_SOURCE_DIR}/../.hg)
    execute_process(COMMAND
        ${HG_EXECUTABLE} log -r. --template "{latesttag}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE JM_LATEST_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(COMMAND
        ${HG_EXECUTABLE} log -r. --template "{latesttagdistance}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE JM_TAG_DISTANCE
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
        COMMAND
        ${HG_EXECUTABLE} log -r. --template "{node|short}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE HG_REVISION_ID
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    if(JM_LATEST_TAG MATCHES "^r")
        string(SUBSTRING ${JM_LATEST_TAG} 1 -1 JM_LATEST_TAG)
    endif()
    if(JM_TAG_DISTANCE STREQUAL "0")
        set(JM_VERSION "${JM_LATEST_TAG}")
    else()
        set(JM_VERSION "${JM_LATEST_TAG}+${JM_TAG_DISTANCE}-${HG_REVISION_ID}")
    endif()
elseif(GIT_EXECUTABLE AND EXISTS ${CMAKE_SOURCE_DIR}/.git) # check whereis .git 
    #message(STATUS "git tools")
    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE JM_LATEST_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} describe --tags
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE JM_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
endif()

message(STATUS "jm version ${JM_VERSION}")
