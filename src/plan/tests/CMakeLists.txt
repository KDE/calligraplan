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
    LINK_LIBRARIES calligraplanprivate calligraplankernel calligraplanmain Qt5::Test
)

########## next target ###############

#plan_add_unit_test(TJSchedulerTester
    #TJSchedulerTester.cpp TestSchedulerPluginLoader.cpp
    #LINK_LIBRARIES calligraplanprivate calligraplankernel calligraplanmain calligraplanplugin Qt5::Test
#)
