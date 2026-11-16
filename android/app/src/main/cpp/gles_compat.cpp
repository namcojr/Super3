/*
 * Implementation of override/GL/glew.h: runs upstream desktop-GL renderer code
 * on OpenGL ES 3.2. This file uses the real GL entry points (it does not
 * include the shim).
 */

#include <GLES3/gl32.h>
#include <SDL.h>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

extern "C" {

void Super3GLES_ShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)
{
  std::string src;
  for (GLsizei i = 0; i < count; i++)
  {
    if (!string[i])
      continue;
    if (length && length[i] >= 0)
      src.append(string[i], (size_t)length[i]);
    else
      src.append(string[i]);
  }

  static const std::regex versionLine(R"(#version[ \t]+[0-9]+([ \t]+(core|compatibility|es))?)");
  static const char* esHeader =
    "#version 320 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2D;\n"
    "precision highp usampler2D;\n"
    "precision highp isampler2D;\n"
    "precision highp sampler2DArray;\n"
    "precision highp usampler2DArray;\n"
    "precision highp sampler3D;\n";

  std::smatch m;
  if (std::regex_search(src, m, versionLine))
  {
    // #version must be the first thing in a GLSL ES shader
    src = esHeader + src.substr(0, (size_t)m.position(0)) + src.substr((size_t)(m.position(0) + m.length(0)));
  }

  // GLSL ES has no implicit int -> float conversions. Upstream code that relies
  // on them is listed here (exact text). If upstream rewrites one of these lines
  // the fixup stops matching and any new error appears in the compile log.
  static const std::pair<const char*, const char*> fixups[] = {
    // R3DShaderCommon.h texBiLinear(): ivec2 texPos added to a vec2
    { "* texSize + texPos)", "* texSize + vec2(texPos))" },
    // R3DShaderCommon.h microtexture lookup: ivec2 passed as vec2 texSize
    { "ivec2(0), ivec2(128), tex2Pos", "ivec2(0), vec2(128), tex2Pos" },
  };
  for (const auto& f : fixups)
  {
    const size_t fromLen = std::strlen(f.first);
    for (size_t pos = src.find(f.first); pos != std::string::npos; pos = src.find(f.first, pos + std::strlen(f.second)))
      src.replace(pos, fromLen, f.second);
  }

  const GLchar* p = src.c_str();
  glShaderSource(shader, 1, &p, nullptr);
}

void Super3GLES_CompileShader(GLuint shader)
{
  glCompileShader(shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (ok)
    return;

  GLint logLen = 0, srcLen = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
  glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &srcLen);
  std::vector<char> log((size_t)std::max(logLen, 1)), src((size_t)std::max(srcLen, 1));
  glGetShaderInfoLog(shader, (GLsizei)log.size(), nullptr, log.data());
  glGetShaderSource(shader, (GLsizei)src.size(), nullptr, src.data());
  SDL_LogError(SDL_LOG_CATEGORY_RENDER, "GLES shader compile failed:\n%s", log.data());
  // Numbered source, so the line numbers in the log can be followed
  int line = 1;
  for (char* s = src.data(); s && *s; line++)
  {
    char* nl = std::strchr(s, '\n');
    if (nl) *nl = '\0';
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%4d: %s", line, s);
    s = nl ? nl + 1 : nullptr;
  }
}

void Super3GLES_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
{
  if (format == GL_DEPTH_STENCIL && type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
  {
    // [float depth, uint32 stencil in low 8 bits] per pixel: far plane, stencil 0
    auto* out = static_cast<uint32_t*>(pixels);
    const float far = 1.0f;
    for (GLsizei i = 0; i < width * height; i++)
    {
      std::memcpy(&out[i * 2], &far, sizeof(far));
      out[i * 2 + 1] = 0;
    }
    return;
  }
  if (format == GL_DEPTH_COMPONENT)
  {
    if (type == GL_FLOAT)
      for (GLsizei i = 0; i < width * height; i++) static_cast<float*>(pixels)[i] = 1.0f;
    return;
  }
  glReadPixels(x, y, width, height, format, type, pixels);
}

void Super3GLES_DrawBuffer(GLenum buf)
{
  if (buf >= GL_COLOR_ATTACHMENT0 && buf <= GL_COLOR_ATTACHMENT31)
  {
    GLenum bufs[32];
    const unsigned n = buf - GL_COLOR_ATTACHMENT0;
    for (unsigned i = 0; i < n; i++) bufs[i] = GL_NONE;
    bufs[n] = buf;
    glDrawBuffers((GLsizei)(n + 1), bufs);
    return;
  }
  glDrawBuffers(1, &buf);  // GL_BACK or GL_NONE
}

static bool s_unsynchronized = false;

void Super3GLES_UnsynchronizedUploads(int enable)
{
  s_unsynchronized = enable != 0;
}

void Super3GLES_BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
  // Unsynchronised: no GPU wait. (GL_MAP_INVALIDATE_RANGE_BIT makes Adreno copy the buffer, so it's left out.)
  void* dst = (s_unsynchronized && size > 0)
    ? glMapBufferRange(target, offset, size, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT)
    : nullptr;
  if (dst)
  {
    std::memcpy(dst, data, (size_t)size);
    glUnmapBuffer(target);
  }
  else
    glBufferSubData(target, offset, size, data);
}

}  // extern "C"
