add_definitions(-DTRANSLATION_DOMAIN=\"calligraplan_scheduler_tj\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)

set(LIBTJ_INCLUDE_DIR taskjuggler)
include_directories(
    ${LIBTJ_INCLUDE_DIR}
    ${PLANODF_INCLUDES}
    ${PLANMAIN_INCLUDES}
    ${PLANKERNEL_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

set ( TJScheduler_LIB_SRCS
    PlanTJScheduler.cpp
    taskjuggler/Allocation.cpp
    taskjuggler/CoreAttributes.cpp
    taskjuggler/CoreAttributesList.cpp
    taskjuggler/Project.cpp
    taskjuggler/Task.cpp
    taskjuggler/TaskDependency.cpp
    taskjuggler/TaskList.cpp
    taskjuggler/TaskScenario.cpp
    taskjuggler/Resource.cpp
    taskjuggler/ResourceList.cpp
    taskjuggler/Scenario.cpp
    taskjuggler/ScenarioList.cpp
    taskjuggler/Shift.cpp
    taskjuggler/ShiftList.cpp
    taskjuggler/ShiftSelection.cpp
    taskjuggler/ShiftSelectionList.cpp
    taskjuggler/VacationList.cpp
    taskjuggler/TjMessageHandler.cpp
    taskjuggler/Utility.cpp
#     taskjuggler/XMLFile.cpp
#     taskjuggler/ParserElement.cpp
#     taskjuggler/ParserNode.cpp
#     taskjuggler/ParserTreeContext.cpp
    taskjuggler/Interval.cpp
)

add_library(calligraplantjscheduler SHARED ${TJScheduler_LIB_SRCS})
generate_export_header(calligraplantjscheduler BASE_NAME plantj)
target_link_libraries(calligraplantjscheduler PUBLIC calligraplankernel calligraplanmain)

# plugin
add_library(calligraplantjplugin MODULE PlanTJPlugin.cpp )

#add_definitions(-Dplantjscheduler_EXPORTS)
target_link_libraries(calligraplantjplugin PUBLIC calligraplantjscheduler)

set_target_properties( calligraplantjscheduler PROPERTIES DEFINE_SYMBOL MAKE_PLANTJ_LIB )

install(TARGETS calligraplantjscheduler DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS calligraplantjplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligraplan/schedulers)
