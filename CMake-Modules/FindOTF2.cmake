find_path(OTF2_INCLUDE_DIRS otf2/otf2.h
    $ENV{HOME}/opt/include
    /usr/opt/otf2/include
)

find_library(OTF2_LIBRARIES otf2
    $ENV{HOME}/opt/lib
    /usr/opt/otf2/lib
)


find_package_handle_standard_args(OTF2
	FAIL_MESSAGE "Couldn't find OTF2 library."
	REQUIRED_VARS OTF2_INCLUDE_DIRS OTF2_LIBRARIES
	)

