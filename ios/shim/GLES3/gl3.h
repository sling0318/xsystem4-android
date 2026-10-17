/* The iOS SDK ships the OpenGL ES headers under OpenGLES.framework, while
 * xsystem4 includes the Khronos path <GLES3/gl3.h>. This shim lets the engine
 * build unmodified. */
#ifndef SYSTEM4_IOS_GLES3_GL3_H
#define SYSTEM4_IOS_GLES3_GL3_H

#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>

#endif /* SYSTEM4_IOS_GLES3_GL3_H */
