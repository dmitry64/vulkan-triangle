
include_directories(../stb)

add_subdirectory(window)
include_directories(window)


add_library(graphics
    "application.cpp"
    "application.h"
    "debugcallbacks.h"
    "helperfunctions.h"
    )
target_link_libraries(graphics window)
