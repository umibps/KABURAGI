// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include "debug_drawer.h"
#include "bullet.h"
#include "application.h"
#include "shader_program.h"
#include "memory.h"

#define LINE_LENGTH 2.0f

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eDEBUG_DRAWER_VERTEX_TYPE
{
	DEBUG_DRAWER_VERTEX_TYPE_POSITION,
	DEBUG_DRAWER_VERTEX_TYPE_COLOR
} eDEBUG_DRAWER_VERTEX_TYPE;

static void DebugDrawerDrawLine(
	DEBUG_DRAWER* drawer,
	const float* from,
	const float* to,
	float* color
)
{
	DEBUG_VERTEX vertex;
	vertex.color[0] = color[0];
	vertex.color[1] = color[1];
	vertex.color[2] = color[2];

	vertex.position[0] = from[0];
	vertex.position[1] = from[1];
	vertex.position[2] = from[2];
	StructArrayAppend(drawer->vertices, &vertex);
	
	vertex.position[0] = to[0];
	vertex.position[1] = to[1];
	vertex.position[2] = to[2];
	StructArrayAppend(drawer->vertices, &vertex);

	Uint32ArrayAppend(drawer->indices, (uint32)drawer->index);
	drawer->index++;
	Uint32ArrayAppend(drawer->indices, (uint32)drawer->index);
	drawer->index++;
}

static void DebugDrawerDrawSphere(
	DEBUG_DRAWER* drawer,
	float* position,
	float radius,
	float* color
)
{
	float from[3], to[3];
	//float x_offset[3] = {radius, 0, 0}:
	//float y_offset[3] = {0, radius, 0};
	//float z_offset[3] = {0, 0, radius};
	//const float basis[] = IDENTITY_MATRIX3x3;
	float start[] = {position[0], position[1], position[2]};
/*
	MulMatrixVector3(x_offset, matrix);
	MulMatrixVector3(y_offset, matrix);
	MulMatrixVector3(z_offset, matrix);
*/
	// XY
	from[0] = start[0] - radius;
	from[1] = start[1];
	from[2] = start[2];
	to[0] = start[0];
	to[1] = start[1] + radius;
	to[2] = start[2];
	DebugDrawerDrawLine(drawer, from, to, color);
	from[0] = start[0];
	from[1] = start[1] + radius;
	//from[2] = start[2];
	to[0] = start[0] + radius;
	to[1] = start[1];
	//to[2] = start[2]
	DebugDrawerDrawLine(drawer, from, to, color);
	from[0] = start[0] + radius;
	from[1] = start[1];
	//from[2] = start[2];
	to[0] = start[0];
	to[1] = start[1] - radius;
	//to[2] = start[2];
	DebugDrawerDrawLine(drawer, from, to, color);
	from[0] = start[0];
	from[1] = start[1] - radius;
	//from[2] = start[2];
	to[0] = start[0] - radius;
	to[1] = start[1];
	//to[2] = start[2];
	DebugDrawerDrawLine(drawer, from, to, color);

	// XZ
	from[0] = start[0] - radius;
	from[1] = start[1];
	from[2] = start[2];
	to[0] = start[0];
	to[1] = start[1];
	to[2] = start[2] + radius;
	DebugDrawerDrawLine(drawer, from, to, color);
	from[0] = start[0];
	//from[1] = start[1];
	from[2] = start[2] + radius;
	to[0] = start[0] + radius;
	//to[1] = start[1];
	to[2] = start[2];
	DebugDrawerDrawLine(drawer, from, to, color);
	from[0] = start[0] + radius;
	//from[1] = start[1];
	from[2] = start[2];
	to[0] = start[0];
	//to[1] = start[1];
	to[2] = start[2] - radius;
	DebugDrawerDrawLine(drawer, from, to, color);
	from[0] = start[0];
	//from[1] = start[1];
	from[2] = start[2] - radius;
	to[0] = start[0] - radius;
	//to[1] = start[1];
	to[2] = start[2];
	DebugDrawerDrawLine(drawer, from, to, color);

	// YZ
	from[0] = start[0];
	from[1] = start[1] - radius;
	from[2] = start[2];
	to[0] = start[0];
	to[1] = start[1];
	to[2] = start[2] + radius;
	DebugDrawerDrawLine(drawer, from, to, color);
	//from[0] = start[0];
	from[1] = start[1];
	from[2] = start[2] + radius;
	//to[0] = start[0];
	to[1] = start[1] + radius;
	to[2] = start[2];
	DebugDrawerDrawLine(drawer, from, to, color);
	//from[0] = start[0];
	from[1] = start[1] + radius;
	from[2] = start[2];
	//to[0] = start[0];
	to[1] = start[1];
	to[2] = start[2] - radius;
	DebugDrawerDrawLine(drawer, from, to, color);
	//from[0] = start[0];
	from[1] = start[1];
	from[2] = start[2] - radius;
	//to[0] = start[0];
	to[1] = start[1] - radius;
	to[2] = start[2];
	DebugDrawerDrawLine(drawer, from, to, color);
}

