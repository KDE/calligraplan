
if(KF6CalendarCore_FOUND)
    add_subdirectory( icalendar )
endif()

add_subdirectory( planner )
add_subdirectory(mpxj)

if(WIN32)
    #disable for now
    #add_subdirectory( kplato )
else()
    add_subdirectory( kplato )
endif()
