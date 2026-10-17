/* iOS simulator smoke test for the questions the cross build cannot answer:
 * does an ES 3.0 context come up on iOS's OpenGL stack, do xsystem4's own
 * shaders compile against it, is FreeType able to read from the bundle, and is
 * the sandbox's writable path usable for saves.
 *
 * Not the engine: this links SDL2 + FreeType and reads the engine's real
 * shader files from the bundle.  Output is line-prefixed with SMOKE: and the
 * CI job fails unless the last line is "SMOKE: OK".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <GLES3/gl3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define TAG "SMOKE: "

static const char glsl_preamble[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n";

static int failures;

static void fail(const char *what, const char *detail)
{
	failures++;
	printf(TAG "FAIL %s: %s\n", what, detail ? detail : "unknown");
	fflush(stdout);
}

static char *read_file(const char *base, const char *rel, size_t *len_out)
{
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s", base, rel);

	FILE *fp = fopen(path, "rb");
	if (!fp) {
		printf(TAG "cannot open %s\n", path);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *buf = malloc(size + 1);
	if (!buf || fread(buf, 1, size, fp) != (size_t)size) {
		fclose(fp);
		free(buf);
		return NULL;
	}
	fclose(fp);
	buf[size] = '\0';
	if (len_out)
		*len_out = size;
	return buf;
}

static GLuint compile(GLenum type, const char *source, size_t len)
{
	/* The engine compiles preamble + file body as one string. */
	char *text = malloc(sizeof(glsl_preamble) + len + 1);
	memcpy(text, glsl_preamble, sizeof(glsl_preamble) - 1);
	memcpy(text + sizeof(glsl_preamble) - 1, source, len);
	text[sizeof(glsl_preamble) - 1 + len] = '\0';

	GLuint shader = glCreateShader(type);
	const GLchar *src = text;
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		GLint loglen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
		char *log = malloc(loglen + 1);
		glGetShaderInfoLog(shader, loglen, NULL, log);
		printf(TAG "shader log:\n%s\n", log);
		free(log);
	}
	free(text);
	return ok ? shader : 0;
}