void InitializeDebugDrawer(DEBUG_DRAWER* drawer, void* project_context)
{
	drawer->program.model_view_projection_matrix = -1;

	drawer->vertices = StructArrayNew(sizeof(DEBUG_VERTEX), DEFAULT_BUFFER_SIZE*4);
	drawer->indices = Uint32ArrayNew(DEFAULT_BUFFER_SIZE*4);
	drawer->project_context = project_context;
	drawer->bundle = VertexBundleNew();
	drawer->layout = VertexBundleLayoutNew();
	drawer->flags |= DEBUG_DRAW_FLAG_VISIBLE;

	drawer->drawer = BtDebugDrawNew(drawer,
		(void (*)(void*, const float*, const float*, float*))DebugDrawerDrawLine,
		(void (*)(void*, float*, float, float*))DebugDrawerDrawSphere);
}

void DebugDrawBindVertexBundle(DEBUG_DRAWER* drawer, int bundle)
{
	if(bundle == 0 || VertexBundleLayoutBind(drawer->layout) != 0)
	{
		VertexBundleBind(drawer->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0);
		glVertexAttribPointer(DEBUG_DRAWER_VERTEX_TYPE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(DEBUG_VERTEX), NULL);
		glVertexAttribPointer(DEBUG_DRAWER_VERTEX_TYPE_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(DEBUG_VERTEX),
			(void*)offsetof(DEBUG_VERTEX, color));
		VertexBundleBind(drawer->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0);
	}
}

void DebugDrawBind(DEBUG_DRAWER* drawer)
{
	VertexBundleBind(drawer->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0);
	VertexBundleBind(drawer->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0);
}

void DebugDrawUnbindVertexBundle(DEBUG_DRAWER* drawer)
{
	VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
	VertexBundleUnbind(VERTEX_BUNDLE_INDEX_BUFFER);
}

void DebugDrawerBeginDraw(DEBUG_DRAWER* drawer, MODEL_INTERFACE* model)
{
	float world_view_projection[16];
	PROJECT *project;

	project = (PROJECT*)drawer->project_context;
	GetContextMatrix(world_view_projection, model,
		MATRIX_FLAG_CAMERA_MATRIX | MATRIX_FLAG_WORLD_MATRIX
		| MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX,
		project
	);
	glUseProgram(drawer->program.base_data.program);
	glUniformMatrix4fv(drawer->program.model_view_projection_matrix,
		1, GL_FALSE, world_view_projection);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	DebugDrawBindVertexBundle(drawer, FALSE);
	glEnableVertexAttribArray(DEBUG_DRAWER_VERTEX_TYPE_POSITION);
	glEnableVertexAttribArray(DEBUG_DRAWER_VERTEX_TYPE_COLOR);
}

void DebugDrawerReleaseVertexBundle(DEBUG_DRAWER* drawer, int bundle)
{
	if(bundle == FALSE)
	{
		VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
		VertexBundleUnbind(VERTEX_BUNDLE_INDEX_BUFFER);
	}
}

