# Create a custom library
add_library(
  scratch-nested-subdir-lib
  lib/scratch-nested-subdir-library-source.cc
)

# Link the custom library to the scratch executable
build_exec(
  EXECNAME adaptive-tcp-test
  SOURCE_FILES adaptive-tcp-test.cc
  LIBRARIES_TO_LINK scratch-nested-subdir-lib
                    ${libcore}
                    ${libnetwork}
                    ${libinternet}
                    ${libpoint-to-point}
                    ${libapplications}
                    ${libnetanim}
                    ${mpi}
  # use "${ns3-libs}" "${ns3-contrib-libs}" in case you want to link to all
  # enabled modules
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/adaptive-tcp
)
