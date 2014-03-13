#ifndef _INCLUDED_CONSTRAINT_H_
#define _INCLUDED_CONSTRAINT_H_

#include "types.h"

#define CONSTRAINT_NAME_SIZE 20

typedef struct _CONSTRAINT
{
	uint8 name[CONSTRAINT_NAME_SIZE+1];
	void *constraint;
	float position[3];
	float rotation[3];
	float limit_position_from;
	float limit_position_to;
	float limit_rotation_from;
	float limit_rotqtion_to;
	float stiffness[6];
	int body_a;
	int body_b;
} CONSTRAINT;

#endif	// #ifndef _INCLUDED_CONSTRAINT_H_
