// vid_sdl.h -- sdl video driver 

#include <SDL/SDL.h>
#include "quakedef.h"
#include "d_local.h"

#include "cdaudio.h"
#include "client.h"
#include "common.h"
#include "console.h"
#include "cvar.h"
#include "input.h"
#include "keys.h"
#include "mathlib.h"
#include "quakedef.h"
#include "sdl_common.h"
#include "sound.h"
#include "sys.h"
#include "vid.h"
#include "host.h"
#include "port.h"

int32_t spacing_x_res = 0;

int vid_modenum = VID_MODE_NONE;
static cvar_t m_filter = { "m_filter", "0" };

viddef_t    vid;                // global video state
uint16_t	d_8to16table[256];

int32_t	VGA_width, VGA_height, VGA_rowubytes, VGA_bufferrowubytes = 0;
uint8_t *VGA_pagebase;

static SDL_Surface *screen = NULL, *rl_screen;

static qboolean mouse_avail;
static float   mouse_x, mouse_y;
static int32_t		mouse_oldbuttonstate = 0;

void   VID_SetPalette (const uint8_t *palette)
{
    int32_t		i;
    SDL_Color colors[256];

    for ( i=0; i<256; ++i )
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
    }
    SDL_SetColors(screen, colors, 0, 256);
}

void    VID_ShiftPalette (const uint8_t *palette)
{
    VID_SetPalette(palette);
}

void    VID_Init (const uint8_t *palette)
{
    int32_t		pnum, chunk;
    uint8_t *cache;
    int32_t		cachesize;
    uint8_t video_bpp;
    Uint16 video_w, video_h;
    Uint32 flags;

    // Load the SDL library
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_CDROM) < 0)
        Sys_Error("VID: Couldn't load SDL: %s", SDL_GetError());

    // Set up display mode (width and height)
    vid.width = BASEWIDTH;
    vid.height = BASEHEIGHT;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;

    // check for command-line window size
    if ((pnum=COM_CheckParm("-winsize")))
    {
        if (pnum >= com_argc-2)
            Sys_Error("VID: -winsize <width> <height>\n");
        vid.width = Q_atoi(com_argv[pnum+1]);
        vid.height = Q_atoi(com_argv[pnum+2]);
        if (!vid.width || !vid.height)
            Sys_Error("VID: Bad window width/height\n");
    }
    if ((pnum=COM_CheckParm("-width"))) {
        if (pnum >= com_argc-1)
            Sys_Error("VID: -width <width>\n");
        vid.width = Q_atoi(com_argv[pnum+1]);
        if (!vid.width)
            Sys_Error("VID: Bad window width\n");
    }
    if ((pnum=COM_CheckParm("-height"))) {
        if (pnum >= com_argc-1)
            Sys_Error("VID: -height <height>\n");
        vid.height = Q_atoi(com_argv[pnum+1]);
        if (!vid.height)
            Sys_Error("VID: Bad window height\n");
    }

    // Set video width, height and flags
    flags = (SDL_SWSURFACE|SDL_HWPALETTE);

    if ( COM_CheckParm ("-fullscreen") )
        flags |= SDL_FULLSCREEN;

    if ( COM_CheckParm ("-window") ) {
        flags &= ~SDL_FULLSCREEN;
    }

    // Initialize display 
	if (!(rl_screen = SDL_SetVideoMode(vid.width, vid.height, 16, SDL_HWSURFACE|SDL_ASYNCBLIT|SDL_ANYFORMAT|SDL_HWPALETTE)))
		Sys_Error("VID: Couldn't set video mode: %s\n", SDL_GetError());
	screen = SDL_CreateRGBSurface(SDL_SWSURFACE, vid.width, vid.height, 8, 0,0,0,0);
        
    VID_SetPalette(palette);
    SDL_WM_SetCaption("sdlquake","sdlquake");
    // now know everything we need to know about the buffer
    VGA_width = vid.conwidth = vid.width;
    VGA_height = vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((int32_t		*)vid.colormap + 2048));
    VGA_pagebase = vid.buffer = screen->pixels;
    VGA_rowubytes = vid.rowbytes = screen->pitch;
    vid.conbuffer = vid.buffer;
    vid.conrowbytes = vid.rowbytes;
    vid.direct = 0;
    
    // allocate z buffer and surface cache
    chunk = vid.width * vid.height * sizeof (*d_pzbuffer);
    cachesize = D_SurfaceCacheForRes (vid.width, vid.height);
    chunk += cachesize;
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");
    if (d_pzbuffer == NULL)
        Sys_Error ("Not enough memory for video mode\n");

    // initialize the cache memory 
        cache = (uint8_t *) d_pzbuffer
                + vid.width * vid.height * sizeof (*d_pzbuffer);
    D_InitCaches (cache, cachesize);

    // initialize the mouse
    SDL_ShowCursor(0);
    
    spacing_x_res = vid.width - 320;
    if (spacing_x_res < 0) spacing_x_res = 0;
}

