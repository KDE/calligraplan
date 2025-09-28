
add_definitions(-DTRANSLATION_DOMAIN=\"calligraplanportfolio\")

include_directories(${PLANKERNEL_INCLUDES} ${PLANUI_INCLUDES} config)

set(calligraplanportfolioprivate_LIB_SRCS
    PlanGroupDebug.cpp

    ScheduleManagerDelegate.cpp
    ScheduleManagerInfoDialog.cpp

    SummaryModel.cpp
    SummaryView.cpp
    ProjectsModel.cpp
    GanttModel.cpp
    PortfolioModel.cpp
    PortfolioView.cpp
    PerformanceView.cpp
    ProgressView.cpp
    ResourceModel.cpp
    ResourceUsageModel.cpp
    ResourceUsageView.cpp
    SchedulingModel.cpp
    SchedulingLogModel.cpp
    SchedulingView.cpp
    GanttView.cpp
    DocumentsSaveDialog.cpp
    MainWindow.cpp
    Part.cpp
    MainDocument.cpp
    View.cpp

    PortfolioFactory.cpp
    config/ConfigSkeleton.cpp
    config/ConfigDialog.cpp
)

ki18n_wrap_ui(calligraplanportfolioprivate_LIB_SRCS
    PortfolioView.ui
    PerformanceView.ui
    SchedulingView.ui
    ResourceUsageView.ui
    DocumentsSaveDialog.ui
    ScheduleManagerInfoDialog.ui
)

kconfig_add_kcfg_files(calligraplanportfoliosettings_SRCS config/portfoliosettings.kcfgc)

add_library(calligraplanportfolioprivate SHARED ${calligraplanportfolioprivate_LIB_SRCS} ${calligraplanportfoliosettings_SRCS})
generate_export_header(calligraplanportfolioprivate BASE_NAME planportfolio)

target_link_libraries(calligraplanportfolioprivate
    PUBLIC
        calligraplanui
        calligraplanstore
    PRIVATE
        calligraplanplugin
        KF6::ItemModels
        KF6::ItemViews
        KChart6
        #KF6::IconThemes
)

set_target_properties(calligraplanportfolioprivate PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION})

install(TARGETS calligraplanportfolioprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### Part ###############

set(calligraplanportfoliopart_PART_SRCS PortfolioFactoryInit.cpp)

add_library(calligraplanportfoliopart MODULE ${calligraplanportfoliopart_PART_SRCS})



target_link_libraries(calligraplanportfoliopart PUBLIC KF6::Parts KF6::CoreAddons PRIVATE calligraplanportfolioprivate)

install(TARGETS calligraplanportfoliopart DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligraplan/parts)

########### Executable ###############

set(calligraplanportfolio_SRCS main.cpp)

file(GLOB ICONS_SRCS "icons/*-apps-calligraplanportfolio.*") # TODO proper icon for portfolio
ecm_add_app_icon(calligraplanportfolio_SRCS ICONS ${ICONS_SRCS})
if(WIN32)
    set(_resourcefile "${CMAKE_CURRENT_BINARY_DIR}/calligraplanportfolio_SRCS.rc")
endif()

add_executable( calligraplanportfolio ${calligraplanportfolio_SRCS})

if (APPLE)
    set(PLAN_VERSION_NUMBER_STRING "${PLAN_STABLE_VERSION_MAJOR}.${PLAN_STABLE_VERSION_MINOR}.${PLAN_VERSION_RELEASE}")
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.calligraplanportfolio")
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Plan Portfolio")
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_ICON_FILE "calligraplanportfolio_SRCS.icns")
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${PLAN_VERSION_STRING})
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${PLAN_VERSION_NUMBER_STRING})
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PLAN_VERSION_NUMBER_STRING})
    set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_COPYRIGHT "GNU Public License, V2 or, at your option, any later version.")
endif ()

target_link_libraries(calligraplanportfolio calligraplanportfolioprivate)

install(TARGETS calligraplanportfolio  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### Files ###############

install( FILES
        calligraplanportfolio_shell.rc
        Portfolio_ViewUi.rc
        Portfolio_SummaryViewUi.rc
        Portfolio_PortfolioViewUi.rc
        Portfolio_ProgressViewUi.rc
        Portfolio_SchedulingViewUi.rc
        Portfolio_GanttViewUi.rc
        Portfolio_ResourceUsageViewUi.rc
        Portfolio_PerformanceViewUi.rc
        ../libs/ui/TaskStatusViewUi.rc
        ../libs/ui/GanttViewUi.rc
    DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/calligraplanportfolio
)

if (SharedMimeInfo_FOUND)
    install(FILES calligraplanportfolio_mimetype.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
    update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})
endif ()

install( PROGRAMS  org.kde.calligraplanportfolio.desktop  DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.calligraplanportfolio.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

add_subdirectory(icons)
