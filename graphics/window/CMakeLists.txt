file(GLOB Sources "*.cpp")
file(GLOB Headers "*.h")

add_library(window STATIC ${Sources})
target_link_libraries(window glfw3 vulkan)
