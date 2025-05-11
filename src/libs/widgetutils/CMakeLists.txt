
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
        Qt6::Widgets
        Qt6::Gui
        Qt6::Xml
        Qt6::Core
    PRIVATE
        KF6::I18n
        KF6::ConfigCore
        KF6::ConfigWidgets
)

set_target_properties(calligraplanwidgetutils
    PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)

install(TARGETS calligraplanwidgetutils ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
