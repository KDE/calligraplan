remove_definitions(-DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_ASCII -DQT_RESTRICTED_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

include_directories(
    ${PLANKERNEL_INCLUDES}
    ${PLANWIDGETUTILS_INCLUDES}
)

# call: plankernel_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLANKERNEL_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "${_TEST_NAME}"
        NAME_PREFIX "plan-kernel-"
    )
endmacro()


plankernel_add_unit_test(DateTimeTester DateTimeTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(DurationTester DurationTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(AppointmentIntervalTester AppointmentIntervalTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(EstimateTester EstimateTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(CalendarTester CalendarTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(ResourceTester ResourceTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(ResourceGroupTester ResourceGroupTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(XmlLoaderTester XmlLoaderTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(CommandsTester CommandsTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(WorkInfoCacheTester WorkInfoCacheTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(ScheduleTester ScheduleTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(AccountsTester AccountsTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(AccountsCommandTester AccountsCommandTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(RelationTester RelationTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(ProjectTester ProjectTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(PerformanceTester PerformanceTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(ReScheduleTester ReScheduleTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)

plankernel_add_unit_test(AlternativeRequestTester AlternativeRequestTester.cpp  LINK_LIBRARIES calligraplankernel Qt6::Test)
