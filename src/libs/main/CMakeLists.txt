if (BUILD_TESTING)
    add_subdirectory( tests )
endif ()

kde_enable_exceptions()

if (APPLE)
    find_library(FOUNDATION_LIBRARY Foundation)
endif ()

include_directories(${PLANMAIN_INCLUDES} ${PLANPLUGIN_INCLUDES})

########### next target ###############

if( Qca-qt6_FOUND )
   add_definitions( -DQCA2 )
endif()

set(komain_LIB_SRCS
    ktoolbarhelper.cpp
    kedittoolbar.cpp
    WelcomeView.cpp

    KoAutoSaveRecoveryDialog.cpp
    KoApplication.cpp
    KoComponentData.cpp
    KoDockFactoryBase.cpp
    KoDockRegistry.cpp
    KoDocument.cpp
    KoDocumentEntry.cpp
    KoMainWindow.cpp
    KoPrintingDialog.cpp
    KoPrintJob.cpp
    KoView.cpp
    KoFilterManager.cpp
    KoFilterChain.cpp
    KoFilter.cpp
    KoFilterEntry.cpp
    KoFilterManager_p.cpp
    KoFilterVertex.cpp
    KoFilterGraph.cpp
    KoFilterEdge.cpp
    KoFilterChainLink.cpp
    KoFilterChainLinkList.cpp
    KoUndoStackAction.cpp
    config/KoConfigDocumentPage.cpp
    config/ConfigDocumentationPanel.cpp
#     KoFindBase.cpp
#     KoFindMatch.cpp
#     KoFindOption.cpp
#     KoFindOptionSet.cpp

    KoPart.cpp

    MainDebug.cpp
)

if( Qt6DBus_FOUND )
    set(komain_LIB_SRCS ${komain_LIB_SRCS}
        KoApplicationAdaptor.cpp
        KoViewAdaptor.cpp
        KoPartAdaptor.cpp
    )
endif()

ki18n_wrap_ui(komain_LIB_SRCS
    WelcomeView.ui
    config/KoConfigDocumentPage.ui
    config/ConfigDocumentationPanel.ui
)

add_library(calligraplanmain SHARED  ${komain_LIB_SRCS})
generate_export_header(calligraplanmain EXPORT_FILE_NAME komain_generated_export.h BASE_NAME komain
)

target_link_libraries(calligraplanmain
    PUBLIC
        calligraplanwidgets
        calligraplankundo2
        KF6::CoreAddons
    PRIVATE
        calligraplanplugin
        KF6::Notifications
        KF6::ItemViews
        KF6::KIOFileWidgets
        KF6::IconThemes
        KF6::IconWidgets
        KF6::DBusAddons
)

if( PlasmaActivities_FOUND )
    target_link_libraries(calligraplanmain PRIVATE Plasma::Activities)
endif()

if (APPLE)
    target_link_libraries(calligraplanmain PRIVATE ${FOUNDATION_LIBRARY})
endif ()

set_target_properties(calligraplanmain PROPERTIES
    VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)
install(TARGETS calligraplanmain ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

# TODO: with the new embedded JSON data for plugins there is no schema ATM to define extended properties
# calligradocker.desktop

if (APPLE)
    install(FILES osx.stylesheet DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan )
endif ()

