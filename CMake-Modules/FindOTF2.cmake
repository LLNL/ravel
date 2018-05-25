find_path(OTF2_INCLUDE_DIRS 
    NAMES otf2/otf2.h
    HINTS $ENV{HOME}/opt/include $ENV{HOME}/usr/local/include /usr/opt/otf2/include /usr/local/lib/otf2/include /opt/otf2/include
)

find_library(OTF2_LIBRARIES 
    NAMES otf2
    HINTS $ENV{HOME}/opt/lib $ENV{HOME}/usr/local/lib /usr/opt/otf2/lib /usr/local/lib/otf2/lib /opt/otf2/lib
)


find_package_handle_standard_args(OTF2
	FAIL_MESSAGE "Couldn't find OTF2 library."
	REQUIRED_VARS OTF2_INCLUDE_DIRS OTF2_LIBRARIES
	)

