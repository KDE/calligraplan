include_directories( ${PLANMAIN_INCLUDES}  ${PLANWIDGETS_INCLUDES})

add_definitions(-DFILES_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/")
add_definitions(-DFILES_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}")

find_package(KF5ThreadWeaver ${REQUIRED_KF5_VERSION} REQUIRED)

# call: planwidgets_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLANWIDGETS_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "Plan${_TEST_NAME}"
        NAME_PREFIX "plan-widgets-"
    )
endmacro()

########### next target ###############

planwidgets_add_unit_test(KoProgressUpdaterTest KoProgressUpdater_test.cpp  LINK_LIBRARIES calligraplanwidgets KF5::ThreadWeaver Qt5::Test)

########### end ###############
