
include_directories(
    ${PLAN_SOURCE_DIR}
    ${PLANMAIN_INCLUDES}
)

set(plannerimport_PART_SRCS 
   plannerimport.cpp
)


add_library(calligraplanplannerimport MODULE ${plannerimport_PART_SRCS})

target_link_libraries(calligraplanplannerimport calligraplanprivate calligraplankernel calligraplanmain)

install(TARGETS calligraplanplannerimport  DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligraplan/formatfilters)
