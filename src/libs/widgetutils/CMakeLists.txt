
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
        Qt5::Widgets
        Qt5::Gui
        Qt5::Xml
        Qt5::Core
    PRIVATE
        KF5::I18n
        KF5::ConfigCore
        KF5::ConfigWidgets
)

set_target_properties(calligraplanwidgetutils
    PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)

install(TARGETS calligraplanwidgetutils ${INSTALL_TARGETS_DEFAULT_ARGS})
