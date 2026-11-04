/* Freedroid keeps 500 to 2048-byte path and line buffers on the stack,
 * several deep (find_file under InitPictures, the menus), and the C
 * library's printf wants its share; a Shell gives 4 KB. libnix's startup
 * sets this much up instead. The AmiCradle Team. GPL 2 or later. */
unsigned long __stack = 160 * 1024;
