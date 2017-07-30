CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)

ENABLE_TESTING()
ADD_SUBDIRECTORY(gtests)

INCLUDE_DIRECTORIES(${GTEST_SRC_DIRS})
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../include)

IF(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")
ENDIF()

ADD_EXECUTABLE (tests 	imgui_io.cpp
						imgui_platform.cpp
						imgui_platform.cpp
						imgui_render_queue.cpp
						imgui_rollout.cpp
						imgui_toolbars.cpp
						imgui_ui.cpp
						imgui_main.cpp)

ADD_TEST(test_imgui tests)


