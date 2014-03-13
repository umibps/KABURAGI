// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <limits.h>
#include "material.h"
#include "pmx_model.h"
#include "asset_model.h"
#include "memory_stream.h"
#include "utils.h"
#include "model_helper.h"
#include "application.h"
#include "memory.h"

#define PMX_MATERIAL_UNIT_SIZE 65

typedef struct _PMX_METERIAL_UNIT
{
	float diffuse[4];
	float specular[3];
	float shininess;
	float ambient[3];
	uint8 flags;
	float edge_color[4];
	float edge_size;
} PMX_METERIAL_UNIT;

void MaterialRGB3Calculate(MATERIAL_RGB3* rgb)
{
	COPY_VECTOR3(rgb->result, rgb->base);
	MulVector3(rgb->result, rgb->multi);
	PlusVector3(rgb->result, rgb->add);
}

void MaterialRGB3CalculateMultiWeight(MATERIAL_RGB3* rgb, const float* value, FLOAT_T weight)
{
	rgb->multi[0] = (float)(1 - (1 - value[0]) * weight);
	rgb->multi[1] = (float)(1 - (1 - value[1]) * weight);
	rgb->multi[2] = (float)(1 - (1 - value[2]) * weight);
}

void MaterialRGB3CalculateAddWeight(MATERIAL_RGB3* rgb, const float* value, FLOAT_T weight)
{
	rgb->add[0] = (float)(value[0] * weight);
	rgb->add[1] = (float)(value[1] * weight);
	rgb->add[2] = (float)(value[2] * weight);
}

void MaterialRGB3Reset(MATERIAL_RGB3* rgb)
{
	rgb->multi[0] = 1;
	rgb->multi[1] = 1;
	rgb->multi[2] = 1;

	rgb->add[0] = 0;
	rgb->add[1] = 0;
	rgb->add[2] = 0;

	MaterialRGB3Calculate(rgb);
}

void MaterialRGBA3Calculate(MATERIAL_RGBA3* rgba)
{
	COPY_VECTOR4(rgba->result, rgba->base);
	MulVector4(rgba->result, rgba->multi);
	PlusVector4(rgba->result, rgba->add);
}

void MaterialRGBA3CalculateMultiWeight(MATERIAL_RGBA3* rgba, const float* value, FLOAT_T weight)
{
	rgba->multi[0] = (float)(1 - (1 - value[0]) * weight);
	rgba->multi[1] = (float)(1 - (1 - value[1]) * weight);
	rgba->multi[2] = (float)(1 - (1 - value[2]) * weight);
	rgba->multi[2] = (float)(1 - (1 - value[3]) * weight);
}

void MaterialRGBA3CalculateAddWeight(MATERIAL_RGBA3* rgba, const float* value, FLOAT_T weight)
{
	rgba->add[0] = (float)(value[0] * weight);
	rgba->add[1] = (float)(value[1] * weight);
	rgba->add[2] = (float)(value[2] * weight);
	rgba->add[3] = (float)(value[3] * weight);
}

void MaterialRGBA3Reset(MATERIAL_RGBA3* rgba)
{
	rgba->multi[0] = 1;
	rgba->multi[1] = 1;
	rgba->multi[2] = 1;
	rgba->multi[3] = 1;

	rgba->add[0] = 0;
	rgba->add[1] = 0;
	rgba->add[2] = 0;
	rgba->add[3] = 0;

	MaterialRGBA3Calculate(rgba);
}

MATERIAL_INDEX_RANGE DefaultMaterialGetIndexRange(DEFAULT_MATERIAL* material)
{
	MATERIAL_INDEX_RANGE index = {0};

	return index;
}

