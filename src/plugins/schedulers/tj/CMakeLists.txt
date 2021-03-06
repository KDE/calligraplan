add_definitions(-DTRANSLATION_DOMAIN=\"calligraplan_scheduler_tj\")

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

add_library(plantjscheduler SHARED ${TJScheduler_LIB_SRCS})
generate_export_header(plantjscheduler BASE_NAME plantj)
target_link_libraries(plantjscheduler PUBLIC plankernel planmain)

# plugin
add_library(plantjplugin MODULE PlanTJPlugin.cpp )
# calligraplan_scheduler_desktop_to_json(plantjscheduler plantjscheduler.desktop)
kcoreaddons_desktop_to_json(plantjplugin plantjscheduler.desktop
        SERVICE_TYPES ${PLAN_SOURCE_DIR}/libs/kernel/plan_schedulerplugin.desktop
    )

#add_definitions(-Dplantjscheduler_EXPORTS)
target_link_libraries(plantjplugin PUBLIC plantjscheduler)

set_target_properties( plantjscheduler PROPERTIES DEFINE_SYMBOL MAKE_PLANTJ_LIB )

install(TARGETS plantjscheduler DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS plantjplugin DESTINATION ${PLUGIN_INSTALL_DIR}/calligraplan/schedulers)