static void test_gl(const char *base, SDL_Window *window)
{
	int width, height;
	SDL_GL_GetDrawableSize(window, &width, &height);
	printf(TAG "drawable=%dx%d\n", width, height);

	printf(TAG "GL_VERSION  = %s\n", (const char *)glGetString(GL_VERSION));
	printf(TAG "GL_RENDERER = %s\n", (const char *)glGetString(GL_RENDERER));
	printf(TAG "GLSL        = %s\n", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));

	if (strncmp((const char *)glGetString(GL_VERSION), "OpenGL ES 3.0", 13) != 0)
		fail("context", "not an OpenGL ES 3.0 context");

	char *vert = read_file(base, "shaders/render.v.glsl", NULL);
	char *frag = read_file(base, "shaders/fill.f.glsl", NULL);
	if (!vert || !frag) {
		fail("shader-read", "engine shaders missing from bundle");
		return;
	}

	GLuint program = glCreateProgram();
	GLuint v = compile(GL_VERTEX_SHADER, vert, strlen(vert));
	GLuint f = compile(GL_FRAGMENT_SHADER, frag, strlen(frag));
	if (!v || !f) {
		fail("shader-compile", "see log above");
		return;
	}
	glBindAttribLocation(program, 0, "vertex_pos");
	glBindAttribLocation(program, 1, "vertex_uv");
	glAttachShader(program, v);
	glAttachShader(program, f);
	glLinkProgram(program);

	GLint ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		GLint loglen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
		char *log = malloc(loglen + 1);
		glGetProgramInfoLog(program, loglen, NULL, log);
		printf(TAG "link log:\n%s\n", log);
		free(log);
		fail("shader-link", "see log above");
		return;
	}
	printf(TAG "engine shaders compiled and linked\n");

	static const GLfloat triangle[] = {
		-1.f, -1.f, 0.f, 1.f, 0.f, 0.f,
		 3.f, -1.f, 0.f, 1.f, 2.f, 0.f,
		-1.f,  3.f, 0.f, 1.f, 0.f, 2.f,
	};
	GLuint vao, buffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(4 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glUseProgram(program);
	static const GLfloat identity[16] = {
		1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
	};
	glUniformMatrix4fv(glGetUniformLocation(program, "world_transform"), 1, GL_FALSE, identity);
	glUniformMatrix4fv(glGetUniformLocation(program, "view_transform"), 1, GL_FALSE, identity);
	glUniform4f(glGetUniformLocation(program, "color"), 1.f, 0.f, 0.f, 1.f);

	glViewport(0, 0, width, height);
	glClearColor(0.f, 0.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glFinish();

	GLubyte pixel[4] = { 0, 0, 0, 0 };
	glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	printf(TAG "center pixel rgba=%u,%u,%u,%u\n", pixel[0], pixel[1], pixel[2], pixel[3]);
	if (pixel[0] < 200 || pixel[1] > 60 || pixel[2] > 60)
		fail("render", "the red triangle did not reach the framebuffer");
}

static void test_font(const char *base)
{
	char path[1024];
	snprintf(path, sizeof(path), "%s/fonts/VL-Gothic-Regular.ttf", base);

	FT_Library library;
	if (FT_Init_FreeType(&library)) {
		fail("freetype-init", "FT_Init_FreeType failed");
		return;
	}
	FT_Face face;
	if (FT_New_Face(library, path, 0, &face)) {
		fail("freetype-face", path);
		FT_Done_FreeType(library);
		return;
	}
	FT_Set_Pixel_Sizes(face, 0, 24);
	if (FT_Load_Char(face, 'A', FT_LOAD_RENDER)) {
		fail("freetype-glyph", "FT_Load_Char failed");
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		return;
	}
	FT_Bitmap *bitmap = &face->glyph->bitmap;
	unsigned ink = 0;
	for (unsigned y = 0; y < bitmap->rows; y++)
		for (unsigned x = 0; x < bitmap->width; x++)
			if (bitmap->buffer[y * bitmap->pitch + x])
				ink++;

	printf(TAG "font ok family=%s glyph 'A' = %ux%u ink=%u\n",
	       face->family_name ? face->family_name : "?", bitmap->width, bitmap->rows, ink);
	if (ink == 0)
		fail("freetype-glyph", "empty glyph bitmap");

	FT_Done_Face(face);
	FT_Done_FreeType(library);
}

static void test_writable(void)
{
	char *pref = SDL_GetPrefPath("technology.haniwa", "xsystem4-smoke");
	if (!pref) {
		fail("prefpath", SDL_GetError());
		return;
	}
	char path[1024];
	snprintf(path, sizeof(path), "%ssmoke-save.rsm", pref);

	FILE *fp = fopen(path, "wb");
	if (!fp) {
		fail("save-write", path);
		free(pref);
		return;
	}
	fwrite("rsm", 1, 3, fp);
	fclose(fp);

	fp = fopen(path, "rb");
	char buf[4] = { 0 };
	size_t n = fp ? fread(buf, 1, 3, fp) : 0;
	if (fp)
		fclose(fp);
	printf(TAG "writable path ok %s readback=%s\n", pref, n == 3 ? buf : "(none)");
	if (n != 3)
		fail("save-readback", path);
	remove(path);
	free(pref);
}

static void silent_callback(void *userdata, Uint8 *stream, int len)
{
	(void)userdata;
	memset(stream, 0, len);
}

static void test_audio(void)
{
	SDL_AudioSpec want = { 0 }, have = { 0 };
	want.freq = 44100;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = silent_callback;

	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (!dev) {
		printf(TAG "audio unavailable (informational): %s\n", SDL_GetError());
		return;
	}
	SDL_PauseAudioDevice(dev, 0);
	printf(TAG "audio ok driver=%s freq=%d channels=%d samples=%d\n",
	       SDL_GetCurrentAudioDriver(), have.freq, have.channels, have.samples);
	SDL_Delay(200);
	SDL_CloseAudioDevice(dev);
}

int SDL_main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	printf(TAG "start\n");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		fail("SDL_Init", SDL_GetError());
		return 1;
	}
	SDL_version sdl_version;
	SDL_GetVersion(&sdl_version);
	printf(TAG "sdl %d.%d.%d video=%s audio=%s\n",
	       sdl_version.major, sdl_version.minor, sdl_version.patch,
	       SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver());

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_Window *window = SDL_CreateWindow("xsystem4 smoke",
	                                      SDL_WINDOWPOS_UNDEFINED,
	                                      SDL_WINDOWPOS_UNDEFINED,
	                                      1024, 768,
	                                      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!window) {
		fail("CreateWindow", SDL_GetError());
		return 1;
	}

	SDL_GLContext gl = SDL_GL_CreateContext(window);
	if (!gl) {
		fail("GL_CreateContext", SDL_GetError());
		return 1;
	}
	printf(TAG "GL context created\n");

	const char *base = SDL_GetBasePath();
	printf(TAG "basepath=%s\n", base);

	test_gl(base, window);
	test_font(base);
	test_writable();
	test_audio();

	int events = 0;
	Uint64 until = SDL_GetTicks64() + 4000;
	while (SDL_GetTicks64() < until) {
		SDL_Event e;
		while (SDL_PollEvent(&e))
			events++;
		SDL_GL_SwapWindow(window);
		SDL_Delay(16);
	}
	printf(TAG "pumped %d events in 4s\n", events);

	if (failures == 0)
		printf(TAG "OK\n");
	else
		printf(TAG "%d failure(s)\n", failures);

	SDL_GL_DeleteContext(gl);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return failures == 0 ? 0 : 1;
}
