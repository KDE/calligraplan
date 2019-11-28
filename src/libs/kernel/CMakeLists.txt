
include_directories(
    ${PLANKERNEL_INCLUDES}
    ${PLANKUNDO2_INCLUDES}
    ${PLANSTORE_INCLUDES}
)


########### KPlato kernel library ###############

set(plankernel_LIB_SRCS
    kptglobal.cpp
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
    kptdebug.cpp

    commands/NamedCommand.cpp
    commands/MacroCommand.cpp
    commands/SetTaskModulesCommand.cpp
    commands/AddResourceCmd.cpp
    commands/RemoveResourceCmd.cpp
    commands/AddParentGroupCmd.cpp
    commands/RemoveParentGroupCmd.cpp

    kptschedulerplugin.cpp

    kptconfigbase.cpp

    KPlatoXmlLoaderBase.cpp
)

add_library(plankernel SHARED ${plankernel_LIB_SRCS})
generate_export_header(plankernel)

target_link_libraries(plankernel
    PUBLIC
        plankundo2
        planstore
        planwidgetutils
#         KF5::I18n
#         KF5::CoreAddons
)
if(KF5Holidays_FOUND)
    target_link_libraries(plankernel PUBLIC KF5::Holidays)
endif()

set_target_properties(plankernel PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS plankernel ${INSTALL_TARGETS_DEFAULT_ARGS})

# TODO: with the new embedded JSON data for plugins there is no schema ATM to define extended properties
# plan_schedulerplugin.desktop

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()
