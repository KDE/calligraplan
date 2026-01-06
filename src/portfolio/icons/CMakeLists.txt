file(GLOB calligraplanportfolio_APPICONS "*-apps-calligraplanportfolio.*")
file(GLOB calligraplanportfolio_MIMEICONS "*-mimetypes-*.*")

ecm_install_icons(ICONS ${calligraplanportfolio_APPICONS} ${calligraplanportfolio_MIMEICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
