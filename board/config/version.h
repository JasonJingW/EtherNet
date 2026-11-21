#ifndef PROJECT_VERSION_H
#define PROJECT_VERSION_H

#define PROJECT_NAME "EtherNet"

/* The build supplies shared metadata to version.c. Retain compiler defaults. */
#ifndef PROJECT_BUILD_DATE
#define PROJECT_BUILD_DATE __DATE__
#endif
#ifndef PROJECT_BUILD_TIME
#define PROJECT_BUILD_TIME __TIME__
#endif
#ifndef PROJECT_BUILD_COMMIT
#define PROJECT_BUILD_COMMIT "unknown"
#endif
#ifndef PROJECT_BUILD_COMMON_COMMIT
#define PROJECT_BUILD_COMMON_COMMIT "unknown"
#endif
#ifndef PROJECT_BUILD_APPLICATION_COMMIT
#define PROJECT_BUILD_APPLICATION_COMMIT "unknown"
#endif

/* Start at 0.0.1; increment for each release, not for each build.
 * Decimal carry: 0.0.9 -> 0.1.0; 0.9.9 -> 1.0.0.
 * MINOR and PATCH range from 0 to 9; MAJOR has no single-digit limit.
 */
#define PROJECT_VERSION_MAJOR (0)
#define PROJECT_VERSION_MINOR (0)
#define PROJECT_VERSION_PATCH (1)

#define PROJECT_VERSION_STRINGIFY_IMPL(value) #value
/* Numeric version macros are parenthesized; unwrap them before stringifying. */
#define PROJECT_VERSION_STRINGIFY(value) PROJECT_VERSION_STRINGIFY_IMPL value
#define PROJECT_VERSION_STRING \
    PROJECT_VERSION_STRINGIFY(PROJECT_VERSION_MAJOR) "." \
    PROJECT_VERSION_STRINGIFY(PROJECT_VERSION_MINOR) "." \
    PROJECT_VERSION_STRINGIFY(PROJECT_VERSION_PATCH)

/* Each callback receives one line without a line ending. */
void version_print_banner(void (*write_line)(const char *line));
void version_print_info(void (*write_line)(const char *line));

#endif /* PROJECT_VERSION_H */
