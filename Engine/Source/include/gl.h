#pragma once

#define GL_ENABLED

#ifdef OPENGL_ES
	#define GLFW_INCLUDE_ES31
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <EGL/eglplatform.h>
#else
	#ifndef GL_DISABLE_GLEW
		#define GLEW_ENABLED
		#include <GL/glew.h>

		#ifdef _WIN32
			#include <GL/wglew.h>
		#endif

		#if defined(__linux__) && !defined(USE_QT)
			#include <GL/glxew.h>
		#endif
	#endif
#endif

#include <GLFW/glfw3.h>

#include <include/dll_export.h>

// -------------------------------------------------------------------------
// Check for OpenGL Errors
// Returns 1 if an OpenGL error occurred, 0 otherwise.

#ifdef _DEBUG
#define CheckOpenGLError() OpenGL::CheckError(__FILE__, __LINE__)
#define SilentCheckOpenGLError() OpenGL::CheckError(__FILE__, __LINE__, false)
#else
#define CheckOpenGLError()
#define SilentCheckOpenGLError()
#endif

#define OPENGL_ACQUIRE_LOCK OpenGL::AcquireLock();
#define OPENGL_RELEASE_LOCK OpenGL::ReleaseLock();

namespace OpenGL
{
	// -------------------------------------------------------------------------
	// Check for OpenGL Errors
	// Returns 1 if an OpenGL error occurred, 0 otherwise.
	DLLExport int CheckError(const char *file, int line, bool log = true);

	// OpenGL lock
	DLLExport void AcquireLock();
	DLLExport void ReleaseLock();
};