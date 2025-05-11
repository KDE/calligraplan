
include_directories(
    ${PLAN_SOURCE_DIR}
    ${PLAN_SOURCE_DIR}
    ${PLANMAIN_INCLUDES}
)

set(kplatoimport_PART_SRCS 
   kplatoimport.cpp
)


add_library(calligraplankplatoimport MODULE ${kplatoimport_PART_SRCS})
# calligraplan_filter_desktop_to_json(plankplatoimport plan_kplato_import.desktop)
# kcoreaddons_desktop_to_json(calligraplankplatoimport plan_kplato_import.desktop
#     SERVICE_TYPES ${PLAN_SOURCE_DIR}/servicetypes/calligraplan_filter.desktop
# )

target_link_libraries(calligraplankplatoimport calligraplanprivate calligraplankernel calligraplanmain)

install(TARGETS calligraplankplatoimport  DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligraplan/formatfilters)
