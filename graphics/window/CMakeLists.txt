find_package(Vulkan)
find_package(glfw3)

add_library(window "window.cpp" "window.h")
target_link_libraries(window glfw vulkan VkLayer_utils Xxf86vm Xinerama Xcursor Xrandr X11 pthread dl)
