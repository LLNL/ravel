FIND_PATH(Muster_INCLUDE_DIRS kmedoids.h
    $ENV{HOME}/opt/include
    $ENV{HOME}/opt/muster/include
)

FIND_LIBRARY(Muster_LIBRARIES muster
    $ENV{HOME}/opt/lib
    $ENV{HOME}/opt/muster/lib
)


find_package_handle_standard_args(Muster
	FAIL_MESSAGE "Couldn't find Muster."
	REQUIRED_VARS Muster_INCLUDE_DIRS Muster_LIBRARIES
	)

