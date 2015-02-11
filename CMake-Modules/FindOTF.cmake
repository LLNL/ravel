find_path(OTF_INCLUDE_DIRS otf.h
  PATHS $ENV{HOME}/opt/include $ENV{HOME}/opt/include/otf
  PATH_SUFFIXES open-trace-format)

find_library(
  OTF_LIBRARIES otf
  PATHS $ENV{HOME}/opt/lib
  PATH_SUFFIXES open-trace-format)

if (NOT OTF_LIBRARIES)
  find_library(
    OTF_LIBRARIES
    PATHS $ENV{HOME}/opt/lib
    PATH_SUFFIXES open-trace-format)
endif()

find_package_handle_standard_args(
  OTF
  FAIL_MESSAGE "Couldn't find OTF library."
  REQUIRED_VARS OTF_INCLUDE_DIRS OTF_LIBRARIES)

