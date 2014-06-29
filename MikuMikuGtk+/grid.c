// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <zlib.h>
#include "grid.h"
#include "memory.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GRID_VERTEX
{
	float position[3];
	float color[3];
} GRID_VERTEX;

void InitializeGrid(GRID* grid)
{
	grid->program.model_view_projection_matrix = -1;
	grid->vertices = StructArrayNew(sizeof(GRID_VERTEX), DEFAULT_BUFFER_SIZE);
	grid->indices = Uint32ArrayNew(DEFAULT_BUFFER_SIZE);

	grid->size[0] = 50.0f;
	grid->size[1] = 50.0f;
	grid->size[2] = 50.0f;
	grid->size[3] = 5.0f;

	grid->line_color[0] = 127;
	grid->line_color[1] = 127;
	grid->line_color[2] = 127;

	grid->axis_x_color[0] = 255;
	grid->axis_x_color[1] = 0;
	grid->axis_x_color[2] = 0;

	grid->axis_y_color[0] = 0;
	grid->axis_y_color[1] = 255;
	grid->axis_y_color[2] = 0;

	grid->axis_z_color[0] = 0;
	grid->axis_z_color[1] = 0;
	grid->axis_z_color[2] = 255;

	grid->flags |= GRID_FLAG_VISIBLE;
}

void DrawGrid(GRID* grid, float* matrix)
{
	if((grid->flags & GRID_FLAG_VISIBLE) != 0
		&& (grid->program.base_data.flags & SHADER_PROGRAM_FLAG_LINKED) != 0)
	{
		glEnableClientState(GL_COLOR_ARRAY);

		ShaderProgramBind(&grid->program.base_data);
		glUniformMatrix4fv(grid->program.model_view_projection_matrix, 1, GL_FALSE, matrix);
		GridBindVertexBundle(grid, TRUE);
		//glInterleavedArrays(GL_C3F_V3F, 0, NULL);
		glDrawElements(GL_LINES, grid->num_indices, GL_UNSIGNED_INT, NULL);
		GridReleaseVertexBundle(grid, TRUE);
		ShaderProgramUnbind(&grid->program.base_data);

		glDisableClientState(GL_COLOR_ARRAY);
	}
}

void GridAddShaderFromFile(
	GRID* grid,
	const char* system_path,
	GLuint type
)
{
	char *source;
	FILE *fp = fopen(system_path, "rb");
	if(fp != NULL)
	{
		long size;

		(void)fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		(void)fseek(fp, 0, SEEK_SET);
		source = (char*)MEM_ALLOC_FUNC(size);
		(void)fread(source, 1, size, fp);

		ShaderProgramAddShaderSource(&grid->program.base_data, source, type);

		(void)fclose(fp);
		MEM_FREE_FUNC(source);
	}
	else
	{
		source = LoadSubstituteShaderSource(system_path);

		ShaderProgramAddShaderSource(&grid->program.base_data,
			source, type);

		(void)MEM_FREE_FUNC(source);
	}
}

void GridAddLine(
	GRID* grid,
	const float* from,
	const float* to,
	const uint8* color,
	int* index
)
{
	GRID_VERTEX vertex;

	vertex.color[0] = color[0]/255.0f;
	vertex.color[1] = color[1]/255.0f;
	vertex.color[2] = color[2]/255.0f;

	vertex.position[0] = from[0];
	vertex.position[1] = from[1];
	vertex.position[2] = from[2];

	StructArrayAppend(grid->vertices, &vertex);

	vertex.position[0] = to[0];
	vertex.position[1] = to[1];
	vertex.position[2] = to[2];

	StructArrayAppend(grid->vertices, &vertex);

	Uint32ArrayAppend(grid->indices, (uint32)*index);
	(*index)++;
	Uint32ArrayAppend(grid->indices, (uint32)*index);
	(*index)++;
}

void GridProgramEnableAttributes(GRID_PROGRAM* program)
{
	glEnableVertexAttribArray(GRID_VERTEX_TYPE_POSITION);
	glEnableVertexAttribArray(GRID_VERTEX_TYPE_COLOR);
}

void GridProgramDisableAttributes(GRID_PROGRAM* program)
{
	glDisableVertexAttribArray(GRID_VERTEX_TYPE_POSITION);
	glDisableVertexAttribArray(GRID_VERTEX_TYPE_COLOR);
}