void DebugDrawerFlushDraw(DEBUG_DRAWER* drawer)
{
	size_t num_indices = drawer->indices->num_data;
	if(num_indices > 0)
	{
		glBufferData(GL_ARRAY_BUFFER, sizeof(DEBUG_VERTEX) * num_indices,
			(void*)drawer->vertices->buffer, GL_DYNAMIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * num_indices,
			(void*)drawer->indices->buffer, GL_DYNAMIC_DRAW);
		DebugDrawUnbindVertexBundle(drawer);
		DebugDrawBindVertexBundle(drawer, FALSE);
		glEnableVertexAttribArray(DEBUG_DRAWER_VERTEX_TYPE_POSITION);
		glEnableVertexAttribArray(DEBUG_DRAWER_VERTEX_TYPE_COLOR);
		glDrawElements(GL_LINES, (GLsizei)num_indices, GL_UNSIGNED_INT, NULL);
	}

	drawer->vertices->num_data = 0;
	drawer->indices->num_data = 0;
	drawer->index = 0;
	glUseProgram(0);
	glDisableClientState(GL_COLOR_ARRAY);
	DebugDrawerReleaseVertexBundle(drawer, FALSE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
}

void DebugDrawerDrawWorld(DEBUG_DRAWER* drawer, WORLD* world, unsigned int flags)
{
	if((drawer->program.base_data.flags & SHADER_PROGRAM_FLAG_LINKED) != 0)
	{
		drawer->flags = flags;
		SetBtWorldDebugDrawer(world->world, drawer->drawer);
		SetBtDebugMode(drawer->drawer, drawer->flags);
		DebugDrawerBeginDraw(drawer, NULL);
		BtDebugDrawWorld(world->world);
		DebugDrawerFlushDraw(drawer);
		SetBtDebugMode(drawer->drawer, 0);
		SetBtWorldDebugDrawer(world->world, world->debug_drawer->drawer);
	}
}

int DebugDrawerLink(DEBUG_DRAWER* drawer)
{
	int ok;
	glBindAttribLocation(drawer->program.base_data.program, DEBUG_DRAWER_VERTEX_TYPE_POSITION, "inPosition");
	glBindAttribLocation(drawer->program.base_data.program, DEBUG_DRAWER_VERTEX_TYPE_COLOR, "inColor");
	ok = ShaderProgramLink(&drawer->program.base_data);
	if(ok != FALSE)
	{
		drawer->program.model_view_projection_matrix = glGetUniformLocation(drawer->program.base_data.program,
			"modelViewProjectionMatrix");
	}

	return ok;
}

void DebugDrawerAddShaderFromFile(
	DEBUG_DRAWER* drawer,
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

		ShaderProgramAddShaderSource(&drawer->program.base_data, source, type);

		(void)fclose(fp);
	}
	else
	{
		source = LoadSubstituteShaderSource(system_path);
		ShaderProgramAddShaderSource(&drawer->program.base_data,
			source, type);
	}

	MEM_FREE_FUNC(source);
}

void DebugDrawerLoad(DEBUG_DRAWER* drawer)
{
	APPLICATION *mikumiku;
	PROJECT *project;
	char *system_path;
	char *utf8_path;

	project = (PROJECT*)drawer->project_context;
	mikumiku = project->application_context;

	drawer->program.base_data.program = glCreateProgram();

	utf8_path = g_build_filename(mikumiku->paths.shader_directory_path, "gui/grid.vsh", NULL);
	system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
	DebugDrawerAddShaderFromFile(drawer, system_path, GL_VERTEX_SHADER);
	g_free(utf8_path);
	g_free(system_path);

	utf8_path = g_build_filename(mikumiku->paths.shader_directory_path, "gui/grid.fsh", NULL);
	system_path = g_locale_from_utf8(utf8_path, -1, NULL, NULL, NULL);
	DebugDrawerAddShaderFromFile(drawer, system_path, GL_FRAGMENT_SHADER);
	g_free(utf8_path);
	g_free(system_path);

	if(DebugDrawerLink(drawer) != FALSE)
	{
		MakeVertexBundle(drawer->bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0, GL_DYNAMIC_DRAW,
			0, 0);
		MakeVertexBundle(drawer->bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0, GL_DYNAMIC_DRAW,
			0, 0);
		(void)MakeVertexBundleLayout(drawer->layout);
		VertexBundleLayoutBind(drawer->layout);
		DebugDrawBindVertexBundle(drawer, FALSE);
		glEnableVertexAttribArray(DEBUG_DRAWER_VERTEX_TYPE_POSITION);
		glEnableVertexAttribArray(DEBUG_DRAWER_VERTEX_TYPE_COLOR);
		//VertexBundleLayoutUnbind(drawer->layout);
		DebugDrawerReleaseVertexBundle(drawer, FALSE);
	}
}