void    VID_Shutdown (void)
{
    SDL_Quit();
}

void    VID_Update (vrect_t *rects)
{
    SDL_Rect *sdlrects;
    int32_t		n, i;
    vrect_t *rect;

    // Two-pass system, since Quake doesn't do it the SDL way...

    // First, count the number of rectangles
    n = 0;
    for (rect = rects; rect; rect = rect->pnext)
        ++n;

    // Second, copy them to SDL rectangles and update
    if (!(sdlrects = (SDL_Rect *)alloca(n*sizeof(*sdlrects))))
        Sys_Error("Out of memory");
    i = 0;
    for (rect = rects; rect; rect = rect->pnext)
    {
        sdlrects[i].x = rect->x;
        sdlrects[i].y = rect->y;
        sdlrects[i].w = rect->width;
        sdlrects[i].h = rect->height;
        ++i;
    }
    SDL_BlitSurface(screen, NULL, rl_screen, NULL);
    SDL_UpdateRects(rl_screen, n, sdlrects);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int32_t		x, int32_t		y, const uint8_t *pbitmap, int32_t		width, int32_t		height)
{
    uint8_t *offset;

    if (!screen) return;
    if ( x < 0 ) x = screen->w+x-1;
    offset = (uint8_t *)screen->pixels + y*screen->pitch + x;
    while ( height-- )
    {
        memcpy(offset, pbitmap, width);
        offset += screen->pitch;
        pbitmap += width;
    }
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int32_t		x, int32_t		y, int32_t		width, int32_t		height)
{
    if (!screen) return;
    if (x < 0) x = screen->w+x-1;
    SDL_BlitSurface(screen, NULL, rl_screen, NULL);
    SDL_UpdateRect(rl_screen, x, y, width, height);
}


/*
================
Sys_SendKeyEvents
================
*/

