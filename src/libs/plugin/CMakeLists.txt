include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(koplugin_LIB_SRCS
    KoPluginLoader.cpp
)

add_library(calligraplanplugin SHARED ${koplugin_LIB_SRCS})
generate_export_header(calligraplanplugin BASE_NAME koplugin)

target_link_libraries(calligraplanplugin
    PUBLIC
        Qt::Core
    PRIVATE
        KF6::ConfigCore
        KF6::CoreAddons
)

set_target_properties(calligraplanplugin PROPERTIES
    VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)
install(TARGETS calligraplanplugin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