void DebugDrawerDrawBone(
	DEBUG_DRAWER* drawer,
	BONE_INTERFACE* bone,
	POINTER_ARRAY* selected,
	POINTER_ARRAY* ik,
	int skip_draw_line
)
{
#define SPHERE_RADIUS 0.2f
	void *transform = ((PROJECT*)drawer->project_context)->application_context->transform;
	float dest[3];
	float origin[3];
	float position[3];
	float color[3] = {0, 0, 0};

	if(bone == NULL || bone->is_visible(bone) == 0)
	{
		return;
	}

	bone->get_destination_origin(bone, dest);
	BtTransformMultiVector3(bone->local_transform, dest, dest);
	bone->get_world_transform(bone, transform);
	BtTransformGetOrigin(transform, origin);
	BtTransformMulti(bone->local_transform, transform, transform);
	BtTransformGetOrigin(transform, position);

	if(PointerArrayDoesCointainData(selected, bone) != FALSE)
	{
		BtDebugDrawSphere(drawer->drawer, position, SPHERE_RADIUS, color);
		// To red
		color[0] = 1;
	}
	else if(bone->has_fixed_axis(bone) != FALSE)
	{
		color[0] = 1,	color[2] = 1;
		BtDebugDrawSphere(drawer->drawer, position, SPHERE_RADIUS, color);
		color[0] = 0;
	}
	else if(PointerArrayDoesCointainData(ik, bone) != FALSE)
	{
		color[0] = 1,	color[1] = 0.75f;
		BtDebugDrawSphere(drawer->drawer, position, SPHERE_RADIUS,color);
	}
	else
	{
		color[2] = 1;
		BtDebugDrawSphere(drawer->drawer, position, SPHERE_RADIUS, color);
	}

	if(skip_draw_line == FALSE)
	{
#define CONE_RADIUS 0.05f
		static const float basis[9] = IDENTITY_MATRIX3x3;
		void *trans = BtTransformNew(basis);
		float draw_origin[3] = {CONE_RADIUS, 0, 0};
		float from[3];
		BtTransformSetOrigin(trans, draw_origin);
		BtTransformMultiVector3(trans, position, from);
		DebugDrawerDrawLine(drawer, from, dest, color);
		draw_origin[0] = - CONE_RADIUS;
		BtTransformSetOrigin(trans, draw_origin);
		BtTransformMultiVector3(trans, position, from);
		DebugDrawerDrawLine(drawer, from, dest, color);
		DeleteBtTransform(trans);
	}
}

BONE_INTERFACE* DebugDrawerFindSpecialBone(DEBUG_DRAWER* drawer, MODEL_INTERFACE* model)
{
#define ROOT_STR "Root"
	LABEL_INTERFACE **labels;
	LABEL_INTERFACE *label;
	BONE_INTERFACE *bone = NULL;
	int num_labels;
	int num_children;
	int i;

	labels = (LABEL_INTERFACE**)model->get_labels(model, &num_labels);
	for(i=0; i<num_labels; i++)
	{
		label = labels[i];
		num_children = label->get_count(label);
		if(label->special != FALSE)
		{
			if(num_children > 0 && strcmp(ROOT_STR, label->name) == 0)
			{
				bone = (BONE_INTERFACE*)label->get_bone(label, 0);
				break;
			}
		}
	}

	MEM_FREE_FUNC(labels);
	return bone;
}