int GridProgramLink(GRID_PROGRAM* program)
{
	int ok;

	glBindAttribLocation(program->base_data.program, GRID_VERTEX_TYPE_POSITION, "inPosition");
	glBindAttribLocation(program->base_data.program, GRID_VERTEX_TYPE_COLOR, "inColor");

	ok = ShaderProgramLink(&program->base_data);
	if(ok != FALSE)
	{
		program->model_view_projection_matrix = glGetUniformLocation(program->base_data.program,
			"modelViewProjectionMatrix");
	}

	return ok;
}

void GridBindVertexBundle(GRID* grid, int bundle)
{
	if(bundle == FALSE || VertexBundleLayoutBind(grid->layout) == FALSE)
	{
		VertexBundleBind(grid->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0);
		VertexBundleBind(grid->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0);
		glVertexAttribPointer(GRID_VERTEX_TYPE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(GRID_VERTEX), 0);
		glVertexAttribPointer(GRID_VERTEX_TYPE_COLOR, 3, GL_FLOAT, GL_FALSE,
			sizeof(GRID_VERTEX), (void*)offsetof(GRID_VERTEX, color));
		GridProgramEnableAttributes(&grid->program);
		VertexBundleBind(grid->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0);
	}
}

void GridReleaseVertexBundle(GRID* grid, int bundle)
{
	if(bundle == FALSE || VertexBundleLayoutUnbind(grid->layout) == FALSE)
	{
		GridProgramDisableAttributes(&grid->program);
		VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
		VertexBundleUnbind(VERTEX_BUNDLE_INDEX_BUFFER);
	}
}

void LoadGrid(GRID* grid, void* project_context)
{
	APPLICATION *mikumiku = (APPLICATION*)project_context;
	char *system_path;
	char *utf8_path;

	(void)memset(&grid->program, 0, sizeof(grid->program));

	DeleteVertexBundle(&grid->bundle);
	grid->bundle = VertexBundleNew();

	DeleteVertexBundleLayout(&grid->layout);
	grid->layout = VertexBundleLayoutNew();

	MakeShaderProgram(&grid->program.base_data);

	utf8_path = g_build_filename(mikumiku->paths.shader_directory_path, "gui/grid.vsh", NULL);
	system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
	GridAddShaderFromFile(grid, system_path, GL_VERTEX_SHADER);
	g_free(utf8_path);
	g_free(system_path);

	utf8_path = g_build_filename(mikumiku->paths.shader_directory_path, "gui/grid.fsh", NULL);
	system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
	GridAddShaderFromFile(grid, system_path, GL_FRAGMENT_SHADER);
	g_free(utf8_path);
	g_free(system_path);

	if(GridProgramLink(&grid->program) != FALSE)
	{
		GRID_VERTEX *vertex;
		float zero[3] = {0};
		float from[3] = {0}, to[3] = {0};
		float width, height;
		float depth;
		float grid_size;
		float x, z;
		int index = 0;

		width = grid->size[0],	height = grid->size[1];
		depth = grid->size[2];
		grid_size = grid->size[3];

		from[2] = -width;
		for(x = -width; x <= width; x += grid_size)
		{
			from[0] = x;
			to[0] = x,	to[2] = (x == 0.0f) ? 0.0f : width;
			GridAddLine(grid, from, to, grid->line_color, &index);
		}

		from[0] = -height;
		to[1] = 0;
		for(z = -height; z <= height; z += grid_size)
		{
			from[2] = z;
			to[0] = (z == 0.0f) ? 0.0f : height,	to[2] = z;
			GridAddLine(grid, from, to, grid->line_color, &index);
		}

		to[0] = width,	to[2] = 0;
		GridAddLine(grid, zero, to, grid->axis_x_color, &index);

		to[0] = 0,	to[1] = height;
		GridAddLine(grid, zero, to, grid->axis_y_color, &index);

		to[1] = 0,	to[2] = depth;
		GridAddLine(grid, zero, to, grid->axis_z_color, &index);

		vertex = (GRID_VERTEX*)grid->vertices->buffer;
		MakeVertexBundle(grid->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0, GL_STATIC_DRAW,
			vertex->position,	sizeof(*vertex) * grid->vertices->num_data);
		MakeVertexBundle(grid->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0, GL_STATIC_DRAW,
			grid->indices->buffer, sizeof(int) * grid->indices->num_data);

		(void)MakeVertexBundleLayout(grid->layout);
		(void)VertexBundleLayoutBind(grid->layout);
		GridBindVertexBundle(grid, FALSE);
		GridProgramEnableAttributes(&grid->program);
		(void)VertexBundleLayoutUnbind(grid->layout);
		GridReleaseVertexBundle(grid, FALSE);

		grid->num_indices = index;
	}
}

#ifdef __cplusplus
}
#endif
