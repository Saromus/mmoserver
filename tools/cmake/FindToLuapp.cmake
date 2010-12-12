FIND_PATH(TOLUAPP_INCLUDE_DIR tolua++.h
    PATHS
        $ENV{TOLUAPP_ROOT}
        ${TOLUAPP_ROOT}
    HINTS
        $ENV{TOLUAPP_ROOT}/include
        ${TOLUAPP_ROOT}/include
)
MARK_AS_ADVANCED(TOLUAPP_INCLUDE_DIR)

FIND_LIBRARY(TOLUAPP_LIBRARY_DEBUG
    NAMES tolua++ toluapp toluapp.lib
    PATH
        $ENV{TOLUAPP_ROOT}
        ${TOLUAPP_ROOT}
    HINTS
        $ENV{TOLUAPP_ROOT}/lib
        ${TOLUAPP_ROOT}/lib
        $ENV{TOLUAPP_ROOT}/win32/vc9/withLua51_Debug
        ${TOLUAPP_ROOT}/win32/vc9/withLua51_Debug
)

FIND_LIBRARY(TOLUAPP_LIBRARY_RELEASE
    NAMES tolua++ toluapp toluapp.lib
    PATH
        $ENV{TOLUAPP_ROOT}
        ${TOLUAPP_ROOT}
    HINTS
		$ENV{TOLUAPP_ROOT}/lib
        ${TOLUAPP_ROOT}/lib
        $ENV{TOLUAPP_ROOT}/win32/vc9/withLua51_Release
        ${TOLUAPP_ROOT}/win32/vc9/withLua51_Release
)
        
IF(TOLUAPP_INCLUDE_DIR AND TOLUAPP_LIBRARY_DEBUG AND TOLUAPP_LIBRARY_RELEASE)
    SET(TOLUAPP_FOUND TRUE)
ENDIF()

IF(TOLUAPP_FOUND)
    IF (NOT ToLuapp_FIND_QUIETLY)
        MESSAGE(STATUS "Found TOLUAPP")
    ENDIF()
ELSE()
    IF (ToLuapp_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find TOLUAPP")
    ENDIF()
ENDIF()