void InitializeDefaultMaterial(DEFAULT_MATERIAL* material, APPLICATION* application_context)
{
	(void)memset(material, 0, sizeof(*material));

	material->interface_data.index = -1;
	material->interface_data.set_index_range = (void (*)(void*, void*))DummyFuncNoReturn2;
	material->interface_data.get_index_range = (MATERIAL_INDEX_RANGE (*)(void*))DefaultMaterialGetIndexRange;
	material->interface_data.get_toon_texture_index = (int (*)(void*))DummyFuncMinusOneReturn;
	material->interface_data.get_edge_size = (FLOAT_T (*)(void*))DummyFuncFloatTOneReturn;
	material->interface_data.get_ambient = (void (*)(void*, float*))DummyFuncGetZeroVector4;
	material->interface_data.get_diffuse = (void (*)(void*, float*))DummyFuncGetZeroVector4;
	material->interface_data.get_specular = (void (*)(void*, float*))DummyFuncGetZeroVector4;
	material->interface_data.get_edge_color = (void (*)(void*, float*))DummyFuncGetZeroVector4;
	material->interface_data.get_shininess = (float (*)(void*))DummyFuncFloatZeroReturn;
	material->interface_data.get_main_texture_blend = (void (*)(void*, float*))DummyFuncGetWhiteColor;
	material->interface_data.get_sphere_texture_blend = (void (*)(void*, float*))DummyFuncGetWhiteColor;
	material->interface_data.get_toon_texture_blend = (void (*)(void*, float*))DummyFuncGetWhiteColor;
	material->interface_data.has_shadow = (int (*)(void*))DummyFuncZeroReturn;
	material->interface_data.has_shadow_map = (int (*)(void*))DummyFuncZeroReturn;
	material->interface_data.is_self_shadow_enabled = (int (*)(void*))DummyFuncZeroReturn;
	material->interface_data.is_edge_enabled = (int (*)(void*))DummyFuncZeroReturn;

	material->application_context = application_context;
}

static MATERIAL_INDEX_RANGE PmxMaterialGetIndexRange(PMX_MATERIAL* material)
{
	return material->index_range;
}

static void PmxMaterialSetIndexRange(PMX_MATERIAL* material, MATERIAL_INDEX_RANGE* range)
{
	if(memcmp(&material->index_range, range, sizeof(*range)) != 0)
	{
		// ADD_QUEUE_EVENT
		material->index_range = *range;
	}
}

static int PmxMaterialGetToonTextureIndex(PMX_MATERIAL* material)
{
	return material->toon_texture_index;
}

static FLOAT_T PmxMaterialGetEdgeSize(PMX_MATERIAL* material)
{
	return material->edge_size[0] * material->edge_size[1] + material->edge_size[2];
}

static void PmxMaterialGetEdgeColor(PMX_MATERIAL* material, float* color)
{
	COPY_VECTOR4(color, material->edge_color.result);
}

static void PmxMaterialGetAmbient(PMX_MATERIAL* material, float* ambient)
{
	COPY_VECTOR4(ambient, material->ambient.result);
}

static void PmxMaterialGetDiffuse(PMX_MATERIAL* material, float* diffuse)
{
	COPY_VECTOR4(diffuse, material->diffuse.result);
}

static void PmxMaterialGetSpecular(PMX_MATERIAL* material, float* specular)
{
	COPY_VECTOR4(specular, material->specular.result);
}

static void PmxMaterialGetMainTextureBlend(PMX_MATERIAL* material, float* blend)
{
	COPY_VECTOR4(blend, material->main_texture_blend.result);
}

static void PmxMaterialGetSphereTextureBlend(PMX_MATERIAL* material, float* blend)
{
	COPY_VECTOR4(blend, material->sphere_texture_blend.result);
}

static void PmxMaterialGetToonTextureBlend(PMX_MATERIAL* material, float* blend)
{
	COPY_VECTOR4(blend, material->toon_texture_blend.result);
}

static float PmxMaterialGetShininess(PMX_MATERIAL* material)
{
	return material->shininess[0] * material->shininess[1] + material->shininess[2];
}

static int PmxMaterialHasShadow(PMX_MATERIAL* material)
{
	return (material->interface_data.flags & MATERIAL_FLAG_HAS_SHADOW) != 0
		&& (material->interface_data.flags & MATERIAL_FLAG_ENABLE_POINAT_DRAW) == 0;
}

static int PmxMaterialHasShadowMap(PMX_MATERIAL* material)
{
	return (material->interface_data.flags & MATERIAL_FLAG_HAS_SHADOW_MAP) != 0
		&& (material->interface_data.flags & MATERIAL_FLAG_ENABLE_POINAT_DRAW) == 0;
}

static int PmxMaterialIsSelfShadowEnabled(PMX_MATERIAL* material)
{
	return (material->interface_data.flags & MATERIAL_FLAG_ENABLE_SELF_SHADOW) != 0
		&& (material->interface_data.flags & MATERIAL_FLAG_ENABLE_POINAT_DRAW) == 0;
}

static int PmxMaterialIsEdgeEnabled(PMX_MATERIAL* material)
{
	return (material->interface_data.flags & MATERIAL_FLAG_ENABLE_EDGE) != 0
		&& (material->interface_data.flags & MATERIAL_FLAG_ENABLE_POINAT_DRAW) == 0
		&& (material->interface_data.flags & MATERIAL_FLAG_ENABLE_LINE_DRAW) == 0;
}

