add_definitions(-DTRANSLATION_DOMAIN=\"krossmoduleplan\")

include_directories( 
    ${PLAN_SOURCE_DIR}
    ${PLAN_INCLUDES}
    ${PLANKROSS_INCLUDES}
)

add_subdirectory( scripts )
if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

set(krossmoduleplan_PART_SRCS
    Account.cpp
    Calendar.cpp
    Node.cpp
    Resource.cpp
    ResourceGroup.cpp
    Schedule.cpp
    Project.cpp
    Module.cpp
    ScriptingPart.cpp
    ScriptingWidgets.cpp
    ScriptingDebug.cpp
)

ki18n_wrap_ui(krossmoduleplan_PART_SRCS
    ScriptingDataQueryView.ui
)

# TODO: plugin should not be SHARED, but MODULE. Needs to be SHARED because tests link to it -> fix with util lib/objects
add_library(krossmodulecalligraplan SHARED ${krossmoduleplan_PART_SRCS})
generate_export_header(krossmodulecalligraplan
    BASE_NAME planscripting
    EXPORT_FILE_NAME planscripting_generated_export.h
)
calligraplan_viewplugin_desktop_to_json(krossmoduleplan planscripting.desktop)

target_link_libraries(krossmodulecalligraplan
    calligraplanprivate
    calligraplankernel
    calligraplanmain
    calligraplankross
)

install(TARGETS krossmodulecalligraplan DESTINATION ${PLUGIN_INSTALL_DIR}/calligraplan/extensions)
install(FILES scripting.rc DESTINATION ${DATA_INSTALL_DIR}/calligraplan/viewplugins)
