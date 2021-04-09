set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( .. ${PLAN_INCLUDES})

# call: plan_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLAN_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "${_TEST_NAME}"
        NAME_PREFIX "plan-"
    )
endmacro()


########## next target ###############

plan_add_unit_test(InsertProjectTester
    InsertProjectTester.cpp
    LINK_LIBRARIES planprivate plankernel planmain Qt5::Test
)
