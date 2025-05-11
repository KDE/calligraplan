
set(PLAN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)

set(PLANPLUGIN_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/plugin
    ${CMAKE_CURRENT_BINARY_DIR}/libs/plugin
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
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/models/reportgenerator
)

set(PLANUI_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ui
    ${CMAKE_CURRENT_BINARY_DIR}/libs/ui
    ${PLAN_SOURCE_DIR}/libs/ui/gantt
)

set(PLAN_INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}/plan

    ${PLANKERNEL_INCLUDES}
    ${PLANMODELS_INCLUDES}
    ${PLANUI_INCLUDES}
    ${PLANMAIN_INCLUDES}
)

# For odf
set(RNG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/devtools/scripts)

if(KF6Holidays_FOUND)
    add_definitions(-DHAVE_KHOLIDAYS)
endif()

# Define plugin install prefix
set (PLAN_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${KDE_INSTALL_PLUGINDIR}")


if (KF6AkonadiContact_FOUND)
    # disable for now: there is a bug
    # it only works if you use kde contacts (of course) but many use other stuff, so gets disappointed
    add_definitions(-DPLAN_KDEPIMLIBS_FOUND)
    message(WARNING "AkonadiContacs available, but function is disabled due to Bug 311940")
endif ()

if (PLANCHARTDEBUG)
    add_definitions(-DPLAN_CHART_DEBUG)
endif ()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

#add_subdirectory(interfaces)
add_subdirectory(servicetypes)

add_subdirectory( toolbar )
add_subdirectory( plugins )

add_subdirectory( libs )

add_subdirectory( plan )
add_subdirectory( workpackage )
add_subdirectory(portfolio)

add_subdirectory(convert)

# FIXME Fix tests on Windows
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_subdirectory(tests)
endif()
