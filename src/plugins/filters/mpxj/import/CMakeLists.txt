message(STATUS "Building mpxj import filters")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLANMAIN_INCLUDES}
)

set(PLAN_FILTER_INSTALL_DIR ${KDE_INSTALL_PLUGINDIR}/calligraplan/formatfilters)

add_library(planmpxjimport MODULE mpxjimport.cpp)

target_link_libraries(planmpxjimport calligraplanmain)
install(TARGETS planmpxjimport DESTINATION ${PLAN_FILTER_INSTALL_DIR})

if (SharedMimeInfo_FOUND)
    install(FILES plan_mpxj_mimetype.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
    update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})
endif ()
