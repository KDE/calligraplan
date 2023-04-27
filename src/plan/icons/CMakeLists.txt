file(GLOB calligraplan_APPICONS "*-apps-calligraplan.*")
file(GLOB calligraplan_MIMEICONS "*-mimetypes-*.*")

ecm_install_icons(ICONS ${calligraplan_APPICONS} ${calligraplan_MIMEICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
