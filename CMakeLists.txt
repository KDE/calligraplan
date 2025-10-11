if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif ()

# define the version of Plan
# update these version for every release:
# Alpha x.x.89
# Beta: x.x.90 upwards. (Beta 1 = 90, Beta 2 = 91 ...)
# Stable x.x.0 upwards
project(calligraplan VERSION 3.3.89)
set(PLAN_YEAR 2023) # update every year

cmake_minimum_required(VERSION 3.16)
if (POLICY CMP0002)
    cmake_policy(SET CMP0002 NEW)
endif ()
if (POLICY CMP0017)
    cmake_policy(SET CMP0017 NEW)
endif ()
if (POLICY CMP0022)
    cmake_policy(SET CMP0022 NEW)
endif ()
if (POLICY CMP0026)
    cmake_policy(SET CMP0026 NEW)
endif()
if (POLICY CMP0046)
    cmake_policy(SET CMP0046 NEW)
endif ()
if (POLICY CMP0059)
    cmake_policy(SET CMP0059 NEW)
endif()
if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()
if (POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
if (POLICY CMP0115)
    cmake_policy(SET CMP0115 NEW)
endif()

set(PLAN_VERSION_STRING ${PROJECT_VERSION})
set(PLAN_STABLE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(PLAN_STABLE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(PLAN_VERSION_RELEASE ${PROJECT_VERSION_PATCH})
set(PLAN_RELEASE_TYPE "Stable")
if (PROJECT_VERSION_PATCH EQUAL 89)
    set(PLAN_ALPHA 1)
    set(PLAN_RELEASE_TYPE "Alpha")
elseif (PROJECT_VERSION_PATCH GREATER 89)
    set(PLAN_BETA 1)
    math(EXPR PLAN_BETA_VERSION "${PLAN_VERSION_RELEASE} - 89")
    set(PLAN_RELEASE_TYPE "Beta ${PLAN_BETA_VERSION}")
endif()

# Define the generic version of the Calligra Plan libraries here
# This makes it easy to advance it when the next release comes.
# 14 was the last GENERIC_CALLIGRA_LIB_VERSION_MAJOR of the previous Calligra series
# (2.x) so we're starting with 15 in 3.x series.
#if(PLAN_STABLE_VERSION_MAJOR EQUAL 3)
    math(EXPR GENERIC_PLAN_LIB_VERSION_MAJOR "${PLAN_STABLE_VERSION_MAJOR} + 15")
#else()
    ## let's make sure we won't forget to update the "15"
    #message(FATAL_ERROR "Reminder: please update offset == 15 used to compute GENERIC_PLAN_LIB_VERSION_MAJOR to something bigger")
#endif()
set(GENERIC_PLAN_LIB_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(GENERIC_PLAN_LIB_SOVERSION "${PROJECT_VERSION_MAJOR}")

message(STATUS "")
message(STATUS "Build Calligra Plan ${PLAN_VERSION_STRING} (${PLAN_RELEASE_TYPE}) with cmake version: ${CMAKE_VERSION}, required version: ${CMAKE_MINIMUM_REQUIRED_VERSION}")
message(STATUS "")


# ensure out-of-source build
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" isBuildInSource)
if(isBuildInSource)
    message(FATAL_ERROR "Compiling Plan inside the source folder is not possible.\nPlease refer to the build instruction: https://community.kde.org/Calligra/Building/3\nYou need to clean up the source folder from all build artifacts just created, otherwise further building attempts will fail again: With a git repo, you can use \"git clean -df\" in the toplevel source folder (attention! will remove also uncommitted changes to the source code). With sources from a file bundle (like a zip file), delete the source folder and unbundle the sources again.")
endif()


############
#############
## Options ##
#############
############

option(PACKAGERS_BUILD "Build support of multiple CPU architectures in one binary. Should be used by packagers only." ON)

##########################
###########################
## Look for ECM, Qt, KF6 ##
###########################
##########################

set(QT_REQUIRED_VERSION "6.5.0")
set(KF_MIN_VERSION "6.3.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

# ECM KDE macros (include first, to have their policies and settings effect all other macros)
set(KDE_COMPILERSETTINGS_LEVEL 6.0)
include(KDEInstallDirs)
include(KDECMakeSettings NO_POLICY_SCOPE)
include(KDECompilerSettings NO_POLICY_SCOPE)
# CMake macros
include(CMakePackageConfigHelpers)
include(WriteBasicConfigVersionFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckIncludeFile)
include(GenerateExportHeader)
include(FeatureSummary)
# ECM macros
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMMarkAsTest)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)

# Own macros
# This macro is a copy from calligra so only needs to be included when plan is build stand alone
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(MacroOptionalFindPackage)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED
    COMPONENTS
        Archive
#             Codecs
#             Completion
        Config
        ConfigWidgets
        CoreAddons
        DBusAddons
        GuiAddons
        I18n
        IconThemes
        ItemViews
        ItemModels
        JobWidgets
        KIO
        Notifications
#             NotifyConfig
        Parts
        Sonnet
        TextWidgets
        WidgetsAddons
        WindowSystem
        XmlGui
#         Kross
)

find_package(PlasmaActivities)

set_package_properties(PlasmaActivities PROPERTIES
    DESCRIPTION "Library for Activities"
    PURPOSE "Required for activities support (optional)"
    TYPE OPTIONAL
)
if(PlasmaActivities_FOUND)
    set(HAVE_KACTIVITIES TRUE)
endif()

#find_package(KF5KHtml)
#set_package_properties(KF5KHtml PROPERTIES
    #PURPOSE "Required for rich text support"
    #TYPE REQUIRED
#)

find_package(KF6Holidays)
set_package_properties(KF6Holidays PROPERTIES
    DESCRIPTION "Library for generation of public holidays"
    PURPOSE "Required for holidays support (optional)"
    TYPE RECOMMENDED
)

find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED
    COMPONENTS
        Core
        Gui
        PrintSupport
        Test
        Widgets
        Xml
)
find_package(Qt6DBus ${REQUIRED_QT_VERSION} QUIET)
set_package_properties(Qt5DBus PROPERTIES
    DESCRIPTION "Library for dbus access"
    PURPOSE "Required for dbus support (optional)"
    TYPE OPTIONAL
)

find_package(X11)
if(X11_FOUND)
     set(HAVE_X11 TRUE)
else()
     set(HAVE_X11 FALSE)
endif()

##
## Test SharedMimeInfo
##
macro_optional_find_package(SharedMimeInfo 1.3)
set_package_properties(SharedMimeInfo PROPERTIES
    PURPOSE "Required by Calligra Plan Portfolio Manager and Import/Export filters."
    TYPE REQUIRED
)

# use sane compile flags
add_definitions(
-DQT_USE_QSTRINGBUILDER
-DQT_STRICT_ITERATORS
-DQT_NO_SIGNALS_SLOTS_KEYWORDS
-DQT_NO_URL_CAST_FROM_STRING
-DQT_NO_CAST_TO_ASCII
)

# only with this definition will all the FOO_TEST_EXPORT macro do something
# TODO: check if this can be moved to only those places which make use of it,
# to reduce global compiler definitions that would trigger a recompile of
# everything on a change (like adding/removing tests to/from the build)
if(BUILD_TESTING)
    add_definitions(-DCOMPILING_TESTS)
endif()

# overcome some platform incompatibilities
if(WIN32)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/winquirks)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
    set(WIN32_PLATFORM_NET_LIBS ws2_32.lib netapi32.lib)
