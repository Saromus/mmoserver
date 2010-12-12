# AddMMOServerLibrary is a standardized way to build libraries in the MMOServer
# project. Particularly on windows platforms this manages all the
# machinary to set up a default environment and creating/building/running unit
# unit tests an afterthought for developers.
#
# Function Definition:
#
# AddMMOServerLibrary(library_name
#                     MMOSERVER_DEPS [ARGS] [args1...]           # Dependencies on other MMOServer projects
#                     ADDITIONAL_INCLUDE_DIRS [ARGS] [args1...]  # Additional directories to search for includes
#                     ADDITIONAL_SOURCE_DIRS [ARGS] [args1...]   # Additional directories to search for files to include in the project
#                     DEBUG_LIBRARIES [ARGS] [args1....]         # Additional debug libraries to link the project against
#                     OPTIMIZED_LIBRARIES [ARGS] [args1...])     # Additional optimized libraries to link the project against
#
#
########################
# Simple Example Usage:
########################
#
# include(MMOServerLibrary)
# 
# AddMMOServerLibrary(Common
#     MMOSERVER_DEPS 
#         Utils 
# )
#
#
#########################
# Complex Example Usage:
#########################
# include(MMOServerLibrary)
#
# AddMMOServerLibrary(ScriptEngine
#     MMOSERVER_DEPS 
#         Utils
#         Common
#     ADDITIONAL_SOURCE_DIRS
#         ${CMAKE_CURRENT_SOURCE_DIR}/glue_files
#     ADDITIONAL_INCLUDE_DIRS
#         ${LUA_INCLUDE_DIR} 
#         ${NOISE_INCLUDE_DIR} 
#         ${TOLUAPP_INCLUDE_DIR}
#     DEBUG_LIBRARIES
#         ${LUA_LIBRARY_DEBUG}
#         ${NOISE_LIBRARY_DEBUG}
#         ${TOLUAPP_LIBRARY_DEBUG}
#     OPTIMIZED_LIBRARIES
#         ${LUA_LIBRARY_RELEASE}
#         ${NOISE_LIBRARY_RELEASE}
#         ${TOLUAPP_LIBRARY_RELEASE}
# )
#

INCLUDE(CMakeMacroParseArguments)

