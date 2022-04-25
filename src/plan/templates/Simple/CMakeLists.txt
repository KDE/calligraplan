install( FILES
        Plain.plant
        8HourDay-40HourWeek.plant
#         Basicproject.plant
    DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan/templates/Simple/.source
)
install( FILES
        .directory
        Plain.desktop
        8HourDay-40HourWeek.desktop
#         Basicproject.desktop
    DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan/templates/Simple
)

ecm_install_icons( ICONS
    48-actions-template_timechart.png
    sc-actions-template_timechart.svgz

    DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan/icons
    THEME hicolor
)
