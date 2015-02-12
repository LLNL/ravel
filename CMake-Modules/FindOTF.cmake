find_path(OTF_INCLUDE_DIRS otf.h
  PATHS $ENV{HOME}/opt/include $ENV{HOME}/opt/include/otf
  PATH_SUFFIXES open-trace-format)

find_library(
  OTF_LIBRARIES open-trace-format
  PATHS $ENV{HOME}/opt/lib)

find_package_handle_standard_args(
  OTF
  FAIL_MESSAGE "Couldn't find OTF library."
  REQUIRED_VARS OTF_INCLUDE_DIRS OTF_LIBRARIES)

