#include "main.h"

#include <stdarg.h>
#include <stdlib.h>

#define VIDEO_TAG(X) angrylion##X
#define PluginStartup VIDEO_TAG(PluginStartup)
#define PluginShutdown VIDEO_TAG(PluginShutdown)
#define PluginGetVersion VIDEO_TAG(PluginGetVersion)
#define ChangeWindow VIDEO_TAG(ChangeWindow)
#define InitiateGFX VIDEO_TAG(InitiateGFX)
#define MoveScreen VIDEO_TAG(MoveScreen)
#define ProcessDList VIDEO_TAG(ProcessDList)
#define ProcessRDPList VIDEO_TAG(ProcessRDPList)
#define RomClosed VIDEO_TAG(RomClosed)
#define RomOpen VIDEO_TAG(RomOpen)
#define ShowCFB VIDEO_TAG(ShowCFB)
#define UpdateScreen VIDEO_TAG(UpdateScreen)
#define ViStatusChanged VIDEO_TAG(ViStatusChanged)
#define ViWidthChanged VIDEO_TAG(ViWidthChanged)
#define ReadScreen2 VIDEO_TAG(ReadScreen2)
#define SetRenderingCallback VIDEO_TAG(SetRenderingCallback)
#define ResizeVideoOutput VIDEO_TAG(ResizeVideoOutput)
#define FBRead VIDEO_TAG(FBRead)
#define FBWrite VIDEO_TAG(FBWrite)
#define FBGetFrameBufferInfo VIDEO_TAG(FBGetFrameBufferInfo)

GFX_INFO gfx_al;

int ProcessDListShown = 0;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define VLOG(...) angrylion_WriteLog(M64MSG_VERBOSE, __VA_ARGS__)

static void (*l_DebugCallback)(void *, int, const char *) = NULL;
static void *l_DebugCallContext = NULL;


void angrylion_WriteLog(m64p_msg_level level, const char *msg, ...)
{
   char buf[1024];
   va_list args;
   va_start(args, msg);
   vsnprintf(buf, 1023, msg, args);
   buf[1023]='\0';
   va_end(args);
   if (l_DebugCallback)
   {
      l_DebugCallback(l_DebugCallContext, level, buf);
   }
#if 0
   else
      fprintf(stdout, buf);
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle,
        void *Context, void (*DebugCallback)(void *, int, const char *))
{
   return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
VLOG("CALL PluginShutDown ()\n");
   return M64ERR_SUCCESS; // __LIBRETRO__: Fix warning
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
VLOG("CALL PluginGetVersion ()\n");
   /* set version info */
   if (PluginType != NULL)
      *PluginType = M64PLUGIN_GFX;

   if (PluginVersion != NULL)
      *PluginVersion = 0x016304;

   if (APIVersion != NULL)
      *APIVersion = 0x020100;

   if (PluginNamePtr != NULL)
      *PluginNamePtr = "MAME video Plugin";

   if (Capabilities != NULL)
      *Capabilities = 0;

   return M64ERR_SUCCESS;
}



EXPORT void CALL ChangeWindow (void)
{
VLOG ("changewindow ()\n");
}

EXPORT int CALL InitiateGFX (GFX_INFO Gfx_Info)
{
   VLOG ("InitGRAPHICS ()\n");
	gfx_al = Gfx_Info;
	VLOG ("InitGRAPHICS (2)\n");
	
    return 1;
}

EXPORT void CALL MoveScreen (int xpos, int ypos)
{
 VLOG ("movescreen\n");
}

EXPORT void CALL ProcessDList(void)
{
   if (!ProcessDListShown)
   {
      VLOG ("processdlist ()\n");
      ProcessDListShown = 1;
   }
}

 EXPORT void CALL ProcessRDPList(void)
{
	 VLOG ("processrdplist ()\n");
	rdp_process_list();	
}

int32_t *blitter_buf;
INT32 pitchindwords;

EXPORT void CALL RomClosed (void)
{
   VLOG ("RomClosed ()\n");
   rdp_close();

   if (blitter_buf)
      free(blitter_buf);
}

EXPORT int CALL RomOpen (void)
{
   VLOG ("RomOpen ()\n");
   screen_width = 640;  // prescale width
   screen_height = 625; // prescale height
   blitter_buf = (int32_t*)calloc(screen_width * screen_height, sizeof(int32_t));
   pitchindwords = screen_width;
   screen_pitch = screen_width * 4;
   rdp_init();

   return true;
}

extern int retro_return(bool just_flipping);

EXPORT void CALL ShowCFB (void)
{
	VLOG ("draw2()\n");
	rdp_update();
   retro_return(true);
}

EXPORT void CALL UpdateScreen (void)
{
   VLOG ("draw1 ()\n");
   rdp_update();
   retro_return(true);
}

EXPORT void CALL ViStatusChanged (void)
{
 VLOG ("height\n");

}

EXPORT void CALL ViWidthChanged (void)
{
 VLOG ("width\n");
}

EXPORT void CALL ReadScreen2 (void *dest, int *width, int *height, int front)
{
 VLOG ("read screen\n");
}

EXPORT void CALL SetRenderingCallback(void (*callback)(int))
{
 VLOG ("render callback\n");

}

EXPORT void CALL FBRead(unsigned int addr)
{
 VLOG ("fbread\n");
}

EXPORT void CALL FBWrite(unsigned int addr, unsigned int size)
{
 VLOG ("fbwrite\n");
}

EXPORT void CALL FBGetFrameBufferInfo(void *p)
{
 VLOG ("fbget\n");
}

EXPORT void CALL ResizeVideoOutput(int width, int height)
{
 VLOG ("resize video\n");
}

#ifdef __cplusplus
}
#endif



