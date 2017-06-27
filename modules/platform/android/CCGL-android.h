//
// Created by 单强 on 16/4/12.
//

#ifndef __CCGL_ANDROID_H__
#define __CCGL_ANDROID_H__

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#define	glClearDepth				glClearDepthf
#define glDeleteVertexArrays		glDeleteVertexArraysOES
#define glGenVertexArrays			glGenVertexArraysOES
#define glBindVertexArray			glBindVertexArrayOES
#define glMapBuffer					glMapBufferOES
#define glUnmapBuffer				glUnmapBufferOES

#define GL_DEPTH24_STENCIL8			GL_DEPTH24_STENCIL8_OES
#define GL_WRITE_ONLY				GL_WRITE_ONLY_OES

// GL_GLEXT_PROTOTYPES isn't defined in glplatform.h on android ndk r7
// we manually define it here
#include <GLES2/gl2platform.h>
//#ifndef GL_GLEXT_PROTOTYPES
//#define GL_GLEXT_PROTOTYPES 1
//#endif

// normal process
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
// gl2.h doesn't define GLchar on Android
typedef char GLchar;
// android defines GL_BGRA_EXT but not GL_BRGA
#ifndef GL_BGRA
#define GL_BGRA  0x80E1
#endif

// <EGL/egl.h> exists since android 2.3
#include <EGL/egl.h>
#include <EGL/eglext.h>

//declare here while define in EGLView_android.cpp
extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT;
extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT;
extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT;
extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
extern PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
extern PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;

#define glGenVertexArraysOES glGenVertexArraysOESEXT
#define glBindVertexArrayOES glBindVertexArrayOESEXT
#define glDeleteVertexArraysOES glDeleteVertexArraysOESEXT

void initExtensions();

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#endif // __CCGL_ANDROID_H__
