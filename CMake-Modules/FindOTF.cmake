FIND_PATH(OTF_INCLUDE_DIRS otf.h
    $ENV{HOME}/opt/include
    $ENV{HOME}/opt/include/otf
    $ENV{HOME}/opt/include/open-trace-format
)

FIND_LIBRARY(OTF_LIBRARIES otf
    $ENV{HOME}/opt/lib
)

IF (NOT OTF_LIBRARIES)
    FIND_LIBRARY(OTF_LIBRARIES open-trace-format
	$ENV{HOME}/opt/lib
    )
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(OTF
	FAIL_MESSAGE "Couldn't find OTF library."
	REQUIRED_VARS OTF_INCLUDE_DIRS OTF_LIBRARIES
	)