void DebugDrawerDrawBones(DEBUG_DRAWER* drawer, MODEL_INTERFACE* model, POINTER_ARRAY* selected_bones)
{
	void *transform = ((PROJECT*)drawer->project_context)->application_context->transform;
	BONE_INTERFACE **bones;
	POINTER_ARRAY *bones_for_ik;
	POINTER_ARRAY *linked_bones;
	BONE_INTERFACE *bone;
	BONE_INTERFACE *effector;
	BONE_INTERFACE *linked_bone;
	BONE_INTERFACE *special_bone;
	float origin[3] = {0};
	float bone_origin[3];
	int num_bones;
	int num_links;
	int skip_draw_line;
	int i, j;

	if(model == NULL || (drawer->flags & DEBUG_DRAW_FLAG_VISIBLE) == 0
		|| (drawer->program.base_data.flags & SHADER_PROGRAM_FLAG_LINKED) == 0)
	{
		return;
	}

	bones = (BONE_INTERFACE**)model->get_bones(model, &num_bones);
	bones_for_ik = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	linked_bones = PointerArrayNew(DEFAULT_BUFFER_SIZE);

	DebugDrawerBeginDraw(drawer, model);

	for(i=0; i<num_bones; i++)
	{
		bone = bones[i];
		if(bone->has_inverse_kinematics(bone) != FALSE)
		{
			linked_bones->num_data = 0;
			PointerArrayAppend(bones_for_ik, bone);
			effector = bone->effector_bone;
			if(effector != NULL)
			{
				PointerArrayAppend(bones_for_ik, bone);
			}
			bone->get_effector_bones(bone, linked_bones);
			num_links = (int)linked_bones->num_data;
			for(j=0; j<num_links; j++)
			{
				linked_bone = (BONE_INTERFACE*)linked_bones->buffer[j];
				PointerArrayAppend(bones_for_ik, linked_bone);
			}
		}
	}

	special_bone = DebugDrawerFindSpecialBone(drawer, model);
	if(special_bone != NULL)
	{
		special_bone->get_world_transform(special_bone, transform);
		BtTransformGetOrigin(transform, origin);
	}

	for(i=0; i<num_bones; i++)
	{
		bone = bones[i];
		if(bone->is_interactive(bone) == 0)
		{
			continue;
		}
		bone->get_destination_origin(bone, bone_origin);
		skip_draw_line = CompareVector3(bone_origin, origin);
		DebugDrawerDrawBone(drawer, bone, selected_bones, bones_for_ik, skip_draw_line);
	}

	DebugDrawerFlushDraw(drawer);

	MEM_FREE_FUNC(bones);
	PointerArrayDestroy(&bones_for_ik, NULL);
	PointerArrayDestroy(&linked_bones, NULL);
}

