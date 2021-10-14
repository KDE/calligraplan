if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

########### libkostore ###############

set(calligraplanstore_LIB_SRCS
    KoDirectoryStore.cpp
    KoLZF.cpp
    KoStore.cpp
    KoStoreDevice.cpp
    KoTarStore.cpp
    KoXmlNS.cpp
    KoXmlReader.cpp
    KoXmlWriter.cpp
    KoZipStore.cpp
    StoreDebug.cpp
    KoNetAccess.cpp # temporary while porting
)
if( Qca-qt5_FOUND )
    set(calligraplanstore_LIB_SRCS ${calligraplanstore_LIB_SRCS}
        KoEncryptedStore.cpp
        KoEncryptionChecker.cpp
    )
endif()

add_library(calligraplanstore SHARED ${calligraplanstore_LIB_SRCS})
generate_export_header(calligraplanstore BASE_NAME kostore)

target_link_libraries(calligraplanstore
    PUBLIC
        Qt5::Xml
        Qt5::Core
        KF5::KIOCore
    PRIVATE
        Qt5::Gui
        KF5::Archive
        KF5::KIOWidgets
        KF5::I18n
)
if( Qca-qt5_FOUND )
    target_link_libraries(calligraplanstore PRIVATE qca-qt5 KF5::Wallet)
endif()

set_target_properties(calligraplanstore PROPERTIES
    VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)
install(TARGETS calligraplanstore ${INSTALL_TARGETS_DEFAULT_ARGS} )
