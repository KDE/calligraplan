remove_definitions(-DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_ASCII -DQT_RESTRICTED_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

include_directories(
    ${PLAN_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
)
#message(STATUS "PLAN_INCLUDES: ${PLAN_INCLUDES}")

#TODO: Howto properly find plugins dir?
add_definitions(-DSCHEDULERPLUGINS_DIR="${CMAKE_BINARY_DIR}/bin")

include_directories( .. ../taskjuggler ${PLAN_INCLUDES} ${KOSTORE_INCLUDES})

# call: planschedulers_tj_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLANSCHEDULERS_TJ_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "${_TEST_NAME}"
        NAME_PREFIX "plan-schedulers-tj-"
    )
endmacro()

########### next target ###############

planschedulers_tj_add_unit_test(TaskJugglerTester
    TaskJuggler.cpp
    LINK_LIBRARIES calligraplantjscheduler calligraplanprivate Qt::Test
)

########### next target ###############

planschedulers_tj_add_unit_test(TJTester
    SchedulerTester.cpp
    LINK_LIBRARIES calligraplantjscheduler calligraplanprivate Qt::Test
)

########### next target ###############

planschedulers_tj_add_unit_test(TJSchedulerTester
    TJSchedulerTester.cpp
    LINK_LIBRARIES calligraplantjscheduler calligraplanprivate calligraplanmain Qt::Test
)
