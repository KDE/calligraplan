
set(PLAN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)

set(PLANPLUGIN_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/plugin
)

set(PLANKUNDO2_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/kundo2
    ${CMAKE_CURRENT_BINARY_DIR}/libs/kundo2
)

set(PLANSTORE_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/store
    ${CMAKE_CURRENT_BINARY_DIR}/libs/store
)

set(PLANODF_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/odf
    ${CMAKE_CURRENT_BINARY_DIR}/libs/odf
    ${PLANSTORE_INCLUDES}
)

set(PLANWIDGETUTILS_INCLUDES 
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_CURRENT_BINARY_DIR}/libs/widgetutils
)
set(PLANWIDGETS_INCLUDES 
    ${PLANWIDGETUTILS_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/widgets
    ${CMAKE_CURRENT_BINARY_DIR}/libs/widgets
)

set(PLANMAIN_INCLUDES 
    ${PLANWIDGETS_INCLUDES}
    ${PLANODF_INCLUDES}
    ${PLANKUNDO2_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/main
    ${CMAKE_CURRENT_BINARY_DIR}/libs/main
)

set(PLANKERNEL_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/kernel
    ${CMAKE_CURRENT_BINARY_DIR}/libs/kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/kernel/commands
    ${PLANWIDGETS_INCLUDES}
)    

set(PLANMODELS_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/models
    ${CMAKE_CURRENT_BINARY_DIR}/libs/models
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/models/commands
)    

set(PLANUI_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ui
    ${CMAKE_CURRENT_BINARY_DIR}/libs/ui
)    

set(PLAN_INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}

    ${PLANKERNEL_INCLUDES}
    ${PLANMODELS_INCLUDES}
    ${PLANUI_INCLUDES}
    ${PLANMAIN_INCLUDES}
)

# For odf
set(RNG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/devtools/scripts)

if(KF5Holidays_FOUND)
    add_definitions(-DHAVE_KHOLIDAYS)
endif()

# Define plugin install prefix
set (PLAN_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${PLUGIN_INSTALL_DIR}")


if (KF5AkonadiContact_FOUND)
    # disable for now: there is a bug
    # it only works if you use kde contacts (of course) but many use other stuff, so gets dissapointed
    add_definitions(-DPLAN_KDEPIMLIBS_FOUND)
    message(WARNING "AkonadiContacs available, but function is disabled due to Bug 311940")
endif ()

if (PLANCHARTDEBUG)
    add_definitions(-DPLAN_CHART_DEBUG)
endif ()

#add_subdirectory(interfaces)
add_subdirectory(servicetypes)

add_subdirectory( templates )
add_subdirectory( pics )
add_subdirectory( toolbar )
add_subdirectory( plugins )

add_subdirectory( libs )

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

add_subdirectory( workpackage )
add_subdirectory(portfolio)

include_directories(${PLAN_INCLUDES})

add_definitions(-DTRANSLATION_DOMAIN=\"calligraplan\")

########### KPlato private library ###############

set(planprivate_LIB_SRCS
    welcome/WelcomeView.cpp

    kptviewlistdocker.cpp
    kptviewlist.cpp
    kptviewlistdialog.cpp

    kptschedulesdocker.cpp

    kptconfig.cpp

    config/ConfigWorkVacationPanel.cpp
    config/ConfigProjectPanel.cpp
    config/kpttaskdefaultpanel.cpp
    config/kptworkpackageconfigpanel.cpp
    config/kptcolorsconfigpanel.cpp
    config/ConfigDocumentationPanel.cpp
    config/ConfigTaskModulesPanel.cpp
    config/ConfigProjectTemplatesPanel.cpp
    config/ConfigDialog.cpp

    kptcontext.cpp

    kptfactory.cpp
    kptpart.cpp
    kptmaindocument.cpp
    kptview.cpp
#     KPtViewAdaptor.cpp
    kptprintingcontrolprivate.cpp

    kptschedulerpluginloader.cpp
    kptbuiltinschedulerplugin.cpp
    kptconfigskeleton.cpp

    kptinsertfiledlg.cpp
    kptloadsharedprojectsdialog.cpp

    KPlatoXmlLoader.cpp
)

ki18n_wrap_ui(planprivate_LIB_SRCS
    welcome/WelcomeView.ui

    kptviewlistaddview.ui
    kptviewlisteditview.ui
    kptviewlisteditcategory.ui

    config/ConfigWorkVacationPanel.ui
    config/ConfigProjectPanel.ui
    config/kptconfigtaskpanelbase.ui
    config/kptworkpackageconfigpanel.ui
    config/kptcolorsconfigpanel.ui
    config/ConfigDocumentationPanel.ui
    config/ConfigTaskModulesPanel.ui
    config/ConfigProjectTemplatesPanel.ui

    kptinsertfilepanel.ui
)

kconfig_add_kcfg_files(plansettings_SRCS calligraplansettings.kcfgc)

add_library(planprivate SHARED ${planprivate_LIB_SRCS} ${plansettings_SRCS} )
generate_export_header(planprivate BASE_NAME plan)

target_link_libraries(planprivate
    PUBLIC
        plankernel
        planmodels
        planui
        planmain
    PRIVATE
        planplugin
        KF5::IconThemes
        #KF5::KHtml
)
if(KF5AkonadiContact_FOUND)
    target_link_libraries(planprivate  PRIVATE KF5::AkonadiContact)
endif()
set_target_properties(planprivate PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS planprivate ${INSTALL_TARGETS_DEFAULT_ARGS})

########### KPlato part ###############

set(planpart_PART_SRCS kptfactoryinit.cpp )

add_library(calligraplanpart MODULE ${planpart_PART_SRCS})

#calligraplan_part_desktop_to_json(calligraplanpart planpart.desktop)
if(${KF5_VERSION} VERSION_LESS "5.16.0")
    kcoreaddons_desktop_to_json(calligraplanpart planpart.desktop)
else()
    kcoreaddons_desktop_to_json(calligraplanpart planpart.desktop
#         SERVICE_TYPES ${PLAN_SOURCE_DIR}/servicetypes/calligraplan_part.desktop
    )
endif()


target_link_libraries(calligraplanpart PUBLIC KF5::Parts KF5::CoreAddons PRIVATE planprivate)

install(TARGETS calligraplanpart DESTINATION ${PLUGIN_INSTALL_DIR}/calligraplan/parts)


########### KPlato executable ###############

set(calligraplan_KDEINIT_SRCS main.cpp )

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/*-apps-calligraplan.png")
ecm_add_app_icon(kdeinit_app_ICONS_SRCS ICONS ${ICONS_SRCS})
if(WIN32)
    set(_resourcefile "${CMAKE_CURRENT_BINARY_DIR}/kdeinit_app_ICONS_SRCS.rc")
endif()

kf5_add_kdeinit_executable( calligraplan ${calligraplan_KDEINIT_SRCS})

if (APPLE)
   set_target_properties(calligraplan PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
   set_target_properties(calligraplan PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.calligra.plan")
   set_target_properties(calligraplan PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Plan")
   install( FILES ${CMAKE_CURRENT_BINARY_DIR}/calligraplan_KDEINIT_SRCS.icns DESTINATION ${BUNDLE_INSTALL_DIR}/calligraplan.app/Contents/Resources)
endif ()

target_link_libraries(kdeinit_calligraplan planmain planprivate)

install(TARGETS kdeinit_calligraplan  ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries(calligraplan kdeinit_calligraplan planmain)
install(TARGETS calligraplan  ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES  calligraplan.rc calligraplan_readonly.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/calligraplan)
install( PROGRAMS  org.kde.calligraplan.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES  calligraplanrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES calligraplansettings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(FILES org.kde.calligraplan.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

# TODO: with the new embedded JSON data for plugins there is no schema ATM to define extended properties
# plan_viewplugin.desktop


configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

#add_custom_target(apidox doc/api/gendocs.pl WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
