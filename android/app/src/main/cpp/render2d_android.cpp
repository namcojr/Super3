/*
 * OpenGL ES 3.0 port of upstream Src/Graphics/Render2D.cpp.
 *
 * Mirrors upstream behaviour line for line, but compiles upstream's own 2D
 * shaders (Src/Graphics/Shaders2D.h) as GLSL ES, so upstream shader changes
 * (upscale filters etc.) are picked up on every sync.
 */

#include "Graphics/Render2D.h"
#include "Graphics/Shaders2D.h"
#include "OSD/Logger.h"

#include <string>

namespace
{
  // Turn an upstream desktop "#version 410 core" shader into GLSL ES 3.00.
  std::string ToGLES(std::string src)
  {
    const std::string desktop = "#version 410 core";
    const std::string gles = "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;";
    size_t pos = src.find(desktop);
    if (pos != std::string::npos)
      src.replace(pos, desktop.size(), gles);
    // GLSL ES requires #version on the very first line.
    pos = src.find("#version");
    if (pos != std::string::npos)
      src.erase(0, pos);
    return src;
  }
}

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
}

void CRender2D::RenderFrameBottom(void)
{
  Setup2D(true);
  DrawSurface(m_textureIDs[0]);
}

void CRender2D::RenderFrameTop(void)
{
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
  std::string vs = ToGLES(s_vertexShader);
  std::string fs = ToGLES(std::string(s_fragmentShaderHeader)) + um + s_fragmentShader;

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
