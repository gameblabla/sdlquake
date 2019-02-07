#ifndef PORT_H
#define PORT_H

extern int32_t spacing_x_res;

#ifdef RS90
#define BASEWIDTH 240
#define BASEHEIGHT 160
#elif defined(ARCADE_MINI)
#define BASEWIDTH 480
#define BASEHEIGHT 272
#elif defined(DINGUX)
#define BASEWIDTH 320
#define BASEHEIGHT 240
#elif defined(PAPK3S)
#define BASEWIDTH 800
#define BASEHEIGHT 480
#else
#define BASEWIDTH 1280
#define BASEHEIGHT 720
#endif

#endif
