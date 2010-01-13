#define CONFIG_WIDTH  280
#define CONFIG_HEIGHT 220

#define EMERGE_MAJOR_VERSION 4
#define EMERGE_MINOR_VERSION 2
#define EMERGE_RELEASE_VERSION 0
#define BUILD_VERSION 0

#ifndef STRINGIFY
#define STRINGIFY(s) #s
#endif

#define POINT_VERSION STRINGIFY(EMERGE_MAJOR_VERSION.EMERGE_MINOR_VERSION.EMERGE_RELEASE_VERSION.BUILD_VERSION)
#define COMMA_VERSION EMERGE_MAJOR_VERSION,EMERGE_MINOR_VERSION,EMERGE_RELEASE_VERSION,BUILD_VERSION