endif()

###########################
############################
## Required dependencies  ##
############################
###########################

set(REQUIRED_KDIAGRAM_VERSION 3.0.1)

##
## Test for KGantt
##
macro_optional_find_package(KGantt6 ${REQUIRED_KDIAGRAM_VERSION} QUIET)
set_package_properties(KGantt6 PROPERTIES
    DESCRIPTION "Library for creating Gantt diagrams (part of KDiagram)"
    URL "https://www.kde.org/"
    TYPE REQUIRED
)
if(KGantt_FOUND)
    set(KGANTT_VERSION_MAJOR "${KGantt_VERSION_MAJOR}")
    set(KGANTT_VERSION_MINOR "${KGantt_VERSION_MINOR}")
    set(KGANTT_VERSION_PATCH "${KGantt_VERSION_PATCH}")
endif()

##
## Test for KChart
##
macro_optional_find_package(KChart6 ${REQUIRED_KDIAGRAM_VERSION} QUIET)
set_package_properties(KChart6 PROPERTIES
    DESCRIPTION "Library for creating business charts (part of KDiagram)"
    URL "https://www.kde.org/"
    TYPE REQUIRED
)

##
## Test for QCA2
##
macro_optional_find_package(Qca-qt6 2.1.0 QUIET)
set_package_properties(Qca-qt6 PROPERTIES
    DESCRIPTION "Qt Cryptographic Architecture"
    URL "https:/download.kde.org/stable/qca-qt6"
    PURPOSE "Required for encrypted OpenDocument files and encrypted xls files support"
    TYPE OPTIONAL
)

