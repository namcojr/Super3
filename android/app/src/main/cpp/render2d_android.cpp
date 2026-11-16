/*
 * OpenGL ES 3.0 port of upstream Src/Graphics/Render2D.cpp.
 *
 * Mirrors upstream behaviour line for line and compiles upstream's own 2D
 * shaders (Src/Graphics/Shaders2D.h; converted to GLSL ES by gles_compat.cpp),
 * so upstream shader changes (upscale filters etc.) come with every sync.
 *
 * SUPER3 addition: side bars in widescreen mode (UpdateSideBars/DrawSideBars).
 */

#include "Graphics/Render2D.h"
#include "Graphics/Shaders2D.h"
#include "OSD/Logger.h"

#include <algorithm>
#include <string>
#include <unordered_map>

void CRender2D::Setup2D(bool isBottom)
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  if (isBottom)
  {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glViewport(0, 0, m_totalXPixels, m_totalYPixels);
    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
  }

  bool stretchBottom = m_config["WideBackground"].ValueAsDefault<bool>(false) && isBottom;
  if (!stretchBottom)
    glViewport(m_xOffset - m_correction, m_yOffset + m_correction, m_xPixels, m_yPixels);
}

void CRender2D::DrawSurface(GLuint textureID)
{
  m_drawShader.EnableShader();
  glEnable(GL_BLEND);
  glBindVertexArray(m_vao);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
  glDisable(GL_BLEND);
  m_drawShader.DisableShader();
}

void CRender2D::BeginFrame(void)
{
}

void CRender2D::PreRenderFrame(void)
{
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 512);  // skip the non viewable data
  for (int i = 0; i < 2; i++)
  {
    if (!m_drawBuffers[i])
      continue;
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[i]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 496, 384, GL_RGBA, GL_UNSIGNED_BYTE, m_drawBuffers[i]->data);
  }
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  UpdateSideBars();
}

// Tile layer pixels are 0xAABBGGRR (uploaded as RGBA bytes); 496x384 visible, rows 512 apart.
void CRender2D::UpdateSideBars()
{
  // Switch only after 3 frames in the new state, so single-frame transitions don't flash
  auto settle = [this](bool cover) {
    m_sideFrames = (cover == m_coverSides) ? 0 : m_sideFrames + 1;
    if (m_sideFrames >= 3)
    {
      m_coverSides = cover;
      m_sideFrames = 0;
      DebugLog("Widescreen side bars: %s\n", cover ? "edge colour (4:3 picture)" : "3D");
    }
  };
  const auto& top = m_drawBuffers[1];
  if (!top || !m_config["WideScreen"].ValueAsDefault<bool>(false))
  {
    m_coverSides = false;
    return;
  }

  // Fully opaque top layer? (sampled every 4 pixels; the 3D image can't be seen through it)
  unsigned samples = 0, opaque = 0;
  for (int y = 0; y < 384; y += 4)
    for (int x = 0; x < 496; x += 4, samples++)
      opaque += (top->data[y * 512 + x] >> 24) == 0xFF;
  if (opaque * 1000 < samples * 995)
  {
    settle(false);
    return;
  }

  // Most common colour of each edge column
  for (int side = 0; side < 2; side++)
  {
    const int x = side ? 495 : 0;
    std::unordered_map<uint32_t, int> count;
    uint32_t best = 0;
    int bestCount = 0;
    for (int y = 0; y < 384; y++)
    {
      const uint32_t c = top->data[y * 512 + x] & 0xFFFFFF;
      if (++count[c] > bestCount) { bestCount = count[c]; best = c; }
    }
    m_sideColour[side][0] = (best & 0xFF) / 255.0f;
    m_sideColour[side][1] = ((best >> 8) & 0xFF) / 255.0f;
    m_sideColour[side][2] = ((best >> 16) & 0xFF) / 255.0f;
  }
  settle(true);
}

void CRender2D::DrawSideBars()
{
  const int left = (int)m_xOffset - (int)m_correction;  // where the top layer starts (see Setup2D)
  const int right = left + (int)m_xPixels;
  if (left <= 0 && right >= (int)m_totalXPixels)
    return;

  GLint box[4];
  glGetIntegerv(GL_SCISSOR_BOX, box);
  const GLboolean scissorOn = glIsEnabled(GL_SCISSOR_TEST);
  glEnable(GL_SCISSOR_TEST);
  if (left > 0)
  {
    glScissor(0, 0, left, m_totalYPixels);
    glClearColor(m_sideColour[0][0], m_sideColour[0][1], m_sideColour[0][2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  if (right < (int)m_totalXPixels)
  {
    glScissor(right, 0, m_totalXPixels - right, m_totalYPixels);
    glClearColor(m_sideColour[1][0], m_sideColour[1][1], m_sideColour[1][2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glScissor(box[0], box[1], box[2], box[3]);
  if (!scissorOn)
    glDisable(GL_SCISSOR_TEST);
}

void CRender2D::RenderFrameBottom(void)
{
  Setup2D(true);
  DrawSurface(m_textureIDs[0]);
}

void CRender2D::RenderFrameTop(void)
{
  if (m_coverSides)
    DrawSideBars();
  Setup2D(false);
  DrawSurface(m_textureIDs[1]);
}

void CRender2D::EndFrame(void)
{
}

void CRender2D::AttachDrawBuffers(std::shared_ptr<TileGenBuffer> bottom, std::shared_ptr<TileGenBuffer> top)
{
  m_drawBuffers[0] = bottom;
  m_drawBuffers[1] = top;
}

Result CRender2D::Init(unsigned xOffset, unsigned yOffset, unsigned xRes, unsigned yRes, unsigned totalXRes, unsigned totalYRes, unsigned aaTarget, UpscaleMode upscaleMode)
{
  m_xPixels = xRes;
  m_yPixels = yRes;
  m_xOffset = xOffset;
  m_yOffset = yOffset;
  m_totalXPixels = totalXRes;
  m_totalYPixels = totalYRes;
  m_correction = (UINT32)(((yRes / 384.) * 2.) + 0.5);  // the 2D layer is 2 pixels off the 3D
  m_upscaleMode = (m_xPixels == 496 && m_yPixels == 384) ? UpscaleMode::Nearest : upscaleMode;

  std::string um = "#define UPSCALEMODE " + std::to_string((int)m_upscaleMode) + '\n';
  // Upstream's desktop shaders; gles_compat.cpp converts them to GLSL ES when compiled
  std::string vs = s_vertexShader;
  std::string fs = std::string(s_fragmentShaderHeader) + um + s_fragmentShader;

  m_drawShader.UnloadShaders();
  m_drawShader.LoadShaders(vs.c_str(), fs.c_str());
  m_drawShader.GetUniformLocationMap("tex1");
  m_drawShader.EnableShader();
  glUniform1i(m_drawShader.uniformLocMap["tex1"], 0);
  m_drawShader.DisableShader();

  if (!m_vao)
    glGenVertexArrays(1, &m_vao);
  if (!m_textureIDs[0])
    glGenTextures(2, m_textureIDs);

  for (auto& t : m_textureIDs)
  {
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_upscaleMode == UpscaleMode::Nearest ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_upscaleMode == UpscaleMode::Nearest ? GL_NEAREST : GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 496, 384, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

  return Result::OKAY;
}

CRender2D::CRender2D(const Util::Config::Node& config)
  : m_config(config)
{
  DebugLog("Built Render2D (GLES)\n");
}

CRender2D::~CRender2D(void)
{
  if (m_textureIDs[0])
    glDeleteTextures(2, m_textureIDs);
  if (m_vao)
    glDeleteVertexArrays(1, &m_vao);
}
