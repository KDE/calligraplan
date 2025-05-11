
find_package(ECM ${REQUIRED_ECM_VERSION} QUIET)
if(NOT ECM_FOUND)
    return()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
include(KDEInstallDirs)

macro_optional_find_package(KF6 ${KF_MIN_VERSION} QUIET COMPONENTS DocTools)

if(KF6_FOUND)
  add_subdirectory(plan)
  add_subdirectory(portfolio)
endif()
