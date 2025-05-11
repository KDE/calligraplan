include_directories( .. ../commands ../../kernel ${KOODF_INCLUDES} ${KUNDO2_INCLUDES})

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

# call: planmodels_add_unit_test(<test-name> <sources> LINK_LIBRARIES <library> [<library> [...]] [GUI])
macro(PLANMODELS_ADD_UNIT_TEST _TEST_NAME)
    ecm_add_test( ${ARGN}
        TEST_NAME "${_TEST_NAME}"
        NAME_PREFIX "plan-models-"
    )
endmacro()

########### next target ###############

planmodels_add_unit_test(ResourceGroupModelTester ResourceGroupModelTester.cpp  LINK_LIBRARIES calligraplanmodels Qt6::Test)

########### next target ###############

planmodels_add_unit_test(ResourceModelTester ResourceModelTester.cpp  LINK_LIBRARIES calligraplanmodels Qt6::Test)

########## next target ###############

planmodels_add_unit_test(FlatProxyModelTester FlatProxyModelTester.cpp  LINK_LIBRARIES calligraplanmodels Qt6::Test)

########## next target ###############

planmodels_add_unit_test(WorkPackageProxyModelTester WorkPackageProxyModelTester.cpp  LINK_LIBRARIES calligraplanmodels Qt6::Test)

########## next target ###############

planmodels_add_unit_test(InsertProjectXmlCommandTester InsertProjectXmlCommandTester.cpp  LINK_LIBRARIES calligraplanmodels Qt6::Test)
