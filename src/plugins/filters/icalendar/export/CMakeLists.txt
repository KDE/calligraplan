
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLAN_SOURCE_DIR}
    ${PLANKERNEL_INCLUDES}
    ${PLANMODELS_INCLUDES}
    ${PLANUI_INCLUDES}
    ${PLANMAIN_INCLUDES}
)

set(icalendarexport_SRCS 
   icalendarexport.cpp
   ICalExportDialog.cpp
)

ki18n_wrap_ui(icalendarexport_SRCS
    ICalExportDialog.ui
)

add_library(planicalexport MODULE ${icalendarexport_SRCS})
#calligraplan_filter_desktop_to_json(planicalexport plan_icalendar_export.desktop)
if(${KF5_VERSION} VERSION_LESS "5.16.0")
    kcoreaddons_desktop_to_json(planicalexport plan_icalendar_export.desktop)
else()
    kcoreaddons_desktop_to_json(planicalexport plan_icalendar_export.desktop
        SERVICE_TYPES ${PLAN_SOURCE_DIR}/servicetypes/calligraplan_filter.desktop
    )
endif()

target_link_libraries(planicalexport planprivate plankernel planmain)

if (KF5CalendarCore_FOUND)
    target_link_libraries(planicalexport KF5::CalendarCore)
endif()

install(TARGETS planicalexport  DESTINATION ${PLUGIN_INSTALL_DIR}/calligraplan/formatfilters)
