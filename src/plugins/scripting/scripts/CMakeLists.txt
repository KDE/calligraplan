install(FILES scripts.rc DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan/scripts)

install(FILES
    resourcesimport.py
    busyinfoexport.py
    busyinfoimport.py
    busyinfoclear.py
    plan2csvexport.py
    plan2odsexport.py
    DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan/scripts/extensions)

install(FILES
    sample_project.py
    sample_busyinfocheck.py
    DESTINATION ${KDE_INSTALL_DATADIR}/calligraplan/scripts/samples)
