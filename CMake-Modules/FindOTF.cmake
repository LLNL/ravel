find_path(OTF_INCLUDE_DIRS otf.h
    $ENV{HOME}/opt/include
    $ENV{HOME}/opt/include/otf
    $ENV{HOME}/opt/include/open-trace-format
)

find_library(OTF_LIBRARIES otf
    $ENV{HOME}/opt/lib
)

if (not OTF_LIBRARIES)
    find_library(OTF_LIBRARIES open-trace-format
	$ENV{HOME}/opt/lib
    )
endif()

find_package_handle_standard_args(OTF
	FAIL_MESSAGE "Couldn't find OTF library."
	REQUIRED_VARS OTF_INCLUDE_DIRS OTF_LIBRARIES
	)