void InitializePmxMaterial(PMX_MATERIAL* material)
{
	(void)memset(material, 0, sizeof(*material));
	
	material->shininess[1] = 1;
	material->edge_size[1] = 1;

	material->ambient.multi[0] = 1;
	material->ambient.multi[1] = 1;
	material->ambient.multi[2] = 1;
	material->ambient.result[3] = 1;

	material->diffuse.multi[0] = 1;
	material->diffuse.multi[1] = 1;
	material->diffuse.multi[2] = 1;
	material->diffuse.multi[3] = 1;

	material->specular.multi[0] = 1;
	material->specular.multi[1] = 1;
	material->specular.multi[2] = 1;
	material->specular.result[3] = 1;

	material->edge_color.multi[0] = 1;
	material->edge_color.multi[1] = 1;
	material->edge_color.multi[2] = 1;
	material->edge_color.multi[3] = 1;

	material->main_texture_blend.base[0] = 1;
	material->main_texture_blend.base[1] = 1;
	material->main_texture_blend.base[2] = 1;
	material->main_texture_blend.base[3] = 1;
	material->main_texture_blend.multi[0] = 1;
	material->main_texture_blend.multi[1] = 1;
	material->main_texture_blend.multi[2] = 1;
	material->main_texture_blend.multi[3] = 1;
	MaterialRGBA3Calculate(&material->main_texture_blend);

	material->sphere_texture_blend.base[0] = 1;
	material->sphere_texture_blend.base[1] = 1;
	material->sphere_texture_blend.base[2] = 1;
	material->sphere_texture_blend.base[3] = 1;
	material->sphere_texture_blend.multi[0] = 1;
	material->sphere_texture_blend.multi[1] = 1;
	material->sphere_texture_blend.multi[2] = 1;
	material->sphere_texture_blend.multi[3] = 1;
	MaterialRGBA3Calculate(&material->sphere_texture_blend);

	material->toon_texture_blend.base[0] = 1;
	material->toon_texture_blend.base[1] = 1;
	material->toon_texture_blend.base[2] = 1;
	material->toon_texture_blend.base[3] = 1;
	material->toon_texture_blend.multi[0] = 1;
	material->toon_texture_blend.multi[1] = 1;
	material->toon_texture_blend.multi[2] = 1;
	material->toon_texture_blend.multi[3] = 1;
	MaterialRGBA3Calculate(&material->toon_texture_blend);

	material->interface_data.index = -1;
	material->interface_data.get_index_range =
		(MATERIAL_INDEX_RANGE (*)(void*))PmxMaterialGetIndexRange;
	material->interface_data.set_index_range =
		(void (*)(void*, MATERIAL_INDEX_RANGE*))PmxMaterialSetIndexRange;
	material->interface_data.get_toon_texture_index =
		(int (*)(void*))PmxMaterialGetToonTextureIndex;
	material->interface_data.get_edge_size =
		(FLOAT_T (*)(void*))PmxMaterialGetEdgeSize;
	material->interface_data.get_edge_color =
		(void (*)(void*, float*))PmxMaterialGetEdgeColor;
	material->interface_data.get_ambient =
		(void (*)(void*, float*))PmxMaterialGetAmbient;
	material->interface_data.get_diffuse =
		(void (*)(void*, float*))PmxMaterialGetDiffuse;
	material->interface_data.get_specular =
		(void (*)(void*, float*))PmxMaterialGetSpecular;
	material->interface_data.get_shininess =
		(float (*)(void*))PmxMaterialGetShininess;
	material->interface_data.get_main_texture_blend =
		(void (*)(void*, float*))PmxMaterialGetMainTextureBlend;
	material->interface_data.get_sphere_texture_blend =
		(void (*)(void*, float*))PmxMaterialGetSphereTextureBlend;
	material->interface_data.get_toon_texture_blend =
		(void (*)(void*, float*))PmxMaterialGetToonTextureBlend;
	material->interface_data.has_shadow =
		(int (*)(void*))PmxMaterialHasShadow;
	material->interface_data.has_shadow_map =
		(int (*)(void*))PmxMaterialHasShadowMap;
	material->interface_data.is_self_shadow_enabled =
		(int (*)(void*))PmxMaterialIsSelfShadowEnabled;
	material->interface_data.is_edge_enabled =
		(int (*)(void*))PmxMaterialIsEdgeEnabled;
}

