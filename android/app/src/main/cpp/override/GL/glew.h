/*
 * Android replacement for <GL/glew.h> as included by upstream renderer code
 * (Src/Graphics/New3D, Src/Graphics/Shader.cpp).
 *
 * Upstream targets desktop OpenGL 4.1 core. The calls it makes all exist in
 * OpenGL ES 3.2 except a few, which are redirected here. Shader sources are
 * converted to GLSL ES 3.20 in gles_compat.cpp when they are handed to GL.
 */

#ifndef SUPER3_GLES_GLEW_SHIM_H
#define SUPER3_GLES_GLEW_SHIM_H

#include <GLES3/gl32.h>

#ifdef __cplusplus
extern "C" {
#endif

// GLSL 4.10 core -> GLSL ES 3.20 (version line, default precisions); logs compile errors
void Super3GLES_ShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
void Super3GLES_CompileShader(GLuint shader);
// Depth/stencil cannot be read back in ES: report "nothing hit" instead of leaving garbage
void Super3GLES_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels);
// ES has only glDrawBuffers, and buffer i must be GL_COLOR_ATTACHMENTi or GL_NONE
void Super3GLES_DrawBuffer(GLenum buf);
// While enabled, glBufferSubData writes without waiting for the GPU. Only for regions
// the GPU has not read yet (see the New3D patch in Scripts/sync-upstream.sh).
void Super3GLES_UnsynchronizedUploads(int enable);
void Super3GLES_BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);

#ifdef __cplusplus
}
#endif

#define glShaderSource  Super3GLES_ShaderSource
#define glCompileShader Super3GLES_CompileShader
#define glReadPixels    Super3GLES_ReadPixels
#define glDrawBuffer    Super3GLES_DrawBuffer
#define glBufferSubData Super3GLES_BufferSubData
#define glClearDepth(d) glClearDepthf((GLfloat)(d))

#endif  // SUPER3_GLES_GLEW_SHIM_H
