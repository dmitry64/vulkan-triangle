add_library(window SHARED "window.cpp" "window.h")
target_link_libraries(window glfw3 vulkan)
