#pragma once

#include <GL/glew.h>

class Renderer
{
public:

	void initialize(int width, int height);
	void render();
	void shutdown();

private:
	GLuint mFramebuffer = 0u;
	GLuint mFramebufferTexture = 0u;
	GLuint mPlane = 0u;
	GLuint mProgramToDisplay = 0u;
};
