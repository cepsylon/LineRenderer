#pragma once

#include <GL/glew.h>

class Renderer
{
public:

	void initialize(int width, int height);
	void render();
	void shutdown();

	void start_line(int x, int y)
	{
		mStartX = mEndX = x;
		mStartY = mEndY = y;
		mIsDrawingLine = true;
	}
	void line_endpoint(int x, int y)
	{
		mEndX = x;
		mEndY = y;
	}
	void end_line(int x, int y);
	bool is_drawing_line() const { return mIsDrawingLine; }

private:
	GLuint mFramebuffer = 0u;
	GLuint mFramebufferTexture = 0u;
	GLuint mPlane = 0u;
	GLuint mProgramToDisplay = 0u;
	GLuint mProgramSDF = 0u;

	float mR = 0.0f;
	float mG = 0.0f;
	float mB = 0.0f;
	float mRadius = 0.01f;
	int mStartX = 0;
	int mEndX = 0;
	int mStartY = 0;
	int mEndY = 0;
	bool mIsDrawingLine = false;
};
