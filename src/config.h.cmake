#include <QtGlobal>

// Check windows
#ifdef Q_OS_WIN
   #ifdef _WIN64
     #define ENV64BIT
  #else
    #define ENV32BIT
  #endif
#endif

// Check GCC
#if __GNUC__
  #if defined (__x86_64__) || defined (__ppc64__)
    #define ENV64BIT
  #else
    #define ENV32BIT
  #endif
#endif


/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Defines if we use KActivities */
#cmakedefine HAVE_KACTIVITIES 1

/* Defines project version */
#cmakedefine PLAN_VERSION_STRING "${PLAN_VERSION_STRING}"

/* Defines Plan year */
#cmakedefine PLAN_YEAR "${PLAN_YEAR}"

/* Defines Plan release status */
#cmakedefine PLAN_ALPHA "${PLAN_ALPHA}"
#cmakedefine PLAN_BETA "${PLAN_BETA}"

/* Defines use of KReport */
#cmakedefine PLAN_USE_KREPORT 1

/* Define KGantt version */
#cmakedefine KGANTT_VERSION_MAJOR ${KGANTT_VERSION_MAJOR}
#cmakedefine KGANTT_VERSION_MINOR ${KGANTT_VERSION_MINOR}
#cmakedefine KGANTT_VERSION_PATCH ${KGANTT_VERSION_PATCH}
#define KGANTT_VERSION  ((KGANTT_VERSION_MAJOR<<16)|(KGANTT_VERSION_MINOR<<8)|(KGANTT_VERSION_PATCH))
