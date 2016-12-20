CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(imgui)
ADD_SUBDIRECTORY(demo)

IF(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
ENDIF()

INCLUDE_DIRECTORIES(include thirdparty)

SET (IMGUI_SOURCE_FILES
	src/ui.cpp
	src/io.cpp
	src/render.cpp
	src/rollout.cpp
	src/toolbars.cpp
	src/render_queue.cpp)

SET(IMGUI_HEADER_FILES
	include/ui.h
	include/io.h
	include/rollout.h
	include/platform.h
	include/toolbars.h
	include/render_queue.h
	thirdparty/stb_truetype.h)

SET_SOURCE_FILES_PROPERTIES(IMGUI_HEADER_FILES PROPERTIES HEADER_FILE_ONLY TRUE)
LIST(APPEND IMGUI_SOURCE_FILES ${IMGUI_HEADER_FILES})

ADD_LIBRARY (imgui ${IMGUI_SOURCE_FILES})


