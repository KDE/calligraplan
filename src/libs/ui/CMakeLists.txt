

include_directories(
    ${PLANKERNEL_INCLUDES}
    ${PLANMODELS_INCLUDES}
    ${PLANMAIN_INCLUDES}
    ${PLANWIDGETS_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gantt
)

#add_subdirectory( tests )

########### KPlato private library ###############

if (PLAN_USE_KREPORT)
    message(STATUS "-- Building plan with reports capability")
    add_subdirectory(reports/items)

    set(planreports_LIB_SRC
        reports/reportview.cpp
        reports/reportdata.cpp
        reports/reportsourceeditor.cpp
        reports/reportscripts.cpp
    )
    set(planreports_ui_LIB_SRCS
        reports/reportsourceeditor.ui
        reports/reportnavigator.ui
        reports/reportsectionswidget.ui
        reports/reportgroupsectionswidget.ui
        reports/reporttoolswidget.ui
    )

endif()

set(planui_LIB_SRCS
    ${planreports_LIB_SRC}

    RelationEditorDialog.cpp
    TasksEditController.cpp
#     GroupAllocationPanel.cpp
    TasksEditDialog.cpp

    RichTextWidget.cpp

    reportsgenerator/ReportsGeneratorView.cpp

    gantt/kptganttitemdelegate.cpp
    gantt/DateTimeTimeLine.cpp
    gantt/DateTimeGrid.cpp
    gantt/GanttViewBase.cpp
    kptganttview.cpp

    kptworkpackagesendpanel.cpp
    kptworkpackagesenddialog.cpp
    kptdocumentseditor.cpp
    kptdocumentspanel.cpp
    kptdocumentsdialog.cpp
    
    kptitemviewsettup.cpp
    kptsplitterview.cpp
    kptrelationeditor.cpp
    kptdependencyeditor.cpp
    kptusedefforteditor.cpp
    kpttaskstatusview.cpp
    kptcalendareditor.cpp
    kptviewbase.cpp
    kptaccountseditor.cpp
    kptperteditor.cpp
    kptpertresult.cpp
    kpttaskeditor.cpp
    kptscheduleeditor.cpp
    kptsummarytaskdialog.cpp
    kptsummarytaskgeneralpanel.cpp
    kptresourceappointmentsview.cpp
    kptaccountsviewconfigdialog.cpp
    kptaccountsview.cpp
    kpttaskcostpanel.cpp
    kptmilestoneprogresspanel.cpp
    kptmilestoneprogressdialog.cpp
    kpttaskdialog.cpp
    kptmainprojectdialog.cpp
    kptmainprojectpanel.cpp
    kptrelationdialog.cpp
    kptrequestresourcespanel.cpp
    kptresourcedialog.cpp
    kptstandardworktimedialog.cpp
    kptintervaledit.cpp
    kpttaskgeneralpanel.cpp
    kpttaskprogresspanel.cpp
    kpttaskprogressdialog.cpp
    kpttaskdescriptiondialog.cpp
    kptwbsdefinitiondialog.cpp
    kptwbsdefinitionpanel.cpp
    kptresourceallocationeditor.cpp
    kptworkpackagemergedialog.cpp
    kptrecalculatedialog.cpp

    locale/localemon.cpp
    kptlocaleconfigmoneydialog.cpp

    ResourceGroupDocker.cpp
    ResourceGroupEditor.cpp
    ResourceAllocationView.cpp
    kptresourceeditor.cpp

    performance/KPlatoChart.cpp
    performance/PerformanceStatusBase.cpp
    performance/ProjectStatusView.cpp
    performance/PerformanceStatusView.cpp
    performance/PerformanceTableView.cpp

    ResourceCoverageView.cpp
)

ki18n_wrap_ui(planui_LIB_SRCS
    ${planreports_ui_LIB_SRCS}

    RelationEditorDialog.ui

    kptresourceappointmentsdisplayoptions.ui

    gantt/kptganttchartdisplayoptions.ui
    gantt/kptganttprintingoptions.ui

    kptprintingheaderfooter.ui

    kptworkpackagesendpanel.ui
    
    kptdocumentspanel.ui
    
    performance/PerformanceStatus.ui
    performance/PerformanceStatusViewSettingsPanel.ui
    
    kptcpmwidget.ui
    kptitemviewsettings.ui
    kptpertresult.ui
    standardworktimedialogbase.ui
    kptwbsdefinitionpanelbase.ui
    kptaccountsviewconfigurepanelbase.ui
    kptintervaleditbase.ui
    kpttaskcostpanelbase.ui
    kpttaskdescriptionpanelbase.ui
    kptsummarytaskgeneralpanelbase.ui
    kptmilestoneprogresspanelbase.ui
    resourcedialogbase.ui
    kptmainprojectpanelbase.ui
    relationpanel.ui
    kpttaskgeneralpanelbase.ui
    kpttaskprogresspanelbase.ui
    kptperteditor.ui
    kpttaskstatusviewsettingspanel.ui
    kptworkpackagemergepanel.ui
    kptrecalculatedialog.ui
    kptscheduleeditor.ui
    ResourceAllocationPanel.ui

    locale/localemon.ui
)

add_library(planui SHARED ${planui_LIB_SRCS})
generate_export_header(planui)

target_link_libraries(planui
    PUBLIC
        planmain
        planmodels
        KF5::TextWidgets
    PRIVATE
        KChart
        KF5::ItemViews
        KF5::IconThemes
        KF5::Archive
        KF5::KIOCore
        KF5::KIOFileWidgets
        KF5::KIOWidgets
        KF5::KIONTLM
)
if (PLAN_USE_KREPORT)
    target_link_libraries(planui PUBLIC KReport PRIVATE KPropertyWidgets)
endif()

if(KF5AkonadiContact_FOUND)
    target_link_libraries(planui  PRIVATE KF5::AkonadiContact)
endif()

set_target_properties(planui PROPERTIES VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION} )

install(TARGETS planui ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES
        AccountsEditorUi.rc
        AccountsViewUi.rc
        CalendarEditorUi.rc
        TaskEditorUi.rc
        DependencyEditorUi.rc
        ResourceEditorUi.rc
        ResourceGroupEditorUi.rc
        ResourceAppointmentsViewUi.rc
        ResourceAppointmentsGanttViewUi.rc
        ScheduleEditorUi.rc
        GanttViewUi.rc
        MilestoneGanttViewUi.rc
        WorkPackageViewUi.rc
        WorkPackageViewUi_readonly.rc
        reportsgenerator/ReportsGeneratorViewUi.rc
        performance/PerformanceStatusViewUi.rc
        performance/ProjectStatusViewUi.rc
        PertResultUi.rc
        TaskViewUi.rc
        TaskStatusViewUi.rc
    DESTINATION ${KXMLGUI_INSTALL_DIR}/calligraplan
)

# reports files
install(FILES
        reportsgenerator/templates/EmptyTemplate.odt
        reportsgenerator/templates/ProjectPerformanceCost.odt
        reportsgenerator/templates/ProjectPerformanceEffort.odt
        reportsgenerator/templates/TaskStatus.odt
    DESTINATION ${DATA_INSTALL_DIR}/calligraplan/reports
)
