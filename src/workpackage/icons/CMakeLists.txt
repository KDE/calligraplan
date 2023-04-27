file(GLOB calligraplanwork_APPICONS "*-apps-calligraplanwork.*")
file(GLOB calligraplanwork_MIMEICONS "*-mimetypes-*.*")

ecm_install_icons(ICONS ${calligraplanwork_APPICONS} ${calligraplanwork_MIMEICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