void Sys_SendKeyEvents(void)
{
    SDL_Event event;
    int keycode;
    int keystate, button, keynum;

    while (SDL_PollEvent(&event)) {
	switch (event.type) 
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	    keycode = event.key.keysym.sym;
	    keystate = event.key.state;
	    switch (keycode) {
	    case SDLK_UNKNOWN:
		keynum = K_UNKNOWN;
		break;
	    case SDLK_BACKSPACE:
		keynum = K_BACKSPACE;
		break;
	    case SDLK_TAB:
		keynum = K_TAB;
		break;
	    case SDLK_CLEAR:
		keynum = K_CLEAR;
		break;
	    case SDLK_RETURN:
		keynum = K_ENTER;
		break;
	    case SDLK_PAUSE:
		keynum = K_PAUSE;
		break;
	    case SDLK_ESCAPE:
		keynum = K_ESCAPE;
		break;
	    case SDLK_SPACE:
		keynum = K_SPACE;
		break;
	    case SDLK_EXCLAIM:
		keynum = K_EXCLAIM;
		break;
	    case SDLK_QUOTEDBL:
		keynum = K_QUOTEDBL;
		break;
	    case SDLK_HASH:
		keynum = K_HASH;
		break;
	    case SDLK_DOLLAR:
		keynum = K_DOLLAR;
		break;
	    case SDLK_AMPERSAND:
		keynum = K_AMPERSAND;
		break;
	    case SDLK_QUOTE:
		keynum = K_QUOTE;
		break;
	    case SDLK_LEFTPAREN:
		keynum = K_LEFTPAREN;
		break;
	    case SDLK_RIGHTPAREN:
		keynum = K_RIGHTPAREN;
		break;
	    case SDLK_ASTERISK:
		keynum = K_ASTERISK;
		break;
	    case SDLK_PLUS:
		keynum = K_PLUS;
		break;
	    case SDLK_COMMA:
		keynum = K_COMMA;
		break;
	    case SDLK_MINUS:
		keynum = K_MINUS;
		break;
	    case SDLK_PERIOD:
		keynum = K_PERIOD;
		break;
	    case SDLK_SLASH:
		keynum = K_SLASH;
		break;
	    case SDLK_0:
		keynum = K_0;
		break;
	    case SDLK_1:
		keynum = K_1;
		break;
	    case SDLK_2:
		keynum = K_2;
		break;
	    case SDLK_3:
		keynum = K_3;
		break;
	    case SDLK_4:
		keynum = K_4;
		break;
	    case SDLK_5:
		keynum = K_5;
		break;
	    case SDLK_6:
		keynum = K_6;
		break;
	    case SDLK_7:
		keynum = K_7;
		break;
	    case SDLK_8:
		keynum = K_8;
		break;
	    case SDLK_9:
		keynum = K_9;
		break;
	    case SDLK_COLON:
		keynum = K_COLON;
		break;
	    case SDLK_SEMICOLON:
		keynum = K_SEMICOLON;
		break;
	    case SDLK_LESS:
		keynum = K_LESS;
		break;
	    case SDLK_EQUALS:
		keynum = K_EQUALS;
		break;
	    case SDLK_GREATER:
		keynum = K_GREATER;
		break;
	    case SDLK_QUESTION:
		keynum = K_QUESTION;
		break;
	    case SDLK_AT:
		keynum = K_AT;
		break;
	    case SDLK_LEFTBRACKET:
		keynum = K_LEFTBRACKET;
		break;
	    case SDLK_BACKSLASH:
		keynum = K_BACKSLASH;
		break;
	    case SDLK_RIGHTBRACKET:
		keynum = K_RIGHTBRACKET;
		break;
	    case SDLK_CARET:
		keynum = K_CARET;
		break;
	    case SDLK_UNDERSCORE:
		keynum = K_UNDERSCORE;
		break;
	    case SDLK_BACKQUOTE:
		keynum = K_BACKQUOTE;
		break;
	    case SDLK_a:
		keynum = K_a;
		break;
	    case SDLK_b:
		keynum = K_b;
		break;
	    case SDLK_c:
		keynum = K_c;
		break;
	    case SDLK_d:
		keynum = K_d;
		break;
	    case SDLK_e:
		keynum = K_e;
		break;
	    case SDLK_f:
		keynum = K_f;
		break;
	    case SDLK_g:
		keynum = K_g;
		break;
	    case SDLK_h:
		keynum = K_h;
		break;
	    case SDLK_i:
		keynum = K_i;
		break;
	    case SDLK_j:
		keynum = K_j;
		break;
	    case SDLK_k:
		keynum = K_k;
		break;
	    case SDLK_l:
		keynum = K_l;
		break;
	    case SDLK_m:
		keynum = K_m;
		break;
	    case SDLK_n:
		keynum = K_n;
		break;
	    case SDLK_o:
		keynum = K_o;
		break;
	    case SDLK_p:
		keynum = K_p;
		break;
	    case SDLK_q:
		keynum = K_q;
		break;
	    case SDLK_r:
		keynum = K_r;
		break;
	    case SDLK_s:
		keynum = K_s;
		break;
	    case SDLK_t:
		keynum = K_t;
		break;
	    case SDLK_u:
		keynum = K_u;
		break;
	    case SDLK_v:
		keynum = K_v;
		break;
	    case SDLK_w:
		keynum = K_w;
		break;
	    case SDLK_x:
		keynum = K_x;
		break;
	    case SDLK_y:
		keynum = K_y;
		break;
	    case SDLK_z:
		keynum = K_z;
		break;
	    case SDLK_DELETE:
		keynum = K_DEL;
		break;
	    case SDLK_KP0:
		keynum = K_KP0;
		break;
	    case SDLK_KP1:
		keynum = K_KP1;
		break;
	    case SDLK_KP2:
		keynum = K_KP2;
		break;
	    case SDLK_KP3:
		keynum = K_KP3;
		break;
	    case SDLK_KP4:
		keynum = K_KP4;
		break;
	    case SDLK_KP5:
		keynum = K_KP5;
		break;
	    case SDLK_KP6:
		keynum = K_KP6;
		break;
	    case SDLK_KP7:
		keynum = K_KP7;
		break;
	    case SDLK_KP8:
		keynum = K_KP8;
		break;
	    case SDLK_KP9:
		keynum = K_KP9;
		break;
	    case SDLK_KP_PERIOD:
		keynum = K_KP_PERIOD;
		break;
	    case SDLK_KP_DIVIDE:
		keynum = K_KP_DIVIDE;
		break;
	    case SDLK_KP_MULTIPLY:
		keynum = K_KP_MULTIPLY;
		break;
	    case SDLK_KP_MINUS:
		keynum = K_KP_MINUS;
		break;
	    case SDLK_KP_PLUS:
		keynum = K_KP_PLUS;
		break;
	    case SDLK_KP_ENTER:
		keynum = K_KP_ENTER;
		break;
	    case SDLK_KP_EQUALS:
		keynum = K_KP_EQUALS;
		break;
	    case SDLK_UP:
		keynum = K_UPARROW;
		break;
	    case SDLK_DOWN:
		keynum = K_DOWNARROW;
		break;
	    case SDLK_RIGHT:
		keynum = K_RIGHTARROW;
		break;
	    case SDLK_LEFT:
		keynum = K_LEFTARROW;
		break;
	    case SDLK_INSERT:
		keynum = K_INS;
		break;
	    case SDLK_HOME:
		keynum = K_HOME;
		break;
	    case SDLK_END:
		keynum = K_END;
		break;
	    case SDLK_PAGEUP:
		keynum = K_PGUP;
		break;
	    case SDLK_PAGEDOWN:
		keynum = K_PGDN;
		break;
	    case SDLK_F1:
		keynum = K_F1;
		break;
	    case SDLK_F2:
		keynum = K_F2;
		break;
	    case SDLK_F3:
		keynum = K_F3;
		break;
	    case SDLK_F4:
		keynum = K_F4;
		break;
	    case SDLK_F5:
		keynum = K_F5;
		break;
	    case SDLK_F6:
		keynum = K_F6;
		break;
	    case SDLK_F7:
		keynum = K_F7;
		break;
	    case SDLK_F8:
		keynum = K_F8;
		break;
	    case SDLK_F9:
		keynum = K_F9;
		break;
	    case SDLK_F10:
		keynum = K_F10;
		break;
	    case SDLK_F11:
		keynum = K_F11;
		break;
	    case SDLK_F12:
		keynum = K_F12;
		break;
	    case SDLK_F13:
		keynum = K_F13;
		break;
	    case SDLK_F14:
		keynum = K_F14;
		break;
	    case SDLK_F15:
		keynum = K_F15;
		break;
	    case SDLK_CAPSLOCK:
		keynum = K_CAPSLOCK;
		break;
	    case SDLK_RSHIFT:
		keynum = K_RSHIFT;
		break;
	    case SDLK_LSHIFT:
		keynum = K_LSHIFT;
		break;
	    case SDLK_RCTRL:
		keynum = K_RCTRL;
		break;
	    case SDLK_LCTRL:
		keynum = K_LCTRL;
		break;
	    case SDLK_RALT:
		keynum = K_RALT;
		break;
	    case SDLK_LALT:
		keynum = K_LALT;
		break;
	    case SDLK_MODE:
		keynum = K_MODE;
		break;
	    case SDLK_HELP:
		keynum = K_HELP;
		break;
	    case SDLK_SYSREQ:
		keynum = K_SYSREQ;
		break;
	    case SDLK_MENU:
		keynum = K_MENU;
		break;
	    case SDLK_POWER:
		keynum = K_POWER;
		break;
	    case SDLK_UNDO:
		keynum = K_UNDO;
		break;
	    default:
		    keynum = K_UNKNOWN;
		break;
	    }
	    Key_Event(keynum, keystate);
	    break;

	case SDL_QUIT:
	    Sys_Quit();
	    break;
	default:
	    break;
	}
    }
}


void IN_Init (void)
{
    mouse_avail = 0;
}

void IN_Shutdown (void)
{
    mouse_avail = 0;
}

void IN_Commands (void)
{
}

void IN_Move (usercmd_t *cmd)
{
    if (!mouse_avail)
        return;
}

static void windowed_mouse_f(struct cvar_s *var)
{
}

cvar_t _windowed_mouse = { "_windowed_mouse", "0", true, false, 0, windowed_mouse_f };

void
VID_LockBuffer(void)
{
}

void
VID_UnlockBuffer(void)
{
}

qboolean
VID_SetMode(const qvidmode_t *mode, const byte *palette)
{
	//VID_Init(palette);
}

/*
====================
VID_CheckAdequateMem
====================
*/
qboolean
VID_CheckAdequateMem(int width, int height)
{
    int tbuffersize;

    tbuffersize = width * height * sizeof(*d_pzbuffer);
    tbuffersize += D_SurfaceCacheForRes(width, height);

    /*
     * see if there's enough memory, allowing for the normal mode 0x13 pixel,
     * z, and surface buffers
     */
    if ((host_parms.memsize - tbuffersize + SURFCACHE_SIZE_AT_320X200 +
	 0x10000 * 3) < minimum_memory)
	return false;

    return true;
}
