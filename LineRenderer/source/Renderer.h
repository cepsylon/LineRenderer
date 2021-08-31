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
		mEndX = mVertical ? mStartX : x;
		mEndY = mHorizontal ? mStartY : y;
	}
	void end_line(int x, int y);
	bool is_drawing_line() const { return mIsDrawingLine; }
	void update_radius(float delta) { mRadius += delta * 0.00001f; }
	void toggle_horizontal() { mHorizontal = !mHorizontal; }
	void toggle_vertical() { mVertical = !mVertical; }
	void toggle_mode() { mLineSimple = !mLineSimple; }

private:
	void create_framebuffer(int width, int height);
	void create_buffers();
	void create_shaders();
	void check_shader_compiled(int shader, const char* shader_name);
	void check_program_linked(int program, const char* program_name);
	void assign_random_color();
	void render_line();
	void bind_plane();
	void bind_line();

	GLuint mFramebuffer = 0u;
	GLuint mFramebufferTexture = 0u;
	GLuint mPlane = 0u;
	GLuint mLine = 0u;
	GLuint mProgramToDisplay = 0u;
	GLuint mProgramSDF = 0u;
	GLuint mProgramSimple = 0u;

	float mHalfWidth = 0.0f;
	float mHalfHeight = 0.0f;
	float mR = 0.0f;
	float mG = 0.0f;
	float mB = 0.0f;
	float mRadius = 0.01f;
	int mStartX = 0;
	int mEndX = 0;
	int mStartY = 0;
	int mEndY = 0;
	bool mIsDrawingLine = false;
	bool mHorizontal = false;
	bool mVertical = false;
	bool mLineSimple = false;
};
