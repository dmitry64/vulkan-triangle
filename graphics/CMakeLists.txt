add_subdirectory(window)
include_directories(window)

file(GLOB Sources "*.cpp")
file(GLOB Headers "*.h")

add_library(graphics STATIC ${Sources})
target_link_libraries(graphics window)
