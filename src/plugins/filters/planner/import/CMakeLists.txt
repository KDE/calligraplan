
include_directories(
    ${PLAN_SOURCE_DIR}
    ${PLANMAIN_INCLUDES}
)

set(plannerimport_PART_SRCS 
   plannerimport.cpp
)


add_library(calligraplanplannerimport MODULE ${plannerimport_PART_SRCS})
# calligraplan_filter_desktop_to_json(planplannerimport plan_planner_import.desktop)
# kcoreaddons_desktop_to_json(calligraplanplannerimport plan_planner_import.desktop
#     SERVICE_TYPES ${PLAN_SOURCE_DIR}/servicetypes/calligraplan_filter.desktop
# )

target_link_libraries(calligraplanplannerimport calligraplanprivate calligraplankernel calligraplanmain)

install(TARGETS calligraplanplannerimport  DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligraplan/formatfilters)
