cmake_minimum_required(VERSION 2.8)

project(engine)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(/home/deus/repo/3rdparty/glfw-3.2.1/include)
link_directories(/home/deus/repo/3rdparty/glfw-build/src)

include_directories(/opt/vulkan-sdk/VulkanSDK/1.0.46.0/x86_64/include)
link_directories(/opt/vulkan-sdk/VulkanSDK/1.0.46.0/x86_64/lib)

include_directories(/home/deus/repo/3rdparty/stb)

add_subdirectory(graphics)
include_directories(graphics)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} vulkan VkLayer_utils Xxf86vm Xinerama Xcursor Xrandr X11 pthread dl graphics)
