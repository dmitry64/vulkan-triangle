cmake_minimum_required(VERSION 3.1)

project(engine)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3)
find_package(Vulkan)

include_directories(Vulkan-Hpp/vulkan)

add_subdirectory(graphics)
include_directories(graphics)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} vulkan VkLayer_utils Xxf86vm Xinerama Xcursor Xrandr X11 pthread dl graphics glfw)
