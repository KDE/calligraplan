remove_definitions(-DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_ASCII -DQT_RESTRICTED_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

include_directories( .. ${PLAN_INCLUDES} ../workpackage)

# call: plan_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLAN_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "${_TEST_NAME}"
        NAME_PREFIX "plan-"
    )
endmacro()


plan_add_unit_test(WorkPackageTester
    WorkPackageTester.cpp
    LINK_LIBRARIES calligraplanworkprivate calligraplanprivate calligraplankernel calligraplanmain Qt::Test
)
