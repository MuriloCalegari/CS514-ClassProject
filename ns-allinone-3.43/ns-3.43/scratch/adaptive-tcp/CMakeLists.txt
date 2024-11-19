# Create a custom library
add_library(
  scratch-nested-subdir-lib
  lib/scratch-nested-subdir-library-source.cc
)

# add_library(
#   adaptive-tcp
#   lib/adaptive-tcp.cc
# )

# include_directories(src/internet)

# add_library(
#   nlohmann_json,
#   lib/nlohmann_json/json.hpp
# )

# Link the custom library to the scratch executable
build_exec(
  EXECNAME adaptive-tcp-test
  SOURCE_FILES adaptive-tcp-test.cc
  LIBRARIES_TO_LINK scratch-nested-subdir-lib
                    # adaptive-tcp
                    ${libcore}
                    ${libnetwork}
                    ${libinternet}
                    ${libpoint-to-point}
                    ${libapplications}
                    ${libnetanim}
  # use "${ns3-libs}" "${ns3-contrib-libs}" in case you want to link to all
  # enabled modules
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/adaptive-tcp
)
