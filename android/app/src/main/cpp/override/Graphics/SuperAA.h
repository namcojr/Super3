/*
 * Android replacement for upstream Src/Graphics/SuperAA.h.
 *
 * Upstream SuperAA draws through a desktop OpenGL framebuffer for supersampling
 * and CRT colour correction. SUPER3 draws straight to the GLES back buffer, so
 * this keeps the same interface and does nothing.
 */

#ifndef INCLUDED_SUPERAA_H
#define INCLUDED_SUPERAA_H

#include "Supermodel.h"

class SuperAA
{
public:
  SuperAA(int aaValue, CRTcolor CRTcolors) {}

  void Init(int width, int height) {}
  void Draw() {}
  unsigned GetTargetID() { return 0; }  // 0 = back buffer
};

#endif  // INCLUDED_SUPERAA_H
