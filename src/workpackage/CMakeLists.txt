#project(planwork)

add_definitions(-DTRANSLATION_DOMAIN=\"calligraplanwork\")

add_subdirectory( templates )
add_subdirectory( pics )
#add_subdirectory( toolbar )
#add_subdirectory( tests )

include_directories(${PLAN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/)


########### PlanWork private library ###############

set(planworkprivate_LIB_SRCS
    factory.cpp
    mainwindow.cpp
    part.cpp
    view.cpp
    workpackage.cpp

    taskworkpackagemodel.cpp
    taskworkpackageview.cpp

    taskcompletiondialog.cpp

    debugarea.cpp

    ../KPlatoXmlLoader.cpp #NB
)

ki18n_wrap_ui(planworkprivate_LIB_SRCS
    taskcompletionpanel.ui
)

kconfig_add_kcfg_files(planworksettings_SRCS calligraplanworksettings.kcfgc)

add_library(planworkprivate SHARED ${planworkprivate_LIB_SRCS} ${planworksettings_SRCS})
generate_export_header(planworkprivate BASE_NAME planwork)

target_link_libraries(planworkprivate
    PUBLIC
        plankernel
        planmodels
        planui
        planprivate
        planmain
        KGantt
        KF5::Parts
    PRIVATE
        KF5::IconThemes
)

set_target_properties(planworkprivate PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS planworkprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES calligraplanworksettings.kcfg DESTINATION ${KCFG_INSTALL_DIR})


########### Calligra PlanWork executable ###############

set(calligraplanwork_KDEINIT_SRCS main.cpp commandlineparser.cpp)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/*-apps-calligraplanwork.png")
ecm_add_app_icon(kdeinit_app_ICONS_SRCS ICONS ${ICONS_SRCS})
if(WIN32)
    set(_resourcefile "${CMAKE_CURRENT_BINARY_DIR}/kdeinit_app_ICONS_SRCS.rc")
endif()

kf5_add_kdeinit_executable(calligraplanwork ${calligraplanwork_KDEINIT_SRCS})

if (APPLE)
   set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
   set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.calligra.plan.work")
   set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Plan Work")
endif ()

target_link_libraries(
    kdeinit_calligraplanwork

    planworkprivate
    plankernel
    planmain
    KF5::Parts
    KF5::IconThemes
    KF5::WindowSystem
    KF5::DBusAddons
)

install(TARGETS kdeinit_calligraplanwork  ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries( calligraplanwork kdeinit_calligraplanwork )

install(TARGETS calligraplanwork  ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES  calligraplanwork.rc calligraplanwork_readonly.rc calligraplanworkui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/calligraplanwork)
install( FILES  calligraplanworkrc DESTINATION ${CONFIG_INSTALL_DIR})
install( PROGRAMS  org.kde.calligraplanwork.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})

