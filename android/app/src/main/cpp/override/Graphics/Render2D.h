/*
 * Android replacement for upstream Src/Graphics/Render2D.h.
 *
 * Same public interface as upstream CRender2D (the tile generator calls it
 * directly), implemented with OpenGL ES 3.0 in render2d_android.cpp. Upstream
 * draws the tile layers in software (TileGen::DrawLine), so this class only
 * uploads the two finished layers and draws them below and above the 3D image.
 */

#ifndef INCLUDED_RENDER2D_H
#define INCLUDED_RENDER2D_H

#include <GLES3/gl32.h>
#include <memory>
#include "Supermodel.h"
#include "Util/NewConfig.h"
#include "Graphics/New3D/GLSLShader.h"
#include "Model3/TileGenBuffer.h"
#include "Graphics/IRender2D.h"

class CRender2D : public IRender2D
{
public:
  void BeginFrame(void) override;
  void PreRenderFrame(void) override;
  void RenderFrameBottom(void) override;
  void RenderFrameTop(void) override;
  void EndFrame(void) override;

  void WriteVRAM(unsigned addr, uint32_t data) {}
  void AttachRegisters(const uint32_t* regPtr) {}
  void AttachPalette(const uint32_t* palPtr[2]) {}
  void AttachVRAM(const uint8_t* vramPtr) {}
  void AttachDrawBuffers(std::shared_ptr<TileGenBuffer> bottom, std::shared_ptr<TileGenBuffer> top);

  Result Init(unsigned xOffset, unsigned yOffset, unsigned xRes, unsigned yRes, unsigned totalXRes, unsigned totalYRes, unsigned aaTarget, UpscaleMode upscaleMode) override;

  CRender2D(const Util::Config::Node& config);
  ~CRender2D(void) override;

private:
  void Setup2D(bool isBottom);
  void DrawSurface(GLuint textureID);

  const Util::Config::Node& m_config;

  unsigned m_xPixels = 496;
  unsigned m_yPixels = 384;
  unsigned m_xOffset = 0;
  unsigned m_yOffset = 0;
  unsigned m_totalXPixels = 0;
  unsigned m_totalYPixels = 0;
  unsigned m_correction = 0;
  UpscaleMode m_upscaleMode = UpscaleMode::Nearest;

  GLuint m_vao = 0;
  GLuint m_textureIDs[2] = { 0, 0 };
  GLSLShader m_drawShader;
  std::shared_ptr<TileGenBuffer> m_drawBuffers[2];

  // SUPER3 widescreen: when the top layer covers the whole 4:3 picture (logos,
  // intermissions), the side bars show its edge colours instead of 3D leftovers
  void UpdateSideBars();
  void DrawSideBars();
  bool  m_coverSides = false;
  int   m_sideFrames = 0;       // frames the opposite state has been seen
  float m_sideColour[2][3] = {};  // left, right (RGB 0..1)
};

#endif  // INCLUDED_RENDER2D_H
