/*
 * Android OSD pieces that upstream Supermodel leaves to the platform.
 *
 * Everything else the core needs from the OSD layer is upstream code built
 * unchanged: Src/OSD/{Logger,Outputs,Thread}.cpp and Src/OSD/SDL/Audio.cpp.
 */

#include <SDL.h>
#include <cstdarg>
#include <memory>

#include "OSD/Logger.h"
#include "OSD/Video.h"
#include "android_osd.h"

// Video callbacks around CModel3::RenderFrame(). The GL context is always
// current on the emulator thread, so there is nothing to do.
bool BeginFrameVideo() { return true; }
void EndFrameVideo() {}

namespace
{
  class CAndroidLogger : public CLogger
  {
  public:
    void DebugLog(const char* fmt, va_list vl) override
    {
#ifndef NDEBUG
      SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, fmt, vl);
#endif
    }
    void InfoLog(const char* fmt, va_list vl) override
    {
      SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, vl);
    }
    void ErrorLog(const char* fmt, va_list vl) override
    {
      SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, vl);
    }
  };
}

void InstallAndroidLogger()
{
  SetLogger(std::make_shared<CAndroidLogger>());
}
