include_directories(..)

# call: planwidgetutils_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLANWIDGETUTILS_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "Plan${_TEST_NAME}"
        NAME_PREFIX "plan-widgetutils-"
    )
endmacro()

########### next target ###############

set(kofiledialogtester_SRCS KoFileDialogTester.cpp)
qt5_wrap_ui(kofiledialogtester_SRCS KoFileDialogTester.ui)
add_executable(Plankofiledialogtester ${kofiledialogtester_SRCS})
target_link_libraries(Plankofiledialogtester calligraplanwidgetutils)
