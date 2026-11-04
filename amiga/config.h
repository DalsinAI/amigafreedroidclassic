/* config.h for the AmigaOS 3.x build (amiga/Makefile.amigaos), in place of
 * the one configure writes. The AmiCradle Team. GPL 2 or later. */
#define PACKAGE "freedroid"
#define VERSION "1.2.4"
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_FCNTL_H 1
#define HAVE_DIRENT_H 1
#define HAVE_LIBSDL_MIXER 1
/* Data and config live in the program's drawer. */
#define FD_DATADIR "PROGDIR:"
#define LOCAL_DATADIR "PROGDIR:"
