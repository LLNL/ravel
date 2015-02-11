find_path(Muster_INCLUDE_DIRS kmedoids.h
  PATHS $ENV{HOME}/opt/include $ENV{HOME}/opt/muster/include
  PATH_SUFFIXES muster
)

find_library(Muster_LIBRARIES muster
  PATHS $ENV{HOME}/opt/lib $ENV{HOME}/opt/muster/lib
  PATH_SUFFIXES muster
)


find_package_handle_standard_args(Muster
  FAIL_MESSAGE "Couldn't find Muster."
  REQUIRED_VARS Muster_INCLUDE_DIRS Muster_LIBRARIES)

