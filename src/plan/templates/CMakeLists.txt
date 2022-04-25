add_subdirectory( Simple )

# Special templates, should never be seen by user
install(FILES SharedResources.plant
    DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan/templates/.source
)
