# TJScheduler does not build on Windows
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "TJScheduler not supported on Windows")
else()
    add_subdirectory(tj)
endif()
