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
        KF5::KIOWidgets
        KF5::ConfigWidgets
        Qt5::PrintSupport
        KF5::XmlGui
    PRIVATE
        KF5::GuiAddons
        KF5::WidgetsAddons
        KF5::ConfigCore
#         KF5::Codecs
#         KF5::Completion
        KF5::IconThemes
        KF5::I18n
)

if(GHNS)
    target_link_libraries(calligraplanwidgets PRIVATE KF5::NewStuff)
endif ()

if(X11_FOUND)
    target_link_libraries(calligraplanwidgets PRIVATE Qt5::X11Extras ${X11_LIBRARIES})
endif()

set_target_properties(calligraplanwidgets PROPERTIES
    VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)

install(TARGETS calligraplanwidgets  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

