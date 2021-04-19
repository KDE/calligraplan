
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
    Factory.cpp
    Part.cpp
    MainDocument.cpp
    View.cpp
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

### Part ###
set(planportfoliopart_PART_SRCS Factoryinit.cpp)

add_library(calligraplanportfoliopart MODULE ${planportfoliopart_PART_SRCS})

kcoreaddons_desktop_to_json(calligraplanportfoliopart calligraplanportfoliopart.desktop)


target_link_libraries(calligraplanportfoliopart PUBLIC KF5::Parts KF5::CoreAddons PRIVATE planportfolioprivate)

install(TARGETS calligraplanportfoliopart DESTINATION ${PLUGIN_INSTALL_DIR}/calligraplan/parts)

install( FILES
        ViewUi.rc
        SummaryViewUi.rc
        PortfolioViewUi.rc
        #DetailsViewUi.rc
        SchedulingViewUi.rc
    DESTINATION ${KXMLGUI_INSTALL_DIR}/calligraplan
)

if (SharedMimeInfo_FOUND)
    install(FILES calligraplanportfolio_mimetype.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
    update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
endif ()
