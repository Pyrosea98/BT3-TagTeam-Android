// [android] Budokai Tenkaichi 4 in Dragon Rage (port/bt4): BT3-Recomp's null GL loader of its browser build
// (ps2xRuntime/web/glad_null.cpp, GPLv3): the runtime's own GL renderer is compiled but never run here.
/* [web] The browser build has no desktop GL: the GL loader symbols the (never executed) GL renderer code
   references are null. Generated from the runtime's undefined symbols. */
extern "C" {
// [android] desktop GL entry points that OpenGL ES does not have, called directly by the (never run) GL code
void glClearDepth(double depth) { (void)depth; }
void glGetBufferSubData(unsigned target, long offset, long size, void *data) { (void)target; (void)offset; (void)size; (void)data; }
// Desktop GL entry points WebGL 2 does not have, called directly by the (never executed) GL renderer code.
void glGetTexImage(unsigned target, int level, unsigned format, unsigned type, void *pixels) { (void)target; (void)level; (void)format; (void)type; (void)pixels; }
void glDrawBuffer(unsigned mode) { (void)mode; }
void glGetTexLevelParameteriv(unsigned target, int level, unsigned pname, int *params) { (void)target; (void)level; (void)pname; if (params) *params = 0; }
int gladLoadGL(void *load) { (void)load; return 0; }
int GLAD_GL_ARB_ES3_compatibility = 0;
int GLAD_GL_EXT_texture_compression_s3tc = 0;
int GLAD_GL_KHR_texture_compression_astc_hdr = 0;
int GLAD_GL_KHR_texture_compression_astc_ldr = 0;
void *glad_glActiveTexture = 0;
void *glad_glAttachShader = 0;
void *glad_glBindAttribLocation = 0;
void *glad_glBindBuffer = 0;
void *glad_glBindFramebuffer = 0;
void *glad_glBindRenderbuffer = 0;
void *glad_glBindTexture = 0;
void *glad_glBindVertexArray = 0;
void *glad_glBlendEquation = 0;
void *glad_glBlendEquationSeparate = 0;
void *glad_glBlendFunc = 0;
void *glad_glBlendFuncSeparate = 0;
void *glad_glBlitFramebuffer = 0;
void *glad_glBufferData = 0;
void *glad_glBufferStorage = 0;
void *glad_glBufferSubData = 0;
void *glad_glCheckFramebufferStatus = 0;
void *glad_glClear = 0;
void *glad_glClearColor = 0;
void *glad_glClearDepth = 0;
void *glad_glClientWaitSync = 0;
void *glad_glColorMask = 0;
void *glad_glCompileShader = 0;
void *glad_glCompressedTexImage2D = 0;
void *glad_glCreateProgram = 0;
void *glad_glCreateShader = 0;
void *glad_glCullFace = 0;
void *glad_glDeleteBuffers = 0;
void *glad_glDeleteFramebuffers = 0;
void *glad_glDeleteProgram = 0;
void *glad_glDeleteRenderbuffers = 0;
void *glad_glDeleteShader = 0;
void *glad_glDeleteSync = 0;
void *glad_glDeleteTextures = 0;
void *glad_glDeleteVertexArrays = 0;
void *glad_glDepthFunc = 0;
void *glad_glDepthMask = 0;
void *glad_glDetachShader = 0;
void *glad_glDisable = 0;
void *glad_glDisableVertexAttribArray = 0;
void *glad_glDrawArrays = 0;
void *glad_glDrawArraysInstanced = 0;
void *glad_glDrawBuffers = 0;
void *glad_glDrawElements = 0;
void *glad_glDrawElementsInstanced = 0;
void *glad_glEnable = 0;
void *glad_glEnableVertexAttribArray = 0;
void *glad_glFenceSync = 0;
void *glad_glFramebufferRenderbuffer = 0;
void *glad_glFramebufferTexture2D = 0;
void *glad_glFrontFace = 0;
void *glad_glGenBuffers = 0;
void *glad_glGenerateMipmap = 0;
void *glad_glGenFramebuffers = 0;
void *glad_glGenRenderbuffers = 0;
void *glad_glGenTextures = 0;
void *glad_glGenVertexArrays = 0;
void *glad_glGetAttribLocation = 0;
void *glad_glGetError = 0;
void *glad_glGetFloatv = 0;
void *glad_glGetFramebufferAttachmentParameteriv = 0;
void *glad_glGetIntegerv = 0;
void *glad_glGetProgramInfoLog = 0;
void *glad_glGetProgramiv = 0;
void *glad_glGetShaderInfoLog = 0;
void *glad_glGetShaderiv = 0;
void *glad_glGetTexImage = 0;
void *glad_glGetUniformLocation = 0;
void *glad_glLineWidth = 0;
void *glad_glLinkProgram = 0;
void *glad_glMapBufferRange = 0;
void *glad_glPixelStorei = 0;
void *glad_glPolygonMode = 0;
void *glad_glReadPixels = 0;
void *glad_glRenderbufferStorage = 0;
void *glad_glScissor = 0;
void *glad_glShaderSource = 0;
void *glad_glTexImage2D = 0;
void *glad_glTexParameterf = 0;
void *glad_glTexParameteri = 0;
void *glad_glTexParameteriv = 0;
void *glad_glTexSubImage2D = 0;
void *glad_glUniform1fv = 0;
void *glad_glUniform1i = 0;
void *glad_glUniform1iv = 0;
void *glad_glUniform1uiv = 0;
void *glad_glUniform2fv = 0;
void *glad_glUniform2iv = 0;
void *glad_glUniform2uiv = 0;
void *glad_glUniform3fv = 0;
void *glad_glUniform3iv = 0;
void *glad_glUniform3uiv = 0;
void *glad_glUniform4f = 0;
void *glad_glUniform4fv = 0;
void *glad_glUniform4iv = 0;
void *glad_glUniform4uiv = 0;
void *glad_glUniformMatrix4fv = 0;
void *glad_glUnmapBuffer = 0;
void *glad_glUseProgram = 0;
void *glad_glVertexAttrib1fv = 0;
void *glad_glVertexAttrib2fv = 0;
void *glad_glVertexAttrib3fv = 0;
void *glad_glVertexAttrib4fv = 0;
void *glad_glVertexAttribDivisor = 0;
void *glad_glVertexAttribPointer = 0;
void *glad_glViewport = 0;
}
