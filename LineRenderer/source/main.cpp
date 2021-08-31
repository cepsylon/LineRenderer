#include "Renderer.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <GL/glew.h>
#include <GL/wglew.h>

#include <stdio.h>
#include <assert.h>

static bool windowAlive = true;
static int width = 1024;
static int h_width = width / 2;
static int height = 1024;
static int h_height = height / 2;
static Renderer renderer;

static LRESULT CALLBACK mainWindowCallback(HWND windowHandle, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	if (messageID == WM_CLOSE)
		windowAlive = false;
	else if (messageID == WM_LBUTTONDOWN)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(windowHandle, &pt);
		pt.x -= h_width;
		pt.y -= h_height;
		pt.y = -pt.y;
		if (renderer.is_drawing_line())
			renderer.end_line(pt.x, pt.y);
		else
			renderer.start_line(pt.x, pt.y);
	}
	else if (renderer.is_drawing_line() && messageID == WM_MOUSEMOVE)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(windowHandle, &pt);
		pt.x -= h_width;
		pt.y -= h_height;
		pt.y = -pt.y;
		renderer.line_endpoint(pt.x, pt.y);
	}
	else if (renderer.is_drawing_line() && messageID == WM_MOUSEWHEEL)
	{
		float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
		renderer.update_radius(delta);
	}
	else if (messageID == WM_KEYDOWN)
	{
		if (wParam == VK_CONTROL)
		{
			renderer.toggle_horizontal();
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(windowHandle, &pt);
			pt.x -= h_width;
			pt.y -= h_height;
			pt.y = -pt.y;
			renderer.line_endpoint(pt.x, pt.y);
		}
		else if (wParam == VK_SHIFT)
		{
			renderer.toggle_vertical();
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(windowHandle, &pt);
			pt.x -= h_width;
			pt.y -= h_height;
			pt.y = -pt.y;
			renderer.line_endpoint(pt.x, pt.y);
		}
	}

	return DefWindowProc(windowHandle, messageID, wParam, lParam);
}

int main()
{
	// Create window
	WNDCLASSEX windowClassEx;
	windowClassEx.cbSize = sizeof(WNDCLASSEX);
	windowClassEx.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClassEx.lpfnWndProc = mainWindowCallback;
	windowClassEx.cbClsExtra = 0;
	windowClassEx.cbWndExtra = 0;
	windowClassEx.hInstance = GetModuleHandle(NULL);
	windowClassEx.hIcon = NULL;
	windowClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClassEx.lpszMenuName = NULL;
	windowClassEx.lpszClassName = "mainWindowClass";
	windowClassEx.hIconSm = NULL;
	assert(RegisterClassEx(&windowClassEx));

	// Compute window size
	RECT windowRect = { 0, 0, width, height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);

	// Create window
	HWND windowHandle = CreateWindow(
		windowClassEx.lpszClassName,
		"mainWindow",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		0, 0,
		windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
		NULL, NULL, windowClassEx.hInstance, NULL
	);
	ShowWindow(windowHandle, SW_SHOW);
	UpdateWindow(windowHandle);

	// Initialize OpenGL
	PIXELFORMATDESCRIPTOR pixel_format_descriptor =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		0,                   // Number of bits for the depthbuffer
		0,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	// Choose and set pixel format
	HDC render_device = GetDC(windowHandle);
	int  pixel_format = ChoosePixelFormat(render_device, &pixel_format_descriptor);
	SetPixelFormat(render_device, pixel_format, &pixel_format_descriptor);

	// Create dummy OpenGL context to initialize OpenGL
	HGLRC dummy_gl_context = wglCreateContext(render_device);
	wglMakeCurrent(render_device, dummy_gl_context);
	GLenum error = glewInit();
	if (GLEW_OK != error)
	{
		fprintf(stdout, "Error: %s\n", glewGetErrorString(error));
	}

	// Version for OpenGL
	int attributes[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_FLAGS_ARB, 0,
		0
	};

	// Create actual OpenGL context
	HGLRC glContext = NULL;
	if (wglewIsSupported("WGL_ARB_create_context") == 1)
	{
		glContext = wglCreateContextAttribsARB(render_device, 0, attributes);
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(dummy_gl_context);
		wglMakeCurrent(render_device, glContext);
	}
	else
		fprintf(stdout, "Unsupported OpenGL verion 2.0, please update your drivers");

	// Graphics initialization
	renderer.initialize(width, height);

	// Main loop
	while (windowAlive)
	{
		MSG message;
		while (PeekMessage(&message, windowHandle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		renderer.render();

		SwapBuffers(render_device);
	}

	// Graphics shutdown
	renderer.shutdown();

	// Destroy OpenGL context
	wglDeleteContext(glContext);

	// Destroy window
	DestroyWindow(windowHandle);
	UnregisterClass(windowClassEx.lpszClassName, windowClassEx.hInstance);

	return 0;
}