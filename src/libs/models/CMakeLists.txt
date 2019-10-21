
if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

include_directories(
    ${PLANKERNEL_INCLUDES}
    ${PLANWIDGETS_INCLUDES}
    ${PLANODF_INCLUDES}
    ${PLANKUNDO2_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
)

if (KF5Contacts_FOUND)
    add_definitions(-DPLAN_KCONTACTS_FOUND)
endif ()

########### KPlato data models library ###############

set(planmodels_LIB_SRCS
    reportgenerator/ReportGenerator.cpp
    reportgenerator/ReportGeneratorOdt.cpp

    kptcommonstrings.cpp
    kpttreecombobox.cpp

    commands/InsertProjectXmlCommand.cpp
    commands/InsertTaskModuleCommand.cpp

    kcalendar/kdatetable.cpp
    kcalendar/kdatepicker.cpp
    
    kptnodechartmodel.cpp
    
    kptflatproxymodel.cpp

    kptrelationmodel.cpp
    kptworkpackagemodel.cpp
    kptdocumentmodel.cpp
    kptitemmodelbase.cpp
    kptnodeitemmodel.cpp
    kptdurationspinbox.cpp
    kpttaskstatusmodel.cpp
    kptresourcemodel.cpp
    kptcalendarmodel.cpp
    kptschedulemodel.cpp
    kptaccountsmodel.cpp
    kptpertcpmmodel.cpp
    kptresourceappointmentsmodel.cpp
    kptresourceallocationmodel.cpp

    kpttaskcompletedelegate.cpp
)

add_library(planmodels SHARED ${planmodels_LIB_SRCS})
generate_export_header(planmodels)

target_link_libraries(planmodels
    PUBLIC
        plankernel
        planodf
        KChart
        KGantt
        KF5::KIOWidgets
    PRIVATE
        Qt5::Gui
        KF5::Notifications
        KF5::TextWidgets
        planwidgets
)
if(KF5Contacts_FOUND)
    target_link_libraries(planmodels  PRIVATE KF5::Contacts)
endif()

set_target_properties(planmodels PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS planmodels ${INSTALL_TARGETS_DEFAULT_ARGS})

