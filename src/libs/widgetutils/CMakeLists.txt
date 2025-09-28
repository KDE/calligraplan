
set(kowidgetutils_LIB_SRCS
    Help.cpp
    KoProgressBar.cpp
    KoProgressUpdater.cpp
    KoUpdater.cpp
    KoUpdaterPrivate_p.cpp
    KoFileDialog.cpp
)


add_library(calligraplanwidgetutils SHARED ${kowidgetutils_LIB_SRCS})
generate_export_header(calligraplanwidgetutils BASE_NAME kowidgetutils)

target_link_libraries(calligraplanwidgetutils
    PUBLIC
        Qt::Widgets
        Qt::Gui
        Qt::Xml
        Qt::Core
    PRIVATE
        KF6::I18n
        KF6::ConfigCore
        KF6::ConfigWidgets
)

set_target_properties(calligraplanwidgetutils
    PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)

install(TARGETS calligraplanwidgetutils ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
