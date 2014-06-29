// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "texture_draw_helper.h"
#include "application.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitializeTextureDrawHelper(TEXTURE_DRAW_HELPER* helper, int width, int height)
{
	(void)memset(helper, 0, sizeof(*helper));

	helper->bundle = VertexBundleNew();
	helper->layout = VertexBundleLayoutNew();
	InitializeTextureDrawHelperProgram(&helper->program);

	helper->width = width,	helper->height = height;
}

void ReleaseTextureDrawHelper(TEXTURE_DRAW_HELPER* helper)
{
	DeleteVertexBundle(&helper->bundle);
	DeleteVertexBundleLayout(&helper->layout);
}

void TextureDrawHelperBindVertexBundle(TEXTURE_DRAW_HELPER* helper, int bundle)
{
	if(bundle == FALSE || VertexBundleLayoutBind(helper->layout) == FALSE)
	{
		VertexBundleBind(helper->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_POSITION);
		glVertexAttribPointer(TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
		VertexBundleBind(helper->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_TEXTURE_COORD);
		glVertexAttribPointer(TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_TEXTURE_COORD, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}
}

void TextureDrawHelperUnbindVertexBundle(TEXTURE_DRAW_HELPER* helper, int bundle)
{
	if(bundle == FALSE || VertexBundleLayoutUnbind(helper->layout) == FALSE)
	{
		VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
	}
}

void LoadTextureDrawHelper(
	TEXTURE_DRAW_HELPER* helper,
	const float base_texture_coord[8],
	char* utf8_vertex_shader_file_path,
	char* utf8_fragment_shader_file_path,
	APPLICATION* application
)
{
	char *system_path;

	MakeShaderProgram(&helper->program.base_data);

	system_path = g_locale_from_utf8(utf8_vertex_shader_file_path, -1, NULL, NULL, NULL);
	LoadTextureDrawHelperProgram(&helper->program, system_path, GL_VERTEX_SHADER);
	g_free(system_path);

	system_path = g_locale_from_utf8(utf8_fragment_shader_file_path, -1, NULL, NULL, NULL);
	LoadTextureDrawHelperProgram(&helper->program, system_path, GL_FRAGMENT_SHADER);
	g_free(system_path);

	if(TextureDrawHelperProgramLink(&helper->program) != FALSE)
	{
		float positions[8] = {0, 0, 1, 0, 1, -1, 0, -1};
		MakeVertexBundle(helper->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_POSITION,
			GL_DYNAMIC_DRAW, positions, sizeof(positions));
		(void)memcpy(positions, base_texture_coord, sizeof(positions));
		MakeVertexBundle(helper->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_TEXTURE_COORD,
			GL_STATIC_DRAW, positions, sizeof(positions));
		(void)MakeVertexBundleLayout(helper->layout);
		(void)VertexBundleLayoutBind(helper->layout);
		TextureDrawHelperBindVertexBundle(helper, FALSE);
		glEnableVertexAttribArray(TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_POSITION);
		glEnableVertexAttribArray(TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_TEXTURE_COORD);
		TextureDrawHelperUnbindVertexBundle(helper, FALSE);
		(void)VertexBundleLayoutUnbind(helper->layout);
		VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
	}
}

void TextureDrawHelperUpdateVertexBuffer(TEXTURE_DRAW_HELPER* helper, float rect[4])
{
	float position[8];
	SetVertices2D(position, rect);
	VertexBundleBind(helper->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_POSITION);
	VertexBundleWrite(helper->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0, sizeof(position), position);
	VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
}

void TextureDrawHelperDraw(
	TEXTURE_DRAW_HELPER* helper,
	float* rect,
	const float* position,
	GLuint texture_name
)
{
	float model_view[16] = {1, 0, 0, position[0], 0, 1, 0, position[1], 0, 0, 1, position[2], 0, 0, 0, 1};
	float projection[16] = IDENTITY_MATRIX;

	if(texture_name == 0)
	{
		return;
	}

	glDisable(GL_DEPTH_TEST);
	TextureDrawHelperUpdateVertexBuffer(helper, rect);
	Ortho(projection, 0, (float)helper->width, 0, (float)helper->height, -1.0f, 1.0f);
	ShaderProgramBind(&helper->program.base_data);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_name);
	MulMatrix4x4(projection, model_view, projection);
	//MulMatrix4x4(model_view, projection, projection);
	TextureDrawHelperProgramSetUniformValues(&helper->program, projection, texture_name);
	TextureDrawHelperBindVertexBundle(helper, TRUE);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	TextureDrawHelperUnbindVertexBundle(helper, TRUE);
	ShaderProgramUnbind(&helper->program.base_data);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_DEPTH_TEST);
}

#ifdef __cplusplus
}
#endif