void DebugDrawerDrawBoneTransform(DEBUG_DRAWER* drawer, BONE_INTERFACE* bone, MODEL_INTERFACE* model, int mode)
{
	if((drawer->flags & DEBUG_DRAW_FLAG_VISIBLE) == 0 || bone == NULL
		|| (drawer->program.base_data.flags & SHADER_PROGRAM_FLAG_LINKED) == 0 || bone->has_fixed_axis(bone) == FALSE)
	{
		return;
	}

	DebugDrawerBeginDraw(drawer, model);
	if(mode == 'V')
	{
		void *transform = ((PROJECT*)drawer->project_context)->application_context->transform;
		float origin[3];
		float view_matrix[16];
		float *x = &view_matrix[0], *y = &view_matrix[4], *z = &view_matrix[8];
		float to[3];
		float color[3] = {1, 0, 0};

		bone->get_world_transform(bone, transform);
		BtTransformGetOrigin(transform, origin);

		GetContextMatrix(view_matrix, model,
			MATRIX_FLAG_CAMERA_MATRIX | MATRIX_FLAG_VIEW_MATRIX,
				(PROJECT*)drawer->project_context);

		BtTransformMultiVector3(transform, x, to);
		ScaleVector3(to, LINE_LENGTH);
		DebugDrawerDrawLine(drawer, origin, to, color);

		color[0] = 0,	color[1] = 1;
		BtTransformMultiVector3(transform, y, to);
		ScaleVector3(to, LINE_LENGTH);
		DebugDrawerDrawLine(drawer, origin, to, color);

		color[1] = 0,	color[2] = 1;
		BtTransformMultiVector3(transform, z, to);
		ScaleVector3(to, LINE_LENGTH);
		DebugDrawerDrawLine(drawer, origin, to, color);
	}
	else if(mode == 'L')
	{
		if(bone->has_local_axis(bone) != FALSE)
		{
			void *transform = ((PROJECT*)drawer->project_context)->application_context->transform;
			float axes[9] = IDENTITY_MATRIX3x3;
			float *x = &axes[0], *y = &axes[3], *z = &axes[6];
			float origin[3];
			float to[3];
			float color[3] = {1, 0, 0};

			bone->get_world_transform(bone, transform);
			BtTransformGetOrigin(transform, origin);

			bone->get_local_axes(bone, axes);

			BtTransformMultiVector3(transform, x, to);
			ScaleVector3(to, LINE_LENGTH);
			DebugDrawerDrawLine(drawer, origin, to, color);

			color[0] = 0,	color[1] = 1;
			BtTransformMultiVector3(transform, y, to);
			ScaleVector3(to, LINE_LENGTH);
			DebugDrawerDrawLine(drawer, origin, to, color);

			color[1] = 0,	color[2] = 1;
			BtTransformMultiVector3(transform, z, to);
			ScaleVector3(to, LINE_LENGTH);
			DebugDrawerDrawLine(drawer, origin, to, color);
		}
		else
		{
			void *transform = ((PROJECT*)drawer->project_context)->application_context->transform;
			float origin[3];
			float vector[3] = {LINE_LENGTH, 0, 0};
			float color[3] = {1, 0, 0};
			float to[3];

			bone->get_world_transform(bone, transform);
			BtTransformGetOrigin(transform, origin);

			BtTransformMultiVector3(transform, vector, to);
			DebugDrawerDrawLine(drawer, origin, to, color);

			color[0] = 0,	color[1] = 1;
			vector[0] = 0,	vector[1] = LINE_LENGTH;
			BtTransformMultiVector3(transform, vector, to);
			DebugDrawerDrawLine(drawer, origin, to, color);

			color[1] = 0,	color[2] = 1;
			vector[1] = 0,	vector[2] = LINE_LENGTH;
			BtTransformMultiVector3(transform, vector, to);
			DebugDrawerDrawLine(drawer, origin, to, color);
		}
	}
	else
	{
		void *transform = ((PROJECT*)drawer->project_context)->application_context->transform;
		float origin[3];
		float color[3] = {1, 0, 0};
		float to[3];

		bone->get_world_transform(bone, transform);
		BtTransformGetOrigin(transform, origin);

		to[0] = origin[0] + LINE_LENGTH;
		to[1] = origin[1];
		to[2] = origin[2];
		DebugDrawerDrawLine(drawer, origin, to, color);

		color[0] = 0,	color[1] = 1;
		to[0] = origin[0];
		to[1] = origin[1] + LINE_LENGTH;
		to[2] = origin[2];
		DebugDrawerDrawLine(drawer, origin, to, color);

		color[1] = 0,	color[2] = 1;
		to[0] = origin[0];
		to[1] = origin[1];
		to[2] = origin[2] + LINE_LENGTH;
		DebugDrawerDrawLine(drawer, origin, to, color);
	}

	DebugDrawerFlushDraw(drawer);
}

void DebugDrawerDrawMovableBone(DEBUG_DRAWER* drawer, BONE_INTERFACE* bone, MODEL_INTERFACE* model)
{
	float position[3];
	const float color[] = {0, 1, 1};
	void *transform = ((PROJECT*)drawer->project_context)->application_context->transform;
	if(bone == NULL || bone->is_movable(bone) == FALSE || (drawer->program.base_data.flags & SHADER_PROGRAM_FLAG_LINKED) == 0)
	{
		return;
	}
	DebugDrawerBeginDraw(drawer, model);
	bone->get_world_transform(bone, transform);
	BtTransformGetOrigin(transform, position);
	BtDebugDrawSphere(drawer->drawer, position, 0.5f, color);
	DebugDrawerFlushDraw(drawer);
}

#ifdef __cplusplus
}
#endif
