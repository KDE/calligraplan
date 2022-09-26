
add_subdirectory( templates )
add_subdirectory(icons)

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

include_directories(${PLAN_INCLUDES})

add_definitions(-DTRANSLATION_DOMAIN=\"calligraplan\")

########### KPlato private library ###############

set(calligraplanprivate_LIB_SRCS
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
    config/ConfigTaskModulesPanel.cpp
    config/ConfigProjectTemplatesPanel.cpp
    config/ConfigReportTemplatesPanel.cpp
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

    KPlatoXmlLoader.cpp
    SharedResourcesDialog.cpp
)

ki18n_wrap_ui(calligraplanprivate_LIB_SRCS
    kptviewlistaddview.ui
    kptviewlisteditview.ui
    kptviewlisteditcategory.ui

    config/ConfigWorkVacationPanel.ui
    config/ConfigProjectPanel.ui
    config/kptconfigtaskpanelbase.ui
    config/kptworkpackageconfigpanel.ui
    config/kptcolorsconfigpanel.ui
    config/ConfigTaskModulesPanel.ui
    config/ConfigProjectTemplatesPanel.ui
    config/ConfigReportTemplatesPanel.ui

    kptinsertfilepanel.ui
    SharedResourcesDialog.ui
    CreateReportTemplateDialog.ui
)

kconfig_add_kcfg_files(calligraplansettings_SRCS calligraplansettings.kcfgc)

add_library(calligraplanprivate SHARED ${calligraplanprivate_LIB_SRCS} ${calligraplansettings_SRCS} )
generate_export_header(calligraplanprivate BASE_NAME plan)

target_link_libraries(calligraplanprivate
    PUBLIC
        calligraplanui
    PRIVATE
        calligraplanplugin
        KF5::IconThemes
        #KF5::KHtml
)
if(KF5AkonadiContact_FOUND)
    target_link_libraries(calligraplanprivate  PRIVATE KF5::AkonadiContact)
endif()
set_target_properties(calligraplanprivate PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS calligraplanprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### KPlato part ###############

set(calligraplanpart_PART_SRCS kptfactoryinit.cpp )

add_library(calligraplanpart MODULE ${calligraplanpart_PART_SRCS})

#calligraplan_part_desktop_to_json(calligraplanpart planpart.desktop)
kcoreaddons_desktop_to_json(calligraplanpart planpart.desktop
#         SERVICE_TYPES ${PLAN_SOURCE_DIR}/servicetypes/calligraplan_part.desktop
)


target_link_libraries(calligraplanpart PUBLIC KF5::Parts KF5::CoreAddons PRIVATE calligraplanprivate)

install(TARGETS calligraplanpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligraplan/parts)


########### KPlato executable ###############

set(calligraplan_SRCS main.cpp )

file(GLOB ICONS_SRCS "icons/*-apps-calligraplan.*")
ecm_add_app_icon(calligraplan_SRCS ICONS ${ICONS_SRCS})
if(WIN32)
    set(_resourcefile "${CMAKE_CURRENT_BINARY_DIR}/calligraplan_SRCS.rc")
endif()

add_executable( calligraplan ${calligraplan_SRCS})

if (APPLE)
   set_target_properties(calligraplan PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
   set_target_properties(calligraplan PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.calligra.plan")
   set_target_properties(calligraplan PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Plan")
   set_target_properties(calligraplan PROPERTIES MACOSX_BUNDLE_ICON_FILE "calligraplan_SRCS.icns")
   install( FILES ${CMAKE_CURRENT_BINARY_DIR}/calligraplan_SRCS.icns DESTINATION ${KDE_INSTALL_BUNDLEDIR}/calligraplan.app/Contents/Resources)
endif ()

target_link_libraries(calligraplan calligraplanmain calligraplanprivate)

install(TARGETS calligraplan  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES calligraplan_shell.rc calligraplan.rc calligraplan_readonly.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/calligraplan)
install( PROGRAMS  org.kde.calligraplan.desktop  DESTINATION ${KDE_INSTALL_APPDIR})
install( FILES  calligraplanrc DESTINATION ${KDE_INSTALL_CONFDIR})
install(FILES calligraplansettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES org.kde.calligraplan.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})



#add_custom_target(apidox doc/api/gendocs.pl WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
