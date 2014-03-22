#include <string.h>
#include "pmd_model.h"
#include "material.h"
#include "vmd_motion.h"
#include "memory.h"
/*
typedef struct _PMD_HEADER
{
	uint8 signature[3];
	float version;
	uint8 name[PMD_MODEL_NAME_SIZE];
	uint8 comment[PMD_MODEL_COMMENT_SIZE];
} PMD_HEADER;

typedef struct _PMD_STATE
{
	PMD_MODEL *model;
	STRUCT_ARRAY *positions;
	STRUCT_ARRAY *rotations;
	UINT32_ARRAY *weights;
} PMD_STATE;
*/
/*
PMD_MODEL* PmdModelNew(void)
{
	PMD_MODEL* ret;
	ret = (PMD_MODEL*)MEM_ALLOC_FUNC(sizeof(*ret));

	ret->rotation_offset[3] = 1;
	ret->edge_color[3] = 255;
	InitializePmdBone(&ret->root_bone);
	ret->root_bone.rotation[3] = 1;

	ret->vertices = StructArrayNew(sizeof(VERTEX_C4UB_V3F), PMD_MODEL_BUFFER_SIZE);
	ret->indices = PointerArrayNew(PMD_MODEL_BUFFER_SIZE);
	ret->materials = StructArrayNew(sizeof(MATERIAL), PMD_MODEL_BUFFER_SIZE);
	ret->bones = StructArrayNew(sizeof(BONE), PMD_MODEL_BUFFER_SIZE);
	ret->iks = StructArrayNew(sizeof(IK), PMD_MODEL_BUFFER_SIZE);
	ret->faces = StructArrayNew(sizeof(FACE), PMD_MODEL_BUFFER_SIZE);
	ret->faces_for_ui = StructArrayNew(sizeof(FACE), PMD_MODEL_BUFFER_SIZE);
	ret->rigid_bodies = StructArrayNew(sizeof(RIGID_BODY), PMD_MODEL_BUFFER_SIZE);
	ret->constraints = StructArrayNew(sizeof(CONSTRAINT), PMD_MODEL_BUFFER_SIZE);

	ret->name2bone = ght_create(PMD_MODEL_BUFFER_SIZE);
	ret->name2face = ght_create(PMD_MODEL_BUFFER_SIZE);
	ret->motions = StructArrayNew(sizeof(VMD_MOTION), PMD_MODEL_BUFFER_SIZE);
	ret->bone_category_names = PointerArrayNew(PMD_MODEL_BUFFER_SIZE);
	ret->bone_category_english_names = PointerArrayNew(PMD_MODEL_BUFFER_SIZE);
	ret->skinning_transform = PointerArrayNew(PMD_MODEL_BUFFER_SIZE);
	ret->bones_for_ui = PointerArrayNew(PMD_MODEL_BUFFER_SIZE);
	ret->fades_for_ui_indices = WordArrayNew(PMD_MODEL_BUFFER_SIZE);
	ret->rotate_bones = PointerArrayNew(PMD_MODEL_BUFFER_SIZE);
	ret->is_ik_simulated = ByteArrayNew(PMD_MODEL_BUFFER_SIZE / 8);
	ret->skinned_vertices = StructArrayNew(sizeof(SKIN_VERTEX), PMD_MODEL_BUFFER_SIZE);
}
*/