int PmxMaterialPreparse(
	uint8 *data,
	size_t *data_size,
	size_t rest,
	PMX_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	int32 num_materials;
	uint8 is_shared_toon_texture;
	int texture_index_size = (int)info->texture_index_size;
	size_t num_texture_index_size;
	char *name_ptr;
	int length;
	int i;

	if(MemRead(&num_materials, sizeof(num_materials), 1, &stream) == 0)
	{
		return FALSE;
	}

	info->materials = &data[stream.data_point];
	num_texture_index_size = texture_index_size * 2;
	for(i=0; i<num_materials; i++)
	{
		// 日本語名
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		// 英語名
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		if(MemSeek(&stream, PMX_MATERIAL_UNIT_SIZE, SEEK_CUR) != 0)
		{
			return FALSE;
		}

		// メインテクスチャ + スフィアマップテクスチャテクスチャ
		if(MemSeek(&stream, (long)num_texture_index_size, SEEK_CUR) != 0)
		{
			return FALSE;
		}

		stream.data_point++;
		is_shared_toon_texture = (data[stream.data_point] == 1) ? TRUE : FALSE;
		stream.data_point++;
		// 共有トゥーンテクスチャ
		if(is_shared_toon_texture != FALSE)
		{
			if(MemSeek(&stream, sizeof(uint8), SEEK_CUR) != 0)
			{
				return FALSE;
			}
		}
		// 独立トゥーンテクスチャのインデックス
		else
		{
			if(MemSeek(&stream, texture_index_size, SEEK_CUR) != 0)
			{
				return FALSE;
			}
		}

		// 自由領域
		if((length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;

		// インデックスデータ
		if(MemSeek(&stream, sizeof(int32), SEEK_CUR) != 0)
		{
			return FALSE;
		}
	}
	
	info->materials_count = num_materials;
	*data_size = stream.data_point;

	return TRUE;
}

int LoadPmxMaterials(
	STRUCT_ARRAY* materials,
	POINTER_ARRAY* textures,
	int expected_indices
)
{
	const int num_materials = (int)materials->num_data;
	const int num_textures = (int)textures->num_data;
	int texture_index;
	PMX_MATERIAL *material_ptr = (PMX_MATERIAL*)materials->buffer;
	PMX_MATERIAL *m;
	int actual_indices = 0;
	int i;

	for(i=0; i<num_materials; i++)
	{
		m = &material_ptr[i];

		// メインテクスチャ
		texture_index = m->main_texture_index;
		if(texture_index >= 0)
		{
			if(texture_index >= num_textures)
			{
				return FALSE;
			}
			else
			{
				m->interface_data.main_texture = textures->buffer[texture_index];
			}
		}

		// スフィアマップテクスチャ
		texture_index = m->sphere_texture_index;
		if(texture_index >= 0)
		{
			if(texture_index >= num_textures)
			{
				return FALSE;
			}
			else
			{
				m->interface_data.sphere_texture = textures->buffer[texture_index];
			}
		}

		// トゥーンテクスチャ
		texture_index = m->toon_texture_index;
		if(texture_index >= 0)
		{
			if(texture_index >= num_textures)
			{
				return FALSE;
			}
			else
			{
				m->interface_data.toon_texture = textures->buffer[texture_index];
			}
		}

		m->interface_data.index = i;
		actual_indices += m->index_range.count;
	}

	return actual_indices == expected_indices;
}

void PmxMaterialRead(
	PMX_MATERIAL* material,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
)
{
	MEMORY_STREAM stream = {data, 0, INT_MAX, 1};
	PMX_METERIAL_UNIT unit;
	char *name_ptr;
	int texture_index_size = (int)info->texture_index_size;
	TEXT_ENCODE *encode = info->encoding;
	int length;
	int32 num_name_size;
	uint8 type;

	// 日本語名
	length = GetTextFromStream((char*)data, &name_ptr);
	material->interface_data.name = EncodeText(encode, name_ptr, length);
	stream.data_point += sizeof(int32) + length;

	// 英語名
	length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr);
	material->interface_data.english_name = EncodeText(encode, name_ptr, length);
	stream.data_point += sizeof(int32) + length;

	(void)MemRead(unit.diffuse, sizeof(unit.diffuse), 1, &stream);
	(void)MemRead(unit.specular, sizeof(unit.specular), 1, &stream);
	(void)MemRead(&unit.shininess, sizeof(unit.shininess), 1, &stream);
	(void)MemRead(unit.ambient, sizeof(unit.ambient), 1, &stream);
	(void)MemRead(&unit.flags, sizeof(unit.flags), 1, &stream);
	(void)MemRead(unit.edge_color, sizeof(unit.edge_color), 1, &stream);
	(void)MemRead(&unit.edge_size, sizeof(unit.edge_size), 1, &stream);

	COPY_VECTOR3(material->ambient.base, unit.ambient);
	MaterialRGB3Calculate(&material->ambient);
	COPY_VECTOR4(material->diffuse.base, unit.diffuse);
	MaterialRGBA3Calculate(&material->diffuse);
	COPY_VECTOR3(material->specular.base, unit.specular);
	MaterialRGB3Calculate(&material->specular);
	COPY_VECTOR4(material->edge_color.base, unit.edge_color);
	MaterialRGBA3Calculate(&material->edge_color);
	material->shininess[0] = unit.shininess;
	material->edge_size[0] = unit.edge_size;
	material->interface_data.flags = unit.flags;

	material->main_texture_index = GetSignedValue(&data[stream.data_point], texture_index_size);
	stream.data_point += texture_index_size;
	material->sphere_texture_index = GetSignedValue(&data[stream.data_point], texture_index_size);
	stream.data_point += texture_index_size;

	type = data[stream.data_point];
	stream.data_point++;
	material->interface_data.sphere_texture_render_mode = (eMATERIAL_SPHERE_TEXTURE_RENDER_MODE)type;
	type = data[stream.data_point];
	stream.data_point++;
	if(type == 1)
	{
		material->interface_data.flags |= MATERIAL_FLAG_USE_SHARED_TOON_TEXTURE;
		type = data[stream.data_point];
		stream.data_point++;
		material->toon_texture_index = AdjustSharedToonTextureIndex(type);
	}
	else
	{
		material->interface_data.flags &= ~(MATERIAL_FLAG_USE_SHARED_TOON_TEXTURE);
		material->toon_texture_index = GetSignedValue(&data[stream.data_point], texture_index_size);
		stream.data_point += texture_index_size;
	}

	length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr);
	stream.data_point += sizeof(int32) + length;

	(void)MemRead(&num_name_size, sizeof(num_name_size), 1, &stream);
	material->index_range.count = num_name_size;

	*data_size = stream.data_point;
}

