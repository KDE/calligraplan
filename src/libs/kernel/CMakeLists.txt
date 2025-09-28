
include_directories(
    ${PLANKERNEL_INCLUDES}
    ${PLANKUNDO2_INCLUDES}
    ${PLANSTORE_INCLUDES}
    ${PLANMAIN_INCLUDES}
)


########### KPlato kernel library ###############

set(calligraplankernel_LIB_SRCS
    kptglobal.cpp
    Completion.cpp
    WorkPackage.cpp
    kptlocale.cpp
    kpteffortcostmap.cpp
    kptdocuments.cpp
    kptaccount.cpp
    kptappointment.cpp
    kptnode.cpp
    kptproject.cpp
    kptrelation.cpp
    ResourceGroup.cpp
    Resource.cpp
    kptresourcerequest.cpp
    kpttask.cpp
    kptduration.cpp
    kptdatetime.cpp
    kptcalendar.cpp
    kptschedule.cpp
    kptwbsdefinition.cpp
    kptcommand.cpp
    kptpackage.cpp
    kptxmlloaderobject.cpp
    kptdebug.cpp

    commands/NamedCommand.cpp
    commands/MacroCommand.cpp
    commands/SetTaskModulesCommand.cpp
    commands/AddResourceCmd.cpp
    commands/RemoveResourceCmd.cpp
    commands/AddParentGroupCmd.cpp
    commands/RemoveParentGroupCmd.cpp
    commands/AddParentGroupCmd.cpp
    commands/RemoveParentGroupCmd.cpp
    commands/ModifyResourceRequestAlternativeCmd.cpp
    commands/ResourceGroupModifyCoordinatorCmd.cpp
    commands/AddAlternativeRequestCmd.cpp
    commands/InsertProjectCmd.cpp
    commands/ProjectModifyTimeZoneCmd.cpp
    commands/SetFreedaysCalendarCmd.cpp
    commands/ModifyResourceOriginCmd.cpp
    commands/ModifyResourceGroupOriginCmd.cpp
    commands/ModifyCalendarOriginCmd.cpp
    commands/ModifyScheduleManagerOwnerCmd.cpp

    kptschedulerplugin.cpp

    kptconfigbase.cpp

    SchedulingContext.cpp
    ProjectLoader_v0.cpp
    KPlatoXmlLoaderBase.cpp
)

add_library(calligraplankernel SHARED ${calligraplankernel_LIB_SRCS})
generate_export_header(calligraplankernel BASE_NAME plankernel)

target_link_libraries(calligraplankernel
    PUBLIC
        calligraplankundo2
        calligraplanstore
        calligraplanwidgetutils
        Qt::PrintSupport
#         KF5::I18n
#         KF5::CoreAddons
)
if(KF6Holidays_FOUND)
    target_link_libraries(calligraplankernel PUBLIC KF6::Holidays)
endif()

set_target_properties(calligraplankernel PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS calligraplankernel ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# TODO: with the new embedded JSON data for plugins there is no schema ATM to define extended properties
# plan_schedulerplugin.desktop

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()
