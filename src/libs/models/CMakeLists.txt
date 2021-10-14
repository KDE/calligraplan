
include_directories(
    ${PLANKERNEL_INCLUDES}
    ${PLANWIDGETS_INCLUDES}
    ${PLANODF_INCLUDES}
    ${PLANKUNDO2_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
)

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()

if (KF5Contacts_FOUND)
    add_definitions(-DPLAN_KCONTACTS_FOUND)
endif ()

########### KPlato data models library ###############

set(calligraplanmodels_LIB_SRCS
    reportgenerator/ReportGenerator.cpp
    reportgenerator/ReportGeneratorOdt.cpp

    kptcommonstrings.cpp
    kpttreecombobox.cpp

    commands/InsertProjectXmlCommand.cpp
    commands/InsertTaskModuleCommand.cpp

    kcalendar/kdatetable.cpp
    kcalendar/kdatepicker.cpp

    RequieredResourceDelegate.cpp
    AlternativeResourceDelegate.cpp
    kptitemmodelbase.cpp

    kptnodechartmodel.cpp
    
    kptflatproxymodel.cpp

    kptrelationmodel.cpp
    kptworkpackagemodel.cpp
    kptdocumentmodel.cpp
    kptnodeitemmodel.cpp
    kptdurationspinbox.cpp
    kpttaskstatusmodel.cpp
    kptcalendarmodel.cpp
    kptschedulemodel.cpp
    kptaccountsmodel.cpp
    kptpertcpmmodel.cpp

    AllocatedResourceItemModel.cpp
    ResourceGroupModel.cpp
    ResourceModel.cpp
    ResourceGroupItemModel.cpp
    ResourceItemModel.cpp
    kptresourceappointmentsmodel.cpp
    kptresourceallocationmodel.cpp
    ResourceItemSFModel.cpp
#     GroupAllocationItemModel.cpp

    kpttaskcompletedelegate.cpp
)

add_library(calligraplanmodels SHARED ${calligraplanmodels_LIB_SRCS})
generate_export_header(calligraplanmodels BASE_NAME planmodels)

target_link_libraries(calligraplanmodels
    PUBLIC
        calligraplankernel
        calligraplanodf
        KChart
        KGantt
        KF5::KIOWidgets
        KF5::ItemModels
    PRIVATE
        Qt5::Gui
        KF5::Notifications
        KF5::TextWidgets
        calligraplanwidgets
)
if(KF5Contacts_FOUND)
    target_link_libraries(calligraplanmodels  PRIVATE KF5::Contacts)
endif()

set_target_properties(calligraplanmodels PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS calligraplanmodels ${INSTALL_TARGETS_DEFAULT_ARGS})