void PmxMaterialResetMorph(PMX_MATERIAL* material)
{
	MaterialRGB3Reset(&material->ambient);
	MaterialRGBA3Reset(&material->diffuse);
	MaterialRGB3Reset(&material->specular);
	material->shininess[1] = 1;
	material->shininess[2] = 0;
	MaterialRGBA3Reset(&material->edge_color);
	material->edge_size[1] = 1;
	material->edge_size[2] = 0;
	MaterialRGBA3Reset(&material->main_texture_blend);
	MaterialRGBA3Reset(&material->sphere_texture_blend);
	MaterialRGBA3Reset(&material->toon_texture_blend);
}

void PmxMaterialMergeMorph(PMX_MATERIAL* material, MORPH_MATERIAL* morph, FLOAT_T weight)
{
	CLAMPED(weight, 0, 1);
	if(FuzzyZero((float)weight) != 0)
	{
		PmxMaterialResetMorph(material);
	}
	else
	{
		switch(morph->operation)
		{
		case 0:	// 乗算
			MaterialRGB3CalculateMultiWeight(&material->ambient, morph->ambient, weight);
			MaterialRGBA3CalculateMultiWeight(&material->diffuse, morph->diffuse, weight);
			MaterialRGB3CalculateMultiWeight(&material->specular, morph->specular, weight);
			material->shininess[1] = (float)(1 - (1 - morph->shininess) * weight);
			MaterialRGBA3CalculateMultiWeight(&material->edge_color, morph->edge_color, weight);
			material->edge_size[1] = (float)(1 - (1 - morph->edge_size) * weight);
			MaterialRGBA3CalculateMultiWeight(&material->main_texture_blend, morph->texture_weight, weight);
			MaterialRGBA3CalculateMultiWeight(&material->sphere_texture_blend, morph->sphere_texture_weight, weight);
			MaterialRGBA3CalculateMultiWeight(&material->toon_texture_blend, morph->toon_texture_weight, weight);
			break;
		case 1:	// 加算
			MaterialRGB3CalculateAddWeight(&material->ambient, morph->ambient, weight);
			MaterialRGBA3CalculateAddWeight(&material->diffuse, morph->diffuse, weight);
			MaterialRGB3CalculateAddWeight(&material->specular, morph->specular, weight);
			material->shininess[2] = (float)(morph->shininess * weight);
			MaterialRGBA3CalculateAddWeight(&material->edge_color, morph->edge_color, weight);
			material->edge_size[2] = (float)(morph->edge_size * weight);
			MaterialRGBA3CalculateAddWeight(&material->main_texture_blend, morph->texture_weight, weight);
			MaterialRGBA3CalculateAddWeight(&material->sphere_texture_blend, morph->sphere_texture_weight, weight);
			MaterialRGBA3CalculateAddWeight(&material->toon_texture_blend, morph->toon_texture_weight, weight);
			break;
		}

		MaterialRGB3Calculate(&material->ambient);
		MaterialRGBA3Calculate(&material->diffuse);
		MaterialRGB3Calculate(&material->specular);
		MaterialRGBA3Calculate(&material->edge_color);
		MaterialRGBA3Calculate(&material->main_texture_blend);
		MaterialRGBA3Calculate(&material->sphere_texture_blend);
		MaterialRGBA3Calculate(&material->toon_texture_blend);
	}
}

