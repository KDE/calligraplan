#project(calligraplanwork)

add_definitions(-DTRANSLATION_DOMAIN=\"calligraplanwork\")

add_subdirectory( templates )
add_subdirectory(icons)
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
        KGantt6
        KF6::Parts
    PRIVATE
        KF6::IconThemes
)

set_target_properties(calligraplanworkprivate PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS calligraplanworkprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES calligraplanworksettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})


########### Calligra PlanWork executable ###############

set(calligraplanwork_SRCS main.cpp commandlineparser.cpp)

file(GLOB ICONS_SRCS "icons/*-apps-calligraplanwork.*")
ecm_add_app_icon(calligraplanwork_SRCS ICONS ${ICONS_SRCS})
if(WIN32)
    set(_resourcefile "${CMAKE_CURRENT_BINARY_DIR}/calligraplanwork_SRCS.rc")
endif()

add_executable(calligraplanwork ${calligraplanwork_SRCS})

if (APPLE)
    set(PLAN_VERSION_NUMBER_STRING "${PLAN_STABLE_VERSION_MAJOR}.${PLAN_STABLE_VERSION_MINOR}.${PLAN_VERSION_RELEASE}")
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.calligraplanwork")
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Plan Work")
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_ICON_FILE "calligraplanwork_SRCS.icns")
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${PLAN_VERSION_STRING})
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${PLAN_VERSION_NUMBER_STRING})
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PLAN_VERSION_NUMBER_STRING})
    set_target_properties(calligraplanwork PROPERTIES MACOSX_BUNDLE_COPYRIGHT "GNU Public License, V2 or, at your option, any later version.")
endif ()

target_link_libraries(
    calligraplanwork

    calligraplanworkprivate

    KF6::Parts
    KF6::IconThemes
    KF6::WindowSystem
    KF6::DBusAddons
)

install(TARGETS calligraplanwork  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES  calligraplanwork.rc calligraplanwork_readonly.rc calligraplanworkui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/calligraplanwork)
install( FILES  calligraplanworkrc DESTINATION ${KDE_INSTALL_CONFDIR})
install( PROGRAMS  org.kde.calligraplanwork.desktop  DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.calligraplanwork.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