FUNCTION(AddMMOServerLibrary name)
    PARSE_ARGUMENTS(MMOSERVERLIB "MMOSERVER_DEPS;ADDITIONAL_INCLUDE_DIRS;ADDITIONAL_SOURCE_DIRS;DEBUG_LIBRARIES;OPTIMIZED_LIBRARIES" "" ${ARGN})
    
    LIST(LENGTH MMOSERVERLIB_DEBUG_LIBRARIES _debug_list_length)
    LIST(LENGTH MMOSERVERLIB_OPTIMIZED_LIBRARIES _optimized_list_length)
    LIST(LENGTH MMOSERVERLIB_MMOSERVER_DEPS _project_deps_list_length)
    LIST(LENGTH MMOSERVERLIB_ADDITIONAL_INCLUDE_DIRS _includes_list_length)
    LIST(LENGTH MMOSERVERLIB_ADDITIONAL_SOURCE_DIRS _sources_list_length)
            
    # Grab all of the source files and all of the unit test files.
    FILE(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)          
    FILE(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)          
    FILE(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*_unittest.cc ${CMAKE_CURRENT_SOURCE_DIR}/*_unittest.cpp)
        
    IF(_sources_list_length GREATER 0)
        FOREACH(_source_dir ${MMOSERVERLIB_ADDITIONAL_SOURCE_DIRS})
            FILE(GLOB ADDITIONAL_SOURCES ${_source_dir}/*.cc ${_source_dir}/*.cpp)
            FILE(GLOB ADDITIONAL_HEADERS ${_source_dir}/*.h)
            FILE(GLOB ADDITIONAL_TEST_SOURCES ${_source_dir}/*_unittest.cc ${_source_dir}/*_unittest.cpp)
            
            LIST(APPEND SOURCES ${ADDITIONAL_SOURCES})
            LIST(APPEND HEADERS ${ADDITIONAL_HEADERS})
            LIST(APPEND TEST_SOURCES ${ADDITIONAL_TEST_SOURCES})
        ENDFOREACH()
    ENDIF()
    
    LIST(LENGTH TEST_SOURCES _tests_list_length)    
    IF(_tests_list_length GREATER 0)
        LIST(REMOVE_ITEM SOURCES ${TEST_SOURCES}) # Remove the unit tests from the sources list.        
    ENDIF()
    
    IF(_includes_list_length GREATER 0)
        INCLUDE_DIRECTORIES(${MMOSERVERLIB_ADDITIONAL_INCLUDE_DIRS})
    ENDIF()
    
    # Create the Common library
    ADD_LIBRARY(${name} STATIC ${SOURCES} ${HEADERS})    
    
    IF(_project_deps_list_length GREATER 0)
        ADD_DEPENDENCIES(${name} ${MMOSERVERLIB_MMOSERVER_DEPS})
    ENDIF()

    IF(_tests_list_length GREATER 0)
        # Create an executable for the test and link it to gtest and anh
        INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
        ADD_EXECUTABLE(${name}Tests ${TEST_SOURCES})
        TARGET_LINK_LIBRARIES(${name}Tests 
            ${name}
            ${MMOSERVERLIB_MMOSERVER_DEPS}
            ${GTEST_BOTH_LIBRARIES}
            debug ${Boost_DATE_TIME_LIBRARY_DEBUG}
            debug ${Boost_REGEX_LIBRARY_DEBUG}
            debug ${Boost_SYSTEM_LIBRARY_DEBUG}
            debug ${Boost_THREAD_LIBRARY_DEBUG}
            debug ${GLOG_LIBRARY_DEBUG}
            debug ${TBB_LIBRARY_DEBUG}
            debug ${TBB_MALLOC_LIBRARY_DEBUG}      
            optimized ${Boost_DATE_TIME_LIBRARY_RELEASE}
            optimized ${Boost_REGEX_LIBRARY_RELEASE}
            optimized ${Boost_SYSTEM_LIBRARY_RELEASE}
            optimized ${Boost_THREAD_LIBRARY_RELEASE}
            optimized ${GLOG_LIBRARY_RELEASE}
            optimized ${TBB_LIBRARY}
            optimized ${TBB_MALLOC_LIBRARY})
                
        IF(_project_deps_list_length GREATER 0)
            ADD_DEPENDENCIES(${name}Tests ${MMOSERVERLIB_MMOSERVER_DEPS})
        ENDIF()
    
        IF(_debug_list_length GREATER 0)
            TARGET_LINK_LIBRARIES(${name}Tests debug ${MMOSERVERLIB_DEBUG_LIBRARIES})
        ENDIF()
    
        IF(_optimized_list_length GREATER 0)
            TARGET_LINK_LIBRARIES(${name}Tests optimized ${MMOSERVERLIB_OPTIMIZED_LIBRARIES})
        ENDIF()
        
        IF(WIN32)
            # Set the default output directory for binaries for convenience.
            SET_TARGET_PROPERTIES(${name}Tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}")
                      
            # Create a custom built user configuration so that the "run in debug mode"
            # works without any issues.
    	    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/tools/windows/user_project.vcxproj.in 
    	        ${CMAKE_CURRENT_BINARY_DIR}/${name}Tests.vcxproj.user @ONLY)
    	ENDIF()
        
        GTEST_ADD_TESTS(${name}Tests "" ${TEST_SOURCES})
      
        IF(ENABLE_TEST_REPORT)
            ADD_TEST(NAME All${name}Tests COMMAND ${name}Tests "--gtest_output=xml:${PROJECT_BINARY_DIR}/$<CONFIGURATION>/")
        ENDIF()
    ENDIF()
ENDFUNCTION()