void AssetMaterialSetTextures(ASSET_MATERIAL* material)
{
	struct aiString texture_path;
	if(aiGetMaterialTexture(material->material, aiTextureType_DIFFUSE, 0, &texture_path,
		NULL, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS)
	{
		const char *path = (const char*)texture_path.data;
		const char *separator = "*";
		const char *sph = ".sph";
		const char *spa = ".spa";
		char *texture = Locale2UTF8(path);
		if(strstr(texture, separator) != NULL)
		{
			int num_token;
			char **tokens = SplitString(texture, separator, &num_token);
			if(num_token > 0)
			{
				char *main_texture = tokens[0];
				if(StringCompareIgnoreCase(&main_texture[strlen(main_texture)-4], sph) == 0)
				{
					material->sphere_texture = MEM_STRDUP_FUNC(main_texture);
					material->sphere_texture_render_mode = MATERIAL_MULTI_TEXTURE;
				}
				else
				{
					material->main_texture = MEM_STRDUP_FUNC(main_texture);
				}
			}
			if(num_token >= 2)
			{
				char *sub_texture = tokens[1];
				if(StringCompareIgnoreCase(&sub_texture[strlen(sub_texture)-4], sph) == 0)
				{
					MEM_FREE_FUNC(material->sphere_texture);
					material->sphere_texture = MEM_STRDUP_FUNC(sub_texture);
					material->sphere_texture_render_mode = MATERIAL_MULTI_TEXTURE;
				}
				else if(StringCompareIgnoreCase(&sub_texture[strlen(sub_texture)-4], spa) == 0)
				{
					MEM_FREE_FUNC(material->sphere_texture);
					material->sphere_texture = MEM_STRDUP_FUNC(sub_texture);
					material->sphere_texture_render_mode = MATERIAL_ADD_TEXTURE;
				}
			}
		}
		else if(StringCompareIgnoreCase(&texture[strlen(texture)-4], sph) == 0)
		{
			material->sphere_texture = MEM_STRDUP_FUNC(texture);
			material->sphere_texture_render_mode = MATERIAL_MULTI_TEXTURE;
		}
		MEM_FREE_FUNC(texture);
	}
}

void InitializeAssetMaterial(
	ASSET_MATERIAL* material,
	ASSET_MODEL* model,
	struct aiMaterial* material_ref,
	int num_indices,
	int index,
	void* application_context
)
{
	struct aiColor4D color;
	aiGetMaterialColor(material_ref, AI_MATKEY_COLOR_AMBIENT, &color);
	material->ambient[0] = color.r;
	material->ambient[1] = color.g;
	material->ambient[2] = color.b;
	material->ambient[3] = 1;
	aiGetMaterialColor(material_ref, AI_MATKEY_COLOR_DIFFUSE, &color);
	material->diffuse[0] = color.r;
	material->diffuse[1] = color.g;
	material->diffuse[2] = color.b;
	material->diffuse[3] = 1;
	aiGetMaterialColor(material_ref, AI_MATKEY_COLOR_SPECULAR, &color);
	material->specular[0] = color.r;
	material->specular[1] = color.g;
	material->specular[2] = color.b;
	material->specular[3] = 1;
	aiGetMaterialFloatArray(material_ref, AI_MATKEY_SHININESS, &material->shininess, NULL);
	material->material = material_ref;
	material->num_indices = num_indices;
	material->interface_data.index = index;
	AssetMaterialSetTextures(material);
}
