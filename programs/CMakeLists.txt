#  
# Include and configure the channelflow programs
#
# This file is a part of channelflow version 2.0.
# License is GNU GPL version 2 or later: https://channelflow.org/license
#
set(program_APPS
  simulateflow
  edgetracking
  )

if (WITH_NSOLVER)
    set(program_APPS ${program_APPS}
         continuesoln
         findsoln
         findeigenvals)
endif ()


foreach (program ${program_APPS})
    install_channelflow_application(${program} bin)
endforeach ()
