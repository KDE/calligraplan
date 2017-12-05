set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories(
    ${PLANKERNEL_INCLUDES}
    ${PLAN_SOURCE_DIR}/libs/widgetutils
    ${PLANODF_INCLUDES}
    ${PLANKUNDO2_INCLUDES}
)

# call: plankernel_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLANKERNEL_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "${_TEST_NAME}"
        NAME_PREFIX "plan-kernel-"
    )
endmacro()

########### next target ###############

plankernel_add_unit_test(DateTimeTester DateTimeTester.cpp  LINK_LIBRARIES kplatokernel Qt5::Test)

########## next target ###############

plankernel_add_unit_test(DurationTester DurationTester.cpp  LINK_LIBRARIES kplatokernel Qt5::Test)

########## next target ###############

plankernel_add_unit_test(AppointmentIntervalTester AppointmentIntervalTester.cpp  LINK_LIBRARIES kplatokernel Qt5::Test)

########## next target ###############

plankernel_add_unit_test(EstimateTester EstimateTester.cpp  LINK_LIBRARIES kplatokernel Qt5::Test)

########### next target ###############

plankernel_add_unit_test(CalendarTester CalendarTester.cpp  LINK_LIBRARIES kplatokernel Qt5::Test)

########## next target ###############

plankernel_add_unit_test(ResourceTester ResourceTester.cpp  LINK_LIBRARIES kplatokernel plankundo2 planodf Qt5::Test)

########### next target ###############

plankernel_add_unit_test(ScheduleTester ScheduleTester.cpp  LINK_LIBRARIES kplatokernel Qt5::Test)

########### next target ###############

plankernel_add_unit_test(ProjectTester ProjectTester.cpp  LINK_LIBRARIES planprivate kplatokernel Qt5::Test)

########### next target ###############

plankernel_add_unit_test(AccountsTester AccountsTester.cpp  LINK_LIBRARIES planprivate kplatokernel Qt5::Test)

########### next target ###############

plankernel_add_unit_test(AccountsCommandTester AccountsCommandTester.cpp  LINK_LIBRARIES kplatokernel planmain Qt5::Test)

########### next target ###############

plankernel_add_unit_test(PerformanceTester PerformanceTester.cpp  LINK_LIBRARIES planprivate kplatokernel Qt5::Test)

########### next target ###############

plankernel_add_unit_test(CommandsTester CommandsTester.cpp  LINK_LIBRARIES planprivate kplatokernel plankundo2 Qt5::Test)

########### next target ###############

plankernel_add_unit_test(WorkInfoCacheTester WorkInfoCacheTester.cpp  LINK_LIBRARIES planprivate kplatokernel Qt5::Test)