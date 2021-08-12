
##
## Test SharedMimeInfo
##
macro_optional_find_package(SharedMimeInfo 1.3)
set_package_properties(SharedMimeInfo PROPERTIES
    PURPOSE "Required by Calligra Plan Portfolio Manager."
    TYPE REQUIRED
)

include_directories(${PLAN_INCLUDES})

set(planportfolioprivate_LIB_SRCS
    PlanGroupDebug.cpp
    SummaryModel.cpp
    SummaryView.cpp
    ProjectsModel.cpp
    GanttModel.cpp
    PortfolioModel.cpp
    PortfolioView.cpp
    PerformanceView.cpp
    DetailsView.cpp
    SchedulingModel.cpp
    SchedulingLogModel.cpp
    SchedulingView.cpp
    GanttView.cpp
    Part.cpp
    MainDocument.cpp
    View.cpp

    PortfolioFactory.cpp
)

ki18n_wrap_ui(planportfolioprivate_LIB_SRCS
    PerformanceView.ui
    SchedulingView.ui
)

add_library(planportfolioprivate SHARED ${planportfolioprivate_LIB_SRCS})
generate_export_header(planportfolioprivate BASE_NAME planportfolio)

target_link_libraries(planportfolioprivate
    PUBLIC
        plankernel
        planmodels
        planui
        planmain
        planstore
        planwidgets
    PRIVATE
        planplugin
        KF5::ItemModels
        KF5::ItemViews
        KChart
        #KF5::IconThemes
        #KF5::KHtml
)

set_target_properties(planportfolioprivate PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION})

install(TARGETS planportfolioprivate ${INSTALL_TARGETS_DEFAULT_ARGS})

########### Part ###############

set(planportfoliopart_PART_SRCS PortfolioFactoryInit.cpp)

add_library(calligraplanportfoliopart MODULE ${planportfoliopart_PART_SRCS})

kcoreaddons_desktop_to_json(calligraplanportfoliopart calligraplanportfoliopart.desktop)


target_link_libraries(calligraplanportfoliopart PUBLIC KF5::Parts KF5::CoreAddons PRIVATE planportfolioprivate)

install(TARGETS calligraplanportfoliopart DESTINATION ${PLUGIN_INSTALL_DIR}/calligraplan/parts)

########### Executable ###############

set(calligraplanportfolio_KDEINIT_SRCS main.cpp )

file(GLOB ICONS_SRCS "${CMAKE_SOURCE_DIR}/src/pics/*-apps-calligraplan.png") # TODO
ecm_add_app_icon(kdeinit_app_ICONS_SRCS ICONS ${ICONS_SRCS})
if(WIN32)
    set(_resourcefile "${CMAKE_CURRENT_BINARY_DIR}/kdeinit_app_ICONS_SRCS.rc")
endif()

kf5_add_kdeinit_executable( calligraplanportfolio ${calligraplanportfolio_KDEINIT_SRCS})

if (APPLE)
   set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
   set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.calligra.plan.portfolio")
   set_target_properties(calligraplanportfolio PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Plan Portfolio")
   install( FILES ${CMAKE_CURRENT_BINARY_DIR}/calligraplanportfolio_KDEINIT_SRCS.icns DESTINATION ${BUNDLE_INSTALL_DIR}/calligraplanportfolio.app/Contents/Resources)
endif ()

target_link_libraries(kdeinit_calligraplanportfolio planportfolioprivate)

install(TARGETS kdeinit_calligraplanportfolio  ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries(calligraplanportfolio kdeinit_calligraplanportfolio)
install(TARGETS calligraplanportfolio  ${INSTALL_TARGETS_DEFAULT_ARGS})

########### Files ###############

install( FILES
        Portfolio_ViewUi.rc
        Portfolio_SummaryViewUi.rc
        Portfolio_PortfolioViewUi.rc
        Portfolio_DetailsViewUi.rc
        Portfolio_SchedulingViewUi.rc
        Portfolio_GanttViewUi.rc
    DESTINATION ${KXMLGUI_INSTALL_DIR}/calligraplanportfolio
)

if (SharedMimeInfo_FOUND)
    install(FILES calligraplanportfolio_mimetype.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
    update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
endif ()

install( PROGRAMS  org.kde.calligraplanportfolio.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
