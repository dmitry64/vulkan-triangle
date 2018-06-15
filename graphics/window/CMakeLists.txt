find_package(Vulkan)
find_package(glfw3)

add_library(window "window.cpp" "window.h")
target_link_libraries(window glfw vulkan)
