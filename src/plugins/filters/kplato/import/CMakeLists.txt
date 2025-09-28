
include_directories(
    ${PLAN_SOURCE_DIR}
    ${PLAN_SOURCE_DIR}
    ${PLANMAIN_INCLUDES}
)

set(kplatoimport_PART_SRCS 
   kplatoimport.cpp
)


add_library(calligraplankplatoimport MODULE ${kplatoimport_PART_SRCS})

target_link_libraries(calligraplankplatoimport calligraplanprivate calligraplankernel calligraplanmain)

install(TARGETS calligraplankplatoimport  DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligraplan/formatfilters)
