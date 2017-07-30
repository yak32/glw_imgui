CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(CMakeToolsHelpers OPTIONAL)
PROJECT(imgui)

OPTION(GLW_IMGUI_DEMO "Build demo app" ON)
OPTION(GLW_BUILD_TESTS "Build tests" ON)

IF (GLW_IMGUI_DEMO)
	ADD_SUBDIRECTORY(demo)
ENDIF()

IF(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
ENDIF()

INCLUDE_DIRECTORIES(include thirdparty)

MESSAGE("GLW_JSON_INCLUDE_DIRS: ${GLW_JSON_INCLUDE_DIRS}")

SET (IMGUI_SOURCE_FILES
	src/imgui_ui.cpp
	src/imgui_io.cpp
	src/imgui_render.cpp
	src/imgui_rollout.cpp
	src/imgui_toolbars.cpp
	src/imgui_render_queue.cpp
	thirdparty/texture_atlas/texture_atlas.cpp
	)

SET(IMGUI_HEADER_FILES
	include/imgui_ui.h
	include/imgui_io.h
	include/imgui_rollout.h
	include/imgui_platform.h
	include/imgui_toolbars.h
	include/imgui_render_queue.h
	thirdparty/stb/stb_truetype.h
	thirdparty/texture_atlas/texture_atlas.h)

SET_SOURCE_FILES_PROPERTIES(IMGUI_HEADER_FILES PROPERTIES HEADER_FILE_ONLY TRUE)
LIST(APPEND IMGUI_SOURCE_FILES ${IMGUI_HEADER_FILES})

ADD_LIBRARY (glw_imgui STATIC ${IMGUI_SOURCE_FILES})

IF (GLW_BUILD_TESTS)
	ADD_SUBDIRECTORY(tests)
ENDIF()

