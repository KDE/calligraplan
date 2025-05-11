message(STATUS "Building mpxj import filters")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLANMAIN_INCLUDES}
)

set(PLAN_FILTER_INSTALL_DIR ${KDE_INSTALL_PLUGINDIR}/calligraplan/formatfilters)

add_library(planmpxjimport MODULE mpxjimport.cpp)
# kcoreaddons_desktop_to_json(planmpxjimport plan_mpxj_import.desktop
#     SERVICE_TYPES ${PLAN_SOURCE_DIR}/servicetypes/calligraplan_filter.desktop
# )

target_link_libraries(planmpxjimport calligraplanmain)
install(TARGETS planmpxjimport DESTINATION ${PLAN_FILTER_INSTALL_DIR})

if (SharedMimeInfo_FOUND)
    install(FILES plan_mpxj_mimetype.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
    update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})
endif ()
