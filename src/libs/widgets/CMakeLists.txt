if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

include_directories(${PLANODF_INCLUDES})
include_directories(${PLANWIDGETS_INCLUDE})

set(kowidgets_LIB_SRCS
    ParameterSubstitutionDialog.cpp
    KoResourcePaths.cpp
    KoVBox.cpp
    KoDialog.cpp
    KoGlobal.cpp
    KoPageLayoutDialog.cpp
    KoPageLayoutWidget.cpp
    KoPagePreviewWidget.cpp
    KoUnitDoubleSpinBox.cpp
    KoDpi.cpp
    KoDockWidgetTitleBar.cpp
    KoDockWidgetTitleBarButton.cpp
    KoDocumentInfoDlg.cpp
    WidgetsDebug.cpp
)

ki18n_wrap_ui( kowidgets_LIB_SRCS
    ParameterSubstitutionPanel.ui
    koDocumentInfoAboutWidget.ui
    koDocumentInfoAuthorWidget.ui

    KoPageLayoutWidget.ui
)


add_library(calligraplanwidgets SHARED ${kowidgets_LIB_SRCS})
generate_export_header(calligraplanwidgets BASE_NAME kowidgets)

target_link_libraries(calligraplanwidgets
    PUBLIC
        calligraplanodf
        calligraplanwidgetutils
        KF6::KIOWidgets
        KF6::ConfigWidgets
        Qt6::PrintSupport
        KF6::XmlGui
    PRIVATE
        KF6::GuiAddons
        KF6::WidgetsAddons
        KF6::ConfigCore
#         KF6::Codecs
#         KF6::Completion
        KF6::IconThemes
        KF6::I18n
)

if(GHNS)
    target_link_libraries(calligraplanwidgets PRIVATE KF6::NewStuff)
endif ()

if(X11_FOUND)
    target_link_libraries(calligraplanwidgets PRIVATE ${X11_LIBRARIES})
endif()

set_target_properties(calligraplanwidgets PROPERTIES
    VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)

install(TARGETS calligraplanwidgets  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

