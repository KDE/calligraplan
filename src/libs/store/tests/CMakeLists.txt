include_directories( ${PLANODF_INCLUDES} )

# call: planstore_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLANSTORE_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "Plan${_TEST_NAME}"
        NAME_PREFIX "plan-store-"
    )
endmacro()

########### next target ###############

set(lzftest_SRCS ../KoLZF.cpp TestKoLZF.cpp )
planstore_add_unit_test(TestKoLZF ${lzftest_SRCS}  LINK_LIBRARIES Qt5::Test)

########### next target ###############

set(xmlvectortest_SRCS ../KoLZF.cpp TestKoXmlVector.cpp )
planstore_add_unit_test(TestKoXmlVector ${xmlvectortest_SRCS}  LINK_LIBRARIES Qt5::Test)

########### next target ###############

set(storedroptest_SRCS storedroptest.cpp )
add_executable(Planstoredroptest ${storedroptest_SRCS})
ecm_mark_as_test(Planstoredroptest)
target_link_libraries(Planstoredroptest calligraplanstore KF5::I18n Qt5::Widgets)


