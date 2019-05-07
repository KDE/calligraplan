
include_directories(
    ${PLAN_SOURCE_DIR}
    ${PLANMAIN_INCLUDES}
)

set(plannerimport_PART_SRCS 
   plannerimport.cpp
)


add_library(planplannerimport MODULE ${plannerimport_PART_SRCS})
# calligraplan_filter_desktop_to_json(planplannerimport plan_planner_import.desktop)
if(${KF5_VERSION} VERSION_LESS "5.16.0")
    kcoreaddons_desktop_to_json(planplannerimport plan_planner_import.desktop)
else()
    kcoreaddons_desktop_to_json(planplannerimport plan_planner_import.desktop
        SERVICE_TYPES ${PLAN_SOURCE_DIR}/servicetypes/calligraplan_filter.desktop
    )
endif()

target_link_libraries(planplannerimport planprivate plankernel planmain)

install(TARGETS planplannerimport  DESTINATION ${PLUGIN_INSTALL_DIR}/calligraplan/formatfilters)
