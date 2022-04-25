#project(calligraplanwork)

add_definitions(-DTRANSLATION_DOMAIN=\"calligraplanwork\")

add_subdirectory( templates )
add_subdirectory( pics )
#add_subdirectory( toolbar )
#add_subdirectory( tests )

include_directories(${PLAN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/)


########### PlanWork private library ###############

set(calligraplanworkprivate_LIB_SRCS
    factory.cpp
    mainwindow.cpp
    part.cpp
    view.cpp
    workpackage.cpp

    taskworkpackagemodel.cpp
    taskworkpackageview.cpp

    taskcompletiondialog.cpp

    debugarea.cpp

    ../plan/KPlatoXmlLoader.cpp #NB
)

ki18n_wrap_ui(calligraplanworkprivate_LIB_SRCS
    taskcompletionpanel.ui
)

kconfig_add_kcfg_files(calligraplanworksettings_SRCS calligraplanworksettings.kcfgc)

add_library(calligraplanworkprivate SHARED ${calligraplanworkprivate_LIB_SRCS} ${calligraplanworksettings_SRCS})
generate_export_header(calligraplanworkprivate BASE_NAME planwork)

target_link_libraries(calligraplanworkprivate
    PUBLIC
        calligraplanui
        calligraplanprivate
        calligraplanmain
        KGantt
        KF5::Parts
    PRIVATE
        KF5::IconThemes
)

set_target_properties(calligraplanworkprivate PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS calligraplanworkprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES calligraplanworksettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})


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

    calligraplanworkprivate

    KF5::Parts
    KF5::IconThemes
    KF5::WindowSystem
    KF5::DBusAddons
)

install(TARGETS kdeinit_calligraplanwork  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries( calligraplanwork kdeinit_calligraplanwork )

install(TARGETS calligraplanwork  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES  calligraplanwork.rc calligraplanwork_readonly.rc calligraplanworkui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/calligraplanwork)
install( FILES  calligraplanworkrc DESTINATION ${KDE_INSTALL_CONFDIR})
install( PROGRAMS  org.kde.calligraplanwork.desktop  DESTINATION ${KDE_INSTALL_APPDIR})

