add_subdirectory(window)
include_directories(window)

set(SOURCES "application.cpp" "vertex.cpp")
set(HEADERS "application.h" "debugcallbacks.h" "helperfunctions.h" "vertex.h")

add_library(graphics ${SOURCES} ${HEADERS})
target_link_libraries(graphics window)