find_package(KF6Wallet ${KF_MIN_VERSION})

if(Qca-qt6_FOUND AND KF6Wallet_FOUND)
    add_definitions( -DQCA2 )
endif()

find_package(Perl REQUIRED)

###########################
############################
## Optional dependencies  ##
############################
###########################

### Remove dependency on KF5AkonadiContact
### Review how to get at contacts in more portable way
##
## Test for KF5AkonadiContact
##
# find_package(KF5AkonadiContact CONFIG QUIET)
# set_package_properties(KF5AkonadiContact PROPERTIES
#     DESCRIPTION "Library for Accessing Contacts stored in Akonadi"
#     URL "https://www.kde.org/"
#     PURPOSE "Required for address book support"
#     TYPE OPTIONAL
# )

##
## Test for KF6CalendarCore
##
find_package(KF6CalendarCore CONFIG QUIET)
set_package_properties(KF6CalendarCore PROPERTIES
    DESCRIPTION "KDE Calendar Library"
    URL "https://www.kde.org/"
    PURPOSE "Used by calendar export"
    TYPE OPTIONAL
    )

if(WIN32)
    set(LIB_INSTALL_DIR ${KDE_INSTALL_LIBDIR}
                        RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
                        LIBRARY ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
                        ARCHIVE ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
endif()

find_package(Java 8 COMPONENTS Runtime)
set_package_properties(Java PROPERTIES
    DESCRIPTION "Java Runtime Environment"
    URL "https://www.java.com/"
    PURPOSE "Used by MPXJ based import filters and unit tests"
    TYPE OPTIONAL
)

#### Disable kreport for now, reconsider when it is stable ####
if (TEST_FOR_KREPORT)
    #
    # Test for KReport
    #
    macro_optional_find_package(KReport 3.1 QUIET)
    set_package_properties(KReport PROPERTIES
        PURPOSE "Required for KReport support in Plan"
        TYPE OPTIONAL
    )

    ##
    ## Test for KPropertyWidgets
    ##
    macro_optional_find_package(KPropertyWidgets 3.1 QUIET)
    set_package_properties(KPropertyWidgets PROPERTIES
        PURPOSE "Required for KReports support in Plan"
        TYPE OPTIONAL
    )
    if (KReport_FOUND AND KPropertyWidgets_FOUND)
        set(PLAN_USE_KREPORT TRUE)
    endif()
endif (TEST_FOR_KREPORT)


add_subdirectory(src)
add_subdirectory(devtools)
add_subdirectory(doc)
add_subdirectory(data)

ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
endif()

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

### Summarize findings ####
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)


