#include "parameter.h"
#include "application.h"
#include "technique.h"

void ParameterSetMatrix(PARAMETER* parameter, float matrix[16])
{
	if(parameter->type == PARAMETER_TYPE_FLOAT4x4)
	{
		COPY_MATRIX4x4(parameter->value.matrix, matrix);
	}
}

void MatrixSemanticSetMatrix(
	MATRIX_SEMANTIC* semantic,
	MODEL_INTERFACE* model,
	PARAMETER *parameter,
	int flags
)
{
	if(parameter != NULL)
	{
		float matrix[16];
		GetContextMatrix(matrix, model, semantic->flags | flags, semantic->project);
		ParameterSetMatrix(parameter, matrix);
	}
}

void MatrixSemanticSetMatrices(
	MATRIX_SEMANTIC* semantic,
	MODEL_INTERFACE* model,
	int extra_camera_flags,
	int extra_light_flags
)
{
	MatrixSemanticSetMatrix(semantic, model, &semantic->camera, extra_camera_flags | MATRIX_FLAG_CAMERA_MATRIX);
	MatrixSemanticSetMatrix(semantic, model, &semantic->camera_inversed, extra_camera_flags | MATRIX_FLAG_CAMERA_MATRIX | MATRIX_FLAG_INVERSE_MATRIX);
	MatrixSemanticSetMatrix(semantic, model, &semantic->camera_transposed, extra_camera_flags | MATRIX_FLAG_CAMERA_MATRIX | MATRIX_FLAG_TRANSPOSE_MATRIX);
	MatrixSemanticSetMatrix(semantic, model, &semantic->camera_inverse_transposed, extra_camera_flags | MATRIX_FLAG_CAMERA_MATRIX | MATRIX_FLAG_INVERSE_MATRIX | MATRIX_FLAG_TRANSPOSE_MATRIX);
	MatrixSemanticSetMatrix(semantic, model, &semantic->light, extra_light_flags | MATRIX_FLAG_LIGHT_MATRIX);
	MatrixSemanticSetMatrix(semantic, model, &semantic->light_inversed, extra_camera_flags | MATRIX_FLAG_LIGHT_MATRIX | MATRIX_FLAG_INVERSE_MATRIX);
	MatrixSemanticSetMatrix(semantic, model, &semantic->light_transposed, extra_camera_flags | MATRIX_FLAG_LIGHT_MATRIX | MATRIX_FLAG_TRANSPOSE_MATRIX);
	MatrixSemanticSetMatrix(semantic, model, &semantic->light_inverse_transposed, extra_camera_flags | MATRIX_FLAG_LIGHT_MATRIX | MATRIX_FLAG_INVERSE_MATRIX | MATRIX_FLAG_TRANSPOSE_MATRIX);
}

void EffectSetModelMatrixParameter(
	EFFECT* effect,
	MODEL_INTERFACE* model,
	int extra_camera_flags,
	int extra_light_flags
)
{
	MatrixSemanticSetMatrices(&effect->world, model, extra_camera_flags, extra_light_flags);
	MatrixSemanticSetMatrices(&effect->view, model, extra_camera_flags, extra_light_flags);
	MatrixSemanticSetMatrices(&effect->projection, model, extra_camera_flags, extra_light_flags);
	MatrixSemanticSetMatrices(&effect->world_view, model, extra_camera_flags, extra_light_flags);
	MatrixSemanticSetMatrices(&effect->view_projection, model, extra_camera_flags, extra_light_flags);
	MatrixSemanticSetMatrices(&effect->world_view_projection, model, extra_camera_flags, extra_light_flags);
}

void EffectSetVertexAttributePointer(
	EFFECT* effect,
	eVERTEX_ATTRIBUTE_TYPE vertex_type,
	ePARAMETER_TYPE type,
	size_t stride,
	void* pointer
)
{
	switch(vertex_type)
	{
	case VERTEX_ATTRIBUTE_POSTION:
		glVertexPointer(3, GL_FLOAT, (GLsizei)stride, pointer);
		break;
	case VERTEX_ATTRIBUTE_NORMAL:
		glNormalPointer(GL_FLOAT, (GLsizei)stride, pointer);
		break;
	case VERTEX_ATTRIBUTE_TEXTURE_COORD:
		glTexCoordPointer(2, GL_FLOAT, (GLsizei)stride, pointer);
		break;
	}
}

void EffectActivateVertexAttribute(
	EFFECT* effect,
	eVERTEX_ATTRIBUTE_TYPE vertex_type
)
{
	switch(vertex_type)
	{
	case VERTEX_ATTRIBUTE_POSTION:
		glEnableClientState(GL_VERTEX_ARRAY);
		break;
	case VERTEX_ATTRIBUTE_NORMAL:
		glEnableClientState(GL_NORMAL_ARRAY);
		break;
	case VERTEX_ATTRIBUTE_TEXTURE_COORD:
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		break;
	}
}
