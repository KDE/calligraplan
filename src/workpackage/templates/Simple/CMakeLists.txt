install( FILES  Empty.planworkt  DESTINATION ${KDE_INSTALL_DATADIR}/calligraplanwork/templates/Simple/.source)
install( FILES  .directory Empty.desktop DESTINATION ${KDE_INSTALL_DATADIR}/calligraplanwork/templates/Simple)

ecm_install_icons( ICONS
    16-actions-template_empty.png
    22-actions-template_empty.png
    32-actions-template_empty.png
    48-actions-template_empty.png
    64-actions-template_empty.png
    128-actions-template_empty.png

    DESTINATION ${KDE_INSTALL_DATADIR}/calligraplanwork/icons
    THEME hicolor
)
