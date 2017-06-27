//
// Created by 单强 on 16/5/20.
//

#ifndef __CCGL_IOS_H__
#define __CCGL_IOS_H__

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS

#define	glClearDepth				glClearDepthf
#define glDeleteVertexArrays		glDeleteVertexArraysOES
#define glGenVertexArrays			glGenVertexArraysOES
#define glBindVertexArray			glBindVertexArrayOES
#define glMapBuffer					glMapBufferOES
#define glUnmapBuffer				glUnmapBufferOES

#define GL_DEPTH24_STENCIL8			GL_DEPTH24_STENCIL8_OES
#define GL_WRITE_ONLY				GL_WRITE_ONLY_OES

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#endif // CC_PLATFORM_IOS

#endif // __CCGL_IOS_H__

