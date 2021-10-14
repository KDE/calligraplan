include_directories(${PLANKUNDO2_INCLUDES})

set(kundo2_LIB_SRCS
	kundo2stack.cpp
	kundo2group.cpp
	kundo2view.cpp
	kundo2model.cpp
	kundo2magicstring.cpp
	kundo2commandextradata.cpp
)

add_library(calligraplankundo2 SHARED ${kundo2_LIB_SRCS})
generate_export_header(calligraplankundo2 BASE_NAME kundo2)

target_link_libraries(calligraplankundo2
    PUBLIC
        KF5::I18n
        KF5::XmlGui
        Qt5::Core
        Qt5::Widgets
    PRIVATE
        KF5::ConfigWidgets
)

set_target_properties(calligraplankundo2 PROPERTIES
    VERSION ${GENERIC_PLAN_LIB_VERSION} SOVERSION ${GENERIC_PLAN_LIB_SOVERSION}
)
install(TARGETS calligraplankundo2 ${INSTALL_TARGETS_DEFAULT_ARGS})
