#include <string.h>
#include <limits.h>
#include <math.h>
#include "application.h"
#include "trace.h"
#include "memory.h"

#define MOD(I, L) (((I) >= 0) ? ((I) % (L)) : (((I) % (L)) + (L)))
#define SIGN(X) (((X) < 0) ? -1 : (((X) > 0) ? 1 : 0))
#define XPROD(P1, P2) ((P1).x*(P2).y - (P1).y*(P2).x)
#define CPROD(P0, P1, P2, P3) (((P1).x - (P0).x)*((P3).y - (P2).y) - ((P3).x - (P2).x)*((P1).y - (P0).y))
#define IPROD(P0, P1, P2) (((P1).x - (P0).x)*((P2).x - (P0).x) - ((P1).y - (P0).y)*((P2).y - (P0).y))
#define IPROD1(P0, P1, P2, P3) (((P1).x - (P0).x)*((P3).x - (P2).x) - ((P1).y - (P0).y)*((P3).y - (P2).y))
#define CYCLIC(A, B, C) (((A) <= (C)) ? ((A) <= (B) && (B) < (C)) : ((A) <= (B) || (B) < (C)))
#define INFTY 10000000
#define COS179 -0.999847695156
#define FLOOR_DIVIDE(A, N) (((A) >= (N)) ? ((A) / (N)) : (-1 - (-1 - (A)) / (N)))
#define SQ(X) ((X)*(X))
#define INTERVAL(RESULT, LAMBDA, A, B) ((RESULT)->x = (A).x + (LAMBDA) * ((B).x - (A).x), (RESULT)->y = (A).y + (LAMBDA) * ((B).y - (A).y))
#define DPARA(P0, P1, P2) (((P1).x-(P0).x)*((P2).y-(P0).y) - ((P2).x -(P0).x)*((P1).y-(P0).y))
#define DDENOMINATOR(P0, P2) (SIGN((P2).x-(P0).x)*((P2).x-(P0).x) - (- SIGN((P2).y-(P0).y)*((P2).y-(P0).y)))
#define DDIST(P, Q) (sqrt(SQ((P).x-(Q).x)+SQ((P).y-(Q).y)))
#define BEZIER(RESULT, T, P0, P1, P2, P3) ((RESULT)->x = (1-(T))*(1-(T))*(1-(T))*(P0).x + 3*((1-(T))*(1-(T))*(T))*(P1).x + 3*((T)*(T)*(1-(T)))*(P2).x + (T)*(T)*(T)*(P3).x, \
	(RESULT)->y = (1-(T))*(1-(T))*(1-(T))*(P0).y + 3*((1-(T))*(1-(T))*(T))*(P1).y + 3*((T)*(T)*(1-(T)))*(P2).y + (T)*(T)*(T)*(P3).y)

#define DEFAULT_MINIMUM_DISTANCE (2)

typedef enum _eBITMAP_TRACE_TAG
{
	BITMAP_TRACE_TAG_CURVE_TO = 1,
	BITMAP_TRACE_TAG_CORNER
} eBITMAP_TRACE_TAG;

typedef struct _BITMAP_TRACE_CURVE_POINT
{
	int x, y;
	FLOAT_T line_width;
	uint8 color[4];
} BITMAP_TRACE_CURVE_POINT;

typedef struct _BITMAP_TRACE_CURVE_FLOAT_POINT
{
	FLOAT_T x, y;
} BITMAP_TRACE_CURVE_FLOAT_POINT;

typedef struct _BITMAP_TRACE_CURVE
{
	int n;
	int *tag;
	BITMAP_TRACE_CURVE_FLOAT_POINT (*points)[3];
} BITMAP_TRACE_CURVE;

typedef struct _BITMAP_TRACE_SUMS
{
	int x, y, x2, xy, y2;
} BITMAP_TRACE_SUMS;

typedef struct _BITMAP_TRACE_PRIVATE_CURVE
{
	int n;
	int *tag;
	BITMAP_TRACE_CURVE_FLOAT_POINT (*points)[3];
	int alpha_curve;
	BITMAP_TRACE_CURVE_FLOAT_POINT *vertex;
	FLOAT_T *alpha;
	FLOAT_T *alpha_start;
	FLOAT_T *beta;
} BITMAP_TRACE_PRIVATE_CURVE;

typedef struct _BITMAP_TRACE_PRIVATE_PATH
{
	int length;
	BITMAP_TRACE_CURVE_POINT *points;
	int *longest;
	FLOAT_T blur;

	int x_start, y_start;
	BITMAP_TRACE_SUMS *sums;

	int m;
	int *polygon;

	BITMAP_TRACE_PRIVATE_CURVE curve;
	BITMAP_TRACE_PRIVATE_CURVE ocurve;
	BITMAP_TRACE_PRIVATE_CURVE *fcurve;
} BITMAP_TRACE_PRIVATE_PATH;

typedef struct _BITMAP_TRACE_PATH
{
	int area;
	int sign;
	BITMAP_TRACE_CURVE curve;
	struct _BITMAP_TRACE_PATH *next;

	struct _BITMAP_TRACE_PATH *childlist;
	struct _BITMAP_TRACE_PATH *sibling;

	BITMAP_TRACE_PRIVATE_PATH priv;
} BITMAP_TRACE_PATH;

typedef struct _BOUNDING_BOX
{
	int x1, x2, y1, y2;
} BOUNDING_BOX;

/**************************************************************
* FindNext関数                                                *
* 次にチェックするピクセルの位置を調べる                      *
* 引数                                                        *
* tracer		: トレーサーのデータ                          *
* pixels		: チェックする対象のピクセルデータ            *
* x_position	: チェック開始するX座標の入ったデータアドレス *
* y_position	: チェック開始するY座標の入ったデータアドレス *
* 返り値                                                      *
*	発見:TRUE	見つからない:FALSE                            *
**************************************************************/
static int FindNext(BITMAP_TRACER* tracer, uint8* pixels, int* x_position, int* y_position)
{
	int x, y;
	int x_start = *x_position;

	for(y=*y_position; y>=0; y--)
	{
		for(x=x_start; x<tracer->width; x++)
		{
			if(tracer->threshold(tracer, pixels, x, y) != FALSE)
			{
				//while(tracer->threshold(tracer, x, y) == FALSE)
				*x_position = x;
				*y_position = y;
				return TRUE;
			}
		}
	}

	return FALSE;
}

static int Majority(BITMAP_TRACER* tracer, uint8* bitmap, int x, int y)
{
	int i, a, ct;
	int check_x, check_y;

	for(i=2; i<5; i++)
	{
		ct = 0;
		for(a=-i+1; a<i-1; a++)
		{
			check_x = x+a,	check_y = y+i-1;
			if(check_x >= 0 && check_x < tracer->width
				&& check_y >= 0 && check_y < tracer->height)
			{
				ct += (tracer->threshold(tracer, bitmap, check_x, check_y) != FALSE) ? 1 : -1;
			}
			else
			{
				ct -= 1;
			}

			check_x = x+i-1,	check_y = y+1-1;
			if(check_x >= 0 && check_x < tracer->width
				&& check_y >= 0 && check_y < tracer->height)
			{
				ct += (tracer->threshold(tracer, bitmap, check_x, check_y) != FALSE) ? 1 : -1;
			}
			else
			{
				ct -= 1;
			}

			check_x = x+a-1,	check_y = y-i;
			if(check_x >= 0 && check_x < tracer->width
				&& check_y >= 0 && check_y < tracer->height)
			{
				ct += (tracer->threshold(tracer, bitmap, check_x, check_y) != FALSE) ? 1 : -1;
			}
			else
			{
				ct -= 1;
			}

			check_x = x-i,	check_y = y+a;
			if(check_x >= 0 && check_x < tracer->width
				&& check_y >= 0 && check_y < tracer->height)
			{
				ct += (tracer->threshold(tracer, bitmap, check_x, check_y) != FALSE) ? 1 : -1;
			}
			else
			{
				ct -= 1;
			}

			if(ct > 0)
			{
				return TRUE;
			}
			else if(ct < 0)
			{
				return FALSE;
			}
		}
	}

	return FALSE;
}

/***********************************************
* NewPath関数                                  *
* 新しいパスデータのメモリを確保して初期化する *
* 返り値                                       *
*	新しいパスデータ                           *
***********************************************/
static BITMAP_TRACE_PATH* NewPath(void)
{
	BITMAP_TRACE_PATH *result = NULL;

	result = (BITMAP_TRACE_PATH*)MEM_ALLOC_FUNC(sizeof(*result));
	(void)memset(result, 0, sizeof(*result));

	return result;
}

/*******************************
* PathFreeCurveMembers関数     *
* 曲線データのメモリを開放する *
* 引数                         *
* curve	: 開放する曲線データ   *
*******************************/
static void PathFreeCuverMembers(BITMAP_TRACE_PRIVATE_CURVE* curve)
{
	MEM_FREE_FUNC(curve->tag);
	MEM_FREE_FUNC(curve->points);
	MEM_FREE_FUNC(curve->vertex);
	MEM_FREE_FUNC(curve->alpha);
	MEM_FREE_FUNC(curve->alpha_start);
	MEM_FREE_FUNC(curve->beta);
}

/*******************************
* PathFree関数                 *
* パスデータのメモリを開放する *
* 引数                         *
* path	: 開放するパスデータ   *
*******************************/
static void PathFree(BITMAP_TRACE_PATH* path)
{
	if(path != NULL)
	{
		MEM_FREE_FUNC(path->priv.points);
		MEM_FREE_FUNC(path->priv.longest);
		MEM_FREE_FUNC(path->priv.sums);
		MEM_FREE_FUNC(path->priv.polygon);
		PathFreeCuverMembers(&path->priv.curve);
		PathFreeCuverMembers(&path->priv.ocurve);

		MEM_FREE_FUNC(path);
	}
}

static int PrivateCurveInitialize(BITMAP_TRACE_PRIVATE_CURVE* curve, int n)
{
	(void)memset(curve, 0, sizeof(*curve));
	curve->n = n;
	curve->tag = (int*)MEM_ALLOC_FUNC(sizeof(*curve->tag)*n);
	curve->points = (BITMAP_TRACE_CURVE_FLOAT_POINT(*)[3])MEM_ALLOC_FUNC(sizeof(*curve->points)*n);
	curve->vertex = (BITMAP_TRACE_CURVE_FLOAT_POINT*)MEM_ALLOC_FUNC(sizeof(*curve->vertex)*n);
	curve->alpha = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*curve->alpha)*n);
	curve->alpha_start = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*curve->alpha_start)*n);
	curve->beta = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*curve->beta)*n);

	return TRUE;
}

/*************************************************
* SetBoundingboxPath関数                         *
* パスの最大、最小のX座標、Y座標を調べる         *
* 引数                                           *
* box	: 左上の座標と右下の座標を入れるアドレス *
* path	: 調べるパス                             *
*************************************************/
static void SetBoundingboxPath(BOUNDING_BOX* box, BITMAP_TRACE_PATH* path)
{
	int x, y;
	int i;

	box->x1 = box->y1 = INT_MAX;
	box->x2 = box->y2 = 0;

	for(i=0; i<path->priv.length; i++)
	{
		x = path->priv.points[i].x;
		y = path->priv.points[i].y;

		if(x < box->x1)
		{
			box->x1 = x;
		}
		if(x > box->x2)
		{
			box->x2 = x;
		}
		if(y < box->y1)
		{
			box->y1 = y;
		}
		if(y > box->y2)
		{
			box->y2 = y;
		}
	}
}

/*********************************************
* FindPath関数                               *
* 指定された座標から始まるパスを見つける     *
* 引数                                       *
* tracer		: トレーサーのデータ         *
* pixels		: トレースするピクセルデータ *
* x_start		: X座標の開始位置            *
* y_start		: Y座標の開始位置            *
* next_x		: 次に調べ始めるX座標        *
* next_y		: 次に調べ始めるY座標        *
* sign			: 向き                       *
* turn_policy	: カーブの作り方             *
* 返り値                                     *
*	発見したパス(失敗時はNULL)               *
*********************************************/
static BITMAP_TRACE_PATH* FindPath(
	BITMAP_TRACER* tracer,
	uint8* pixels,
	int start_x,
	int start_y,
	int* next_x,
	int* next_y,
	int sign,
	eBITMAP_TRACE_TURN_POLICY turn_policy
)
{
	int x, y, direction_x, direction_y, length, size, area;
	int x0, y0;
	int c, d, temp;
	int stride;
	BITMAP_TRACE_CURVE_POINT *point, *pt;
	BITMAP_TRACE_PATH *path;
	FLOAT_T line_width, min_width;
	uint8 max_opacity;

	stride = tracer->width * 4;
	x = start_x;
	y = start_y;
	direction_x = 0;
	direction_y = -1;

	length = size = 0;
	point = NULL;
	area = 0;

	while(length < 4096 * 4)
	{
		if(length >= size)
		{
			size += 100;
			size = (int)(1.3 * size);
			pt = (BITMAP_TRACE_CURVE_POINT*)MEM_REALLOC_FUNC(point, size * sizeof(*point));
			if(pt == NULL)
			{
				return NULL;
			}
			point = pt;
		}

		point[length].x = x;
		point[length].y = y;

		max_opacity = pixels[tracer->width*4*y + 4*x + 3];
		point[length].color[0] = pixels[tracer->width*4*y + 4*x + 0];
		point[length].color[1] = pixels[tracer->width*4*y + 4*x + 1];
		point[length].color[2] = pixels[tracer->width*4*y + 4*x + 2];
		min_width = INFTY;
		x0 = x,	y0 = y;
		line_width = 1;
		while(1)
		{
			x++;
			if(x >= tracer->width || tracer->threshold(tracer, pixels, x, y) == FALSE)
			{
				break;
			}
			if(max_opacity < pixels[tracer->width*4*y + 4*x + 3])
			{
				max_opacity = pixels[tracer->width*4*y + 4*x + 3];
				point[length].color[0] = pixels[tracer->width*4*y + 4*x + 0];
				point[length].color[1] = pixels[tracer->width*4*y + 4*x + 1];
				point[length].color[2] = pixels[tracer->width*4*y + 4*x + 2];
			}
			line_width += 1;
		}
		if(min_width > line_width)
		{
			*next_x = x;
			*next_y = y;
			min_width = line_width;
		}

		x = x0;
		line_width = 1;
		while(1)
		{
			y--;
			if(y < 0 || tracer->threshold(tracer, pixels, x, y) == FALSE)
			{
				break;
			}
			if(max_opacity < pixels[tracer->width*4*y + 4*x + 3])
			{
				max_opacity = pixels[tracer->width*4*y + 4*x + 3];
				point[length].color[0] = pixels[tracer->width*4*y + 4*x + 0];
				point[length].color[1] = pixels[tracer->width*4*y + 4*x + 1];
				point[length].color[2] = pixels[tracer->width*4*y + 4*x + 2];
			}
			line_width += 1;
		}
		if(min_width > line_width)
		{
			*next_x = x;
			*next_y = y;
			min_width = line_width;
		}

		y = y0;
		line_width = 1;
		while(1)
		{
			x++;
			y--;
			if(x >= tracer->width
				|| y < 0
				|| tracer->threshold(tracer, pixels, x, y) == FALSE
			)
			{
				break;
			}
			if(max_opacity < pixels[tracer->width*4*y + 4*x + 3])
			{
				max_opacity = pixels[tracer->width*4*y + 4*x + 3];
				point[length].color[0] = pixels[tracer->width*4*y + 4*x + 0];
				point[length].color[1] = pixels[tracer->width*4*y + 4*x + 1];
				point[length].color[2] = pixels[tracer->width*4*y + 4*x + 2];
			}
			line_width += 1.41421356;
		}
		if(min_width > line_width)
		{
			*next_x = x;
			*next_y = y;
			min_width = line_width;
		}
		point[length].line_width = min_width;
		point[length].color[3] = max_opacity;

		x = x0,	y = y0;

		length++;

		x += direction_x;
		y += direction_y;
		area += x*direction_y;

		if(x == start_x && y == start_y)
		{
			break;
		}

		c = tracer->threshold(tracer, pixels, x + (direction_x+direction_y-1)/2, y + (direction_y-direction_x-1)/2);
		d = tracer->threshold(tracer, pixels, x + (direction_x-direction_y-1)/2, y + (direction_y+direction_x-1)/2);
		//c = pixels[(y + (direction_y-direction_x-1)/2)*stride+(x + (direction_x+direction_y-1)/2)*4];
		//d = pixels[(y + (direction_y+direction_x-1)/2)*stride+(x + (direction_x-direction_y-1)/2)*4];

		if(c != FALSE && d == FALSE)
		{
			if(turn_policy == BITMAP_TRACE_TURN_POLICY_RIGHT
				|| (turn_policy == BITMAP_TRACE_TURN_POLICY_BLACK && sign != FALSE)
				|| (turn_policy == BITMAP_TRACE_TURN_POLICY_WHITE && sign == FALSE)
				|| (turn_policy == BITMAP_TRACE_TURN_POLICY_RANDOM && (rand() & 1) != 0)
				|| (turn_policy == BITMAP_TRACE_TURN_POLICY_MAJORITY && Majority(tracer, pixels, x, y) != FALSE)
				|| (turn_policy == BITMAP_TRACE_TURN_POLICY_MINORITY && Majority(tracer, pixels, x, y) == FALSE)
			)
			{
				temp = direction_x;
				direction_x = direction_y;
				direction_y = -temp;
			}
			else
			{
				temp = direction_x;
				direction_x = - direction_y;
				direction_y = temp;
			}
		}
		else if(c != FALSE)
		{
			temp = direction_x;
			direction_x = direction_y;
			direction_y = -temp;
		}
		else if(d == FALSE)
		{
			temp = direction_x;
			direction_x = - direction_y;
			direction_y = temp;
		}
	}

	if((path = NewPath()) == NULL)
	{
		return NULL;
	}

	path->priv.points = point;
	path->priv.length = length;
	path->area = area;
	path->sign = sign;

	return path;
}

/*
* XorPath関数
* 与えられたパスとピクセルデータで排他的論理和を取る
* 引数
* tracer
* pixels
* path
*/
static void XorPath(BITMAP_TRACER* tracer, uint8* pixels, BITMAP_TRACE_PATH* path)
{
	int xa, x, y, k, y_start;

	if(path->priv.length <= 0)
	{
		return;
	}

	y_start = path->priv.points[path->priv.length-1].y;
	xa = path->priv.points[0].x;
	for(k=0; k<path->priv.length; k++)
	{
		x = path->priv.points[k].x;
		y = path->priv.points[k].y;

		if(y != y_start)
		{
			// XorToReference(tracer, pixels, x, MINIMUM(y, y_start), xa);
			y_start = y;
		}
	}
}

/*********************************
* PathlistToTree関数             *
* パスのリストを木構造に変える   *
* 引数                           *
* tracer	: トレーサーのデータ *
* path_list	: パスのリスト       *
* pixels	: ビットマップデータ *
*********************************/
static void PathlistToTree(BITMAP_TRACER* tracer, BITMAP_TRACE_PATH* path_list, uint8* pixels)
{
	BITMAP_TRACE_PATH *p, *p_start;
	BITMAP_TRACE_PATH *heap, *heap_start;
	BITMAP_TRACE_PATH *current;
	BITMAP_TRACE_PATH *head;
	BITMAP_TRACE_PATH **path_list_hook;
	BITMAP_TRACE_PATH **hook_in, **hook_out;
	BOUNDING_BOX bounding_box;

	for(p=path_list; p!=NULL; p=p->next)
	{
		p->sibling = p->next;
		p->childlist = NULL;
	}

	heap = path_list;

	while(heap != NULL)
	{
		current = heap;
		heap = heap->childlist;
		current->childlist = NULL;

		head = current;
		current = current->next;
		head->next = NULL;

		// XorPath(tracer, pixels, head);
		SetBoundingboxPath(&bounding_box, head);

		hook_in = &head->childlist;
		hook_out = &head->next;

		for(p=current; p!=NULL; p=p->next)
		{
			if(p->priv.points[0].y <= bounding_box.y1)
			{
				p->next = *hook_out;
				*hook_out = p;
				hook_out = &p->next;

				*hook_out = current;
				break;
			}

			if(p->priv.points[0].x < tracer->width && (p->priv.points[0].y - 1) < tracer->height
				&& tracer->threshold(tracer, pixels, p->priv.points[0].x, p->priv.points[0].y - 1) != FALSE)
			{
				p->next = *hook_in;
				*hook_in = p;
				hook_in = &p->next;
			}
			else
			{
				p->next = *hook_out;
				*hook_out = p;
				hook_out = &p->next;
			}
		}

		// clear_bm_with_bbox

		if(head->next != NULL)
		{
			head->next->childlist = heap;
			heap = head->next;
		}
		if(head->childlist != NULL)
		{
			head->childlist->childlist = heap;
			heap = head->childlist;
		}
	}

	p = path_list;
	while(p != NULL)
	{
		p_start = p->sibling;
		p->sibling = p->next;
		p = p_start;
	}

	heap = path_list;
	if(heap != NULL)
	{
		heap->next = NULL;
	}
	path_list = NULL;
	path_list_hook = &path_list;
	while(heap != NULL)
	{
		heap_start = heap->next;
		for(p=heap; p!=NULL; p=p->sibling)
		{
			p->next = *path_list_hook;
			*path_list_hook = p;
			path_list_hook = &p->next;

			for(p_start=p->childlist; p_start!=NULL; p_start=p_start->sibling)
			{
				p_start->next = *path_list_hook;
				*path_list_hook = p_start;
				path_list_hook = &p_start->next;
				if(p_start->childlist != NULL)
				{
					BITMAP_TRACE_PATH **hook;
					for(hook=&heap_start; *hook!=NULL; hook=&(*hook)->next) ;
					p_start->childlist->next = *hook;
					*hook = p_start->childlist;
				}
			}
		}
		heap = heap_start;
	}
}

/************************************************************************************
* BitmapToPathlist関数                                                              *
* ビットマップデータをパス情報に変換する                                            *
* 引数                                                                              *
* tracer	: トレーサーのデータ                                                    *
* pixels	: ビットマップデータ(中身をClearするのでコピーを使用すること)           *
* path_list	: パスのリストを入れるポインタのアドレス(不使用になったらMEM_FREE_FUNC) *
* 返り値                                                                            *
*	正常終了:TRUE	異常終了:FALSE                                                  *
************************************************************************************/
static int BitmapToPathlist(BITMAP_TRACER* tracer, uint8* pixels, BITMAP_TRACE_PATH** path_list)
{
	BITMAP_TRACE_PATH *p;
	BITMAP_TRACE_PATH *p_list = NULL;
	BITMAP_TRACE_PATH **path_list_hook = &p_list;
	int before_x, before_y;
	int blur;
	int x, y;
	int next_x, next_y;
	int sign;

	before_x = x = 0;
	before_y = y = tracer->height - 1;

	while(FindNext(tracer, pixels, &x, &y) != FALSE)
	{
		sign = tracer->sign(tracer, pixels, x, y);
		p = FindPath(tracer, pixels, x, y+1, &next_x, &next_y,
			sign, tracer->trace_policy);
		if(p == NULL)
		{
			return FALSE;
		}

		// XorPath(tracer, pixels, p);

		if(p->area <= tracer->minimum_path_size)
		{
			PathFree(p);
		}
		else
		{
			p->next = *path_list_hook;
			*path_list_hook = p;
			path_list_hook = &p->next;

			blur = tracer->blur(tracer, pixels, x, y);
			if(blur > 100)
			{
				p->priv.blur = 100.0;
			}
			else if(blur > 50)
			{
				p->priv.blur = 50.0;
			}
			else
			{
				p->priv.blur = 0;
			}
		}

		tracer->progress_update(tracer->progress_data, (1-y)/(FLOAT_T)tracer->height);

		tracer->clear(tracer, pixels, x, y);

		x = next_x,	y = (next_y <= y) ? next_y : y;
		if(before_x == x && before_y == y)
		{
			x++;
			if(x >= tracer->width)
			{
				x = 0;
				y--;
				if(y < 0)
				{
					break;
				}
			}
		}
		before_x = x,	before_y = y;
	}

	PathlistToTree(tracer, p_list, pixels);
	*path_list = p_list;

	return TRUE;
}

static int CalcSums(BITMAP_TRACE_PRIVATE_PATH* path)
{
	int i, x, y;
	int n = path->length;

	path->sums = (BITMAP_TRACE_SUMS*)MEM_ALLOC_FUNC(sizeof(*path->sums)*(path->length+1));
	(void)memset(path->sums, 0, sizeof(*path->sums)*(path->length+1));

	path->x_start = path->points[0].x;
	path->y_start = path->points[0].y;

	for(i=0; i<n; i++)
	{
		x = path->points[i].x - path->x_start;
		y = path->points[i].y - path->y_start;
		path->sums[i+1].x = path->sums[i].x + x;
		path->sums[i+1].y = path->sums[i].y + y;
		path->sums[i+1].x2 = path->sums[i].x2 + x*x;
		path->sums[i+1].xy = path->sums[i].xy + x*y;
		path->sums[i+1].y2 = path->sums[i].y2 + y*y;
	}

	return TRUE;
}

static int CalcLongest(BITMAP_TRACE_PRIVATE_PATH* path)
{
	BITMAP_TRACE_CURVE_POINT *point = path->points;
	int n = path->length;
	int i, j, k, k1;
	int ct[4], direction;
	BITMAP_TRACE_CURVE_POINT constraint[2];
	BITMAP_TRACE_CURVE_POINT curve;
	BITMAP_TRACE_CURVE_POINT off;
	int *pivot_k;
	int *nc;
	BITMAP_TRACE_CURVE_POINT direction_k;
	int a, b, c, d;

	pivot_k = (int*)MEM_ALLOC_FUNC(sizeof(*pivot_k)*n);
	nc = (int*)MEM_ALLOC_FUNC(sizeof(*nc)*n);

	k = 0;
	for(i=n-1; i>=0; i--)
	{
		if(point[i].x != point[k].x && point[i].y != point[k].y)
		{
			k = i + 1;
		}
		nc[i] = k;
	}

	path->longest = (int*)MEM_ALLOC_FUNC(sizeof(*path->longest)*n);

	for(i=n-1; i>=0; i--)
	{
		ct[0] = ct[1] = ct[2] = ct[3] = 0;

		direction = (3 + 3*(point[MOD(i+1, n)].x - point[i].x) + (point[MOD(i+1, n)].y - point[i].y)) / 2;
		ct[direction]++;

		constraint[0].x = 0;
		constraint[0].y = 0;
		constraint[1].x = 0;
		constraint[1].y = 0;

		k = nc[i];
		k1 = i;
		while(1)
		{
			direction = (3 + 3*SIGN(point[k].x - point[k1].x) + SIGN(point[k].y - point[k1].y)) / 2;
			ct[direction]++;

			if(ct[0] != 0 && ct[1] != 0 && ct[2] != 0 && ct[3] != 0)
			{
				pivot_k[i] = k1;
				goto FOUND_K;
			}

			curve.x = point[k].x - point[i].x;
			curve.y = point[k].y - point[i].y;

			if(XPROD(constraint[0], curve) <= 0 || XPROD(constraint[1], curve) > 0)
			{
				goto CONSTRAINT_VIOL;
			}

			if(abs(curve.x) <= 1 && abs(curve.y) <= 1)
			{
			}
			else
			{
				off.x = curve.x + ((curve.y >= 0 && (curve.y > 0 || curve.x < 0)) ? 1 : -1);
				off.y = curve.y + ((curve.x <= 0 && (curve.x < 0 || curve.y < 0)) ? 1 : -1);
				if(XPROD(constraint[0], off) >= 0)
				{
					constraint[0] = off;
				}
				off.x = curve.x + ((curve.y <= 0 && (curve.y < 0 || curve.x < 0)) ? 1 : -1);
				off.y = curve.y + ((curve.x >= 0 && (curve.x > 0 || curve.y < 0)) ? 1 : -1);
				if(XPROD(constraint[1], off) <= 0)
				{
					constraint[1] = off;
				}
			}
			k1 = k;
			k = nc[k1];

			if(CYCLIC(k, i, k1) == 0)
			{
				break;
			}
		}

CONSTRAINT_VIOL:
		direction_k.x = SIGN(point[k].x - point[k1].x);
		direction_k.y = SIGN(point[k].y - point[k1].y);
		curve.x = point[k1].x - point[i].x;
		curve.y = point[k1].y - point[i].y;

		a = XPROD(constraint[0], curve);
		b = XPROD(constraint[0], direction_k);
		c = XPROD(constraint[1], curve);
		d = XPROD(constraint[1], direction_k);

		j = INFTY;
		if(b < 0)
		{
			j = FLOOR_DIVIDE(a, -b);
		}
		if(d > 0)
		{
			int temp = FLOOR_DIVIDE(-c, d);
			j = MINIMUM(j, temp);
		}
		pivot_k[i] = MOD(k1 + j, n);
FOUND_K:
		;
	}

	j = pivot_k[n-1];
	path->longest[n-1] = j;
	for(i = n - 2; i >= 0; i--)
	{
		if(CYCLIC(i + 1, pivot_k[i], j) != FALSE)
		{
			j = pivot_k[i];
		}
		path->longest[i] = j;
	}

	for(i = n - 1; CYCLIC(MOD(i + 1, n), j, path->longest[i]); i--)
	{
		path->longest[i] = j;
	}

	MEM_FREE_FUNC(pivot_k);
	MEM_FREE_FUNC(nc);

	return TRUE;
}

static FLOAT_T Penalty3(BITMAP_TRACE_PRIVATE_PATH* path, int i, int j)
{
	int n = path->length;
	BITMAP_TRACE_CURVE_POINT *point = path->points;
	BITMAP_TRACE_SUMS *sums = path->sums;

	FLOAT_T x, y, x2, xy, y2;
	FLOAT_T k;
	FLOAT_T a, b, c, s;
	FLOAT_T px, py, ex, ey;

	int r = 0;

	if(j >= n)
	{
		j -= n;
		r = 1;
	}

	if(r == 0)
	{
		x = sums[j+1].x - sums[i].x;
		y = sums[j+1].y - sums[i].y;
		x2 = sums[j+1].x2 - sums[i].x2;
		xy = sums[j+1].xy - sums[i].xy;
		y2 = sums[j+1].y2 - sums[i].y2;
		k = j + 1 - i;
	}
	else
	{
		x = sums[j+1].x - sums[i].x + sums[n].x;
		y = sums[j+1].y - sums[i].y + sums[n].y;
		x2 = sums[j+1].x2 - sums[i].x2 + sums[n].x2;
		xy = sums[j+1].xy - sums[i].xy + sums[n].xy;
		y2 = sums[j+1].y2 - sums[i].y2 + sums[n].y2;
		k = j + 1 - i + n;
	}

	px = (point[i].x + point[j].x) * 0.5 - point[0].x;
	py = (point[i].y + point[j].y) * 0.5 - point[0].y;
	ey = (point[j].x - point[i].x);
	ex = - (point[j].y - point[i].y);

	a = ((x2 - 2*x*px) / k + px*px);
	b = ((xy - x*py - y*px) / k + px*py);
	c = ((y2 - 2*y*py) / k + py*py);

	s = ex*ex*a + 2*ex*ey*b + ey*ey*c;

	return sqrt(s);
}

static int BestPolygon(BITMAP_TRACE_PRIVATE_PATH* path)
{
	FLOAT_T *pen = NULL;
	int *prev;
	int *clip0;
	int *clip1;
	int *segment0;
	int *segment1;
	int n = path->length;
	int i, j, m, k;
	FLOAT_T this_pen;
	FLOAT_T best;
	int c;

	pen = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*pen)*(n+1));
	prev = (int*)MEM_ALLOC_FUNC(sizeof(*prev)*(n+1));
	clip0 = (int*)MEM_ALLOC_FUNC(sizeof(*clip0)*n);
	clip1 = (int*)MEM_ALLOC_FUNC(sizeof(*clip1)*(n+1));
	segment0 = (int*)MEM_ALLOC_FUNC(sizeof(*segment0)*(n+1));
	segment1 = (int*)MEM_ALLOC_FUNC(sizeof(*segment1)*(n+1));

	(void)memset(clip0, 0, sizeof(*clip0) * n);
	(void)memset(clip1, 0, sizeof(*clip1) * (n+1));

	for(i=0; i<n; i++)
	{
		c = MOD(path->longest[MOD(i - 1, n)] - 1, n);
		if(c == 1)
		{
			c = MOD(i + 1, n);
		}
		if(c < 1)
		{
			clip0[i] = n;
		}
		else
		{
			clip0[i] = c;
		}
	}

	j = 1;
	for(i=0; i<n; i++)
	{
		while(j <= clip0[i])
		{
			clip1[j] = i;
			j++;
		}
	}

	i = 0;
	for(j=0; i<n && j<n; j++)
	{
		segment0[j] = i;
		i = clip0[i];
	}
	segment0[j] = n;
	m = j;

	i = n;
	for(j=m; j>0; j--)
	{
		segment1[j] = i;
		i = clip1[i];
	}
	segment1[0] = 0;

	pen[0] = 0;
	for(j=1; j<=m; j++)
	{
		for(i=segment1[j]; i<=segment0[j]; i++)
		{
			best = -1;
			for(k=segment0[j-1]; k>=clip1[i]; k--)
			{
				this_pen = Penalty3(path, k, i) + pen[k];
				if(best < 0 || this_pen < best)
				{
					prev[i] = k;
					best = this_pen;
				}
			}
			pen[i] = best;
		}
	}

	path->m = m;
	path->polygon = (int*)MEM_ALLOC_FUNC(sizeof(*path->polygon)*m);

	for(i=n, j=m-1; i>0 && j>=0; j--)
	{
		i = prev[i];
		path->polygon[j] = i;
	}

	MEM_FREE_FUNC(pen);
	MEM_FREE_FUNC(prev);
	MEM_FREE_FUNC(clip0);
	MEM_FREE_FUNC(clip1);
	MEM_FREE_FUNC(segment0);
	MEM_FREE_FUNC(segment1);

	return TRUE;
}

static void PointSlope(BITMAP_TRACE_PRIVATE_PATH* path, int i, int j, BITMAP_TRACE_CURVE_FLOAT_POINT* center, BITMAP_TRACE_CURVE_FLOAT_POINT* direction)
{
	int n = path->length;
	BITMAP_TRACE_SUMS *sums = path->sums;

	FLOAT_T x, y, x2, xy, y2;
	FLOAT_T k;
	FLOAT_T a, b, c, lambda2, l;
	int r = 0;

	while(j >= n)
	{
		j -= n;
		r += 1;
	}
	while(i >= n)
	{
		i -= n;
		r -= 1;
	}
	while(j < 0)
	{
		j += n;
		r -= 1;
	}
	while(i < 0)
	{
		i += n;
		r += 1;
	}

	x = sums[j+1].x - sums[i].x + r*sums[n].x;
	y = sums[j+1].y - sums[i].y + r*sums[n].y;
	x2 = sums[j+1].x2 - sums[i].x2 + r*sums[n].x2;
	xy = sums[j+1].xy - sums[i].xy + r*sums[n].xy;
	y2 = sums[j+1].y2 - sums[i].y2 + r*sums[n].y2;
	k = j + 1 - i + r*n;

	center->x = x / k;
	center->y = y / k;
	
	a = (x2 - (FLOAT_T)x*x/k)/k;
	b = (xy - (FLOAT_T)x*y/k)/k;
	c = (y2 - (FLOAT_T)y*y/k)/k;

	lambda2 = (a + c + sqrt((a - c)*(a - c) + 4*b*b)) / 2;

	a -= lambda2;
	c -= lambda2;

	direction->x = direction->y = 0;
	if(fabs(a) >= fabs(c))
	{
		l = sqrt(a*a + b*b);
		if(l != 0)
		{
			direction->x = -b / l;
			direction->y = a / l;
		}
	}
	else
	{
		l = sqrt(c*c + b*b);
		if(l != 0)
		{
			direction->x = - c / l;
			direction->y = b / l;
		}
	}
}

static FLOAT_T Quadform(FLOAT_T q[][3], BITMAP_TRACE_CURVE_FLOAT_POINT* w)
{
	FLOAT_T v[3];
	int i, j;
	FLOAT_T sum;

	v[0] = w->x;
	v[1] = w->y;
	v[2] = 1;
	sum = 0.0;

	for(i=0; i<3; i++)
	{
		for(j=0; j<3; j++)
		{
			sum += v[i] * q[i][j] * v[j];
		}
	}

	return sum;
}

static int AdjustVertices(BITMAP_TRACE_PRIVATE_PATH* path)
{
	int m = path->m;
	int *polygon = path->polygon;
	int n = path->length;
	BITMAP_TRACE_CURVE_POINT *point = path->points;
	int x_start = path->x_start;
	int y_start = path->y_start;

	BITMAP_TRACE_CURVE_FLOAT_POINT *center;
	BITMAP_TRACE_CURVE_FLOAT_POINT *direction;
	FLOAT_T (*q)[3][3];
	FLOAT_T v[3];
	FLOAT_T d;
	int i, j, k, l;
	BITMAP_TRACE_CURVE_FLOAT_POINT s;

	center = (BITMAP_TRACE_CURVE_FLOAT_POINT*)MEM_ALLOC_FUNC(sizeof(*center)*m);
	direction = (BITMAP_TRACE_CURVE_FLOAT_POINT*)MEM_ALLOC_FUNC(sizeof(*direction)*m);
	q = (FLOAT_T (*)[3][3])MEM_ALLOC_FUNC(sizeof(*q)*m);

	if(PrivateCurveInitialize(&path->curve, m) == FALSE)
	{
		return FALSE;
	}

	for(i=0; i<m; i++)
	{
		j = polygon[MOD(i + 1, m)];
		j = MOD(j - polygon[i], n) + polygon[i];
		PointSlope(path, polygon[i], j, &center[i], &direction[i]);
	}

	for(i=0; i<m; i++)
	{
		d = SQ(direction[i].x) + SQ(direction[i].y);
		if(d == 0.0)
		{
			for(j=0; j<3; j++)
			{
				for(k=0; k<3; k++)
				{
					q[i][j][k] = 0;
				}
			}
		}
		else
		{
			v[0] = direction[i].y;
			v[1] = - direction[i].x;
			v[2] = - v[1] * center[i].y - v[0] * center[i].x;
			for(l=0; l<3; l++)
			{
				for(k=0; k<3; k++)
				{
					q[i][l][k] = v[l] * v[k] / d;
				}
			}
		}
	}

	for(i=0; i<m; i++)
	{
		FLOAT_T Q[3][3];
		BITMAP_TRACE_CURVE_FLOAT_POINT w;
		FLOAT_T dx, dy;
		FLOAT_T determin;
		FLOAT_T minimum, candidate;
		FLOAT_T x_min, y_min;
		int z;

		s.x = point[polygon[i]].x - x_start;
		s.y = point[polygon[i]].y - y_start;

		j = MOD(i - 1, m);

		for(l=0; l<3; l++)
		{
			for(k=0; k<3; k++)
			{
				Q[l][k] = q[j][l][k] + q[i][l][k];
			}
		}

		while(1)
		{
			determin = Q[0][0] * Q[1][1] - Q[0][1]*Q[1][0];
			if(determin != 0.0)
			{
				w.x = (- Q[0][2]*Q[1][1] + Q[1][2]*Q[0][1]) / determin;
				w.y = (  Q[0][2]*Q[1][0] - Q[1][2]*Q[0][0]) / determin;
				break;
			}

			if(Q[0][0] > Q[1][1])
			{
				v[0] = -Q[0][1];
				v[1] = Q[0][0];
			}
			else if(Q[1][1] != 0.0)
			{
				v[0] = - Q[1][1];
				v[1] = Q[1][0];
			}
			else
			{
				v[0] = 1;
				v[1] = 0;
			}
			d = SQ(v[0]) + SQ(v[1]);
			v[2] = - v[1] * s.y - v[0] * s.x;
			for(l=0; l<3; l++)
			{
				for(k=0; k<3; k++)
				{
					Q[l][k] += v[l] * v[k] / d;
				}
			}
		}
		dx = fabs(w.x - s.x);
		dy = fabs(w.y - s.y);
		if(dx <= 0.5 && dy <= 0.5)
		{
			path->curve.vertex[i].x = (int)(w.x + x_start);
			path->curve.vertex[i].y = (int)(w.y + y_start);
			continue;
		}

		minimum = Quadform(Q, &s);
		x_min = s.x;
		y_min = s.y;

		if(Q[0][0] == 0.0)
		{
			goto FIXX;
		}
		for(z=0; z<2; z++)
		{
			w.y = s.y - 0.5 + z;
			w.x = - (Q[0][1] * w.y + Q[0][2]) / Q[0][0];
			dx = fabs(w.x - s.x);
			candidate = Quadform(Q, &w);
			if(dx <= 0.5 && candidate < minimum)
			{
				minimum = candidate;
				x_min = w.x;
				y_min = w.y;
			}
		}
FIXX:
		if(Q[1][1] == 0.0)
		{
			goto CORNERS;
		}
		for(z=0; z<2; z++)
		{
			w.x = s.x - 0.5 + z;
			w.y = - (Q[1][0] * w.x + Q[1][2]) / Q[1][1];
			dy = fabs(w.y - s.y);
			candidate = Quadform(Q, &w);
			if(dy <= 0.5 && candidate < minimum)
			{
				minimum = candidate;
				x_min = w.x;
				y_min = w.y;
			}
		}
CORNERS:
		for(l=0; l<2; l++)
		{
			for(k=0; k<2; k++)
			{
				w.x = s.x - 0.5 + l;
				w.y = s.y - 0.5 + k;
				candidate = Quadform(Q, &w);
				if(candidate < minimum)
				{
					minimum = candidate;
					x_min = w.x;
					y_min = w.y;
				}
			}
		}

		path->curve.vertex[i].x = (int)x_min + x_start;
		path->curve.vertex[i].y = (int)y_min + y_start;
		continue;
	}

	MEM_FREE_FUNC(center);
	MEM_FREE_FUNC(direction);
	MEM_FREE_FUNC(q);

	return TRUE;
}

static void Reverse(BITMAP_TRACE_PRIVATE_CURVE* curve)
{
	int m = curve->n;
	int i, j;
	BITMAP_TRACE_CURVE_FLOAT_POINT temp;

	for(i=0, j=m-1; i<j; i++, j--)
	{
		temp = curve->vertex[i];
		curve->vertex[i] = curve->vertex[j];
		curve->vertex[j] = temp;
	}
}

static void SmoothCurve(BITMAP_TRACE_PRIVATE_CURVE* curve, FLOAT_T alpha_max)
{
	int m = curve->n;

	int i, j, k;
	FLOAT_T dd, denominator, alpha;
	BITMAP_TRACE_CURVE_FLOAT_POINT p2, p3, p4;

	for(i=0; i<m; i++)
	{
		j = MOD(i + 1, m);
		k = MOD(i + 2, m);
		INTERVAL(&p4, 0.5, curve->vertex[k], curve->vertex[j]);

		denominator = DDENOMINATOR(curve->vertex[i], curve->vertex[j]);
		if(denominator != 0.0)
		{
			dd = DPARA(curve->vertex[i], curve->vertex[j], curve->vertex[k]) / denominator;
			dd = fabs(dd);
			alpha = (dd > 1) ? (1 - 1.0/dd) : 0;
			alpha = alpha / 0.75;
		}
		else
		{
			alpha = 4/3.0;
		}
		curve->alpha_start[j] = alpha;

		if(alpha > alpha_max)
		{
			curve->tag[j] = BITMAP_TRACE_TAG_CORNER;
			curve->points[j][1] = curve->vertex[j];
			curve->points[j][2] = p4;
		}
		else
		{
			if(alpha < 0.55)
			{
				alpha = 0.55;
			}
			else if(alpha > 1)
			{
				alpha = 1;
			}
			INTERVAL(&p2, 0.5 + 0.5*alpha, curve->vertex[i], curve->vertex[j]);
			INTERVAL(&p3, 0.5 + 0.5*alpha, curve->vertex[k], curve->vertex[j]);
			curve->tag[j] = BITMAP_TRACE_TAG_CURVE_TO;
			curve->points[j][0] = p2;
			curve->points[j][1] = p3;
			curve->points[j][2] = p4;
		}
		curve->alpha[j] = alpha;
		curve->beta[j] = 0.5;
	}
	curve->alpha_curve = 1;
}

static FLOAT_T Tangent(
	BITMAP_TRACE_CURVE_FLOAT_POINT* p0,
	BITMAP_TRACE_CURVE_FLOAT_POINT* p1,
	BITMAP_TRACE_CURVE_FLOAT_POINT* p2,
	BITMAP_TRACE_CURVE_FLOAT_POINT* p3,
	BITMAP_TRACE_CURVE_FLOAT_POINT* q0,
	BITMAP_TRACE_CURVE_FLOAT_POINT* q1
)
{
	FLOAT_T A, B, C;
	FLOAT_T a, b, c;
	FLOAT_T d, s, r1, r2;

	A = CPROD(*p0, *p1, *q0, *q1);
	B = CPROD(*p1, *p2, *q0, *q1);
	C = CPROD(*p2, *p3, *q0, *q1);

	a = A - 2*B + C;
	b = -2*A + 2*B;
	c = A;

	d = b*b - 4*a*c;

	if(a == 0.0 || d < 0)
	{
		return -1.0;
	}

	s = sqrt(d);

	r1 = (-b + s) / (2 * a);
	r2 = (-b - s) / (2 * 2);

	if(r1 >= 0 && r1 <= 1)
	{
		return r1;
	}
	else if(r2 >= 0 && r2 <= 1)
	{
		return r2;
	}
	return -1.0;
}

typedef struct _OPTIMIZE_CURVE
{
	FLOAT_T penalty;
	BITMAP_TRACE_CURVE_FLOAT_POINT c[2];
	FLOAT_T t, s;
	FLOAT_T alpha;
} OPTIMIZE_CURVE;

static int OptimizePenalty(
	BITMAP_TRACE_PRIVATE_PATH* path,
	int i,
	int j,
	OPTIMIZE_CURVE* result,
	FLOAT_T optimize_tolerance,
	int* convert_c,
	FLOAT_T* area_c
)
{
	int m = path->curve.n;
	int k, k1, k2, convert, i1;
	FLOAT_T area, alpha, d, d1, d2;
	BITMAP_TRACE_CURVE_FLOAT_POINT p0, p1, p2, p3, point;
	FLOAT_T a, r, a1, a2, a3, a4;
	FLOAT_T s, t;

	if(i == j)
	{
		return TRUE;
	}

	k = i;
	i1 = MOD(i + 1, m);
	k1 = MOD(k + 1, m);
	convert = convert_c[k1];

	d = DDIST(path->curve.vertex[i], path->curve.vertex[i1]);
	for(k=k1; k!=j; k=k1)
	{
		k1 = MOD(k + 1, m);
		k2 = MOD(k + 2, m);
		if(convert_c[k1] != convert)
		{
			return TRUE;
		}
		if(SIGN(CPROD(path->curve.vertex[i], path->curve.vertex[i1], path->curve.vertex[k1], path->curve.vertex[k2])) != convert)
		{
			return TRUE;
		}
		if(IPROD1(path->curve.vertex[i], path->curve.vertex[i1], path->curve.vertex[k1], path->curve.vertex[k2])
			< d * DDIST(path->curve.vertex[k1], path->curve.vertex[k2]) * COS179)
		{
			return TRUE;
		}
	}

	p0 = path->curve.points[MOD(i, m)][2];
	p1 = path->curve.vertex[MOD(i + 1, m)];
	p2 = path->curve.vertex[MOD(j, m)];
	p3 = path->curve.points[MOD(j, m)][2];

	area = area_c[j] - area_c[i];
	area -= DPARA(path->curve.vertex[0], path->curve.points[i][2], path->curve.points[j][2]) / 2;
	if(i >= j)
	{
		area += area_c[m];
	}

	a1 = DPARA(p0, p1, p2);
	a2 = DPARA(p0, p1, p3);
	a3 = DPARA(p0, p2, p3);
	a4 = a1 + a3 - a2;

	if(a2 == a1)
	{
		return TRUE;
	}

	t = a3 / (a3 - a4);
	s = a2 / (a2 - a1);
	a = a2 * t * 0.5;

	if(a == 0.0)
	{
		return TRUE;
	}

	r = area / a;
	alpha = 2 - sqrt(4 - r * (1/0.3));

	INTERVAL(&result->c[0], t * alpha, p0, p1);
	INTERVAL(&result->c[1], s * alpha, p3, p2);
	result->alpha = alpha;
	result->t = t;
	result->s = s;

	p1 = result->c[0];
	p2 = result->c[1];

	result->penalty = 0;

	for(k=MOD(i + 1, m); k!=j; k=k1)
	{
		k1 = MOD(k + 1, m);
		t = Tangent(&p0, &p1, &p2, &p3, &path->curve.vertex[k], &path->curve.vertex[k1]);
		if(t < -0.5)
		{
			return TRUE;
		}

		BEZIER(&point, t, p0, p1, p2, p3);
		d = DDIST(path->curve.vertex[k], path->curve.vertex[k1]);
		if(d == 0.0)
		{
			return TRUE;
		}
		d1 = DPARA(path->curve.vertex[k], path->curve.vertex[k1], point) / d;
		if(fabs(d1) > optimize_tolerance)
		{
			return TRUE;
		}
		if(IPROD(path->curve.vertex[k], path->curve.vertex[k1], point) < 0
			|| IPROD(path->curve.vertex[k1], path->curve.vertex[k], point) < 0)
		{
			return TRUE;
		}
		result->penalty += SQ(d1);
	}

	for(k=i; k!=j; k=k1)
	{
		k1 = MOD(k + 1, m);
		t = Tangent(&p0, &p1, &p2, &p3, &path->curve.points[k][2], &path->curve.points[k1][2]);
		if(t < -0.5)
		{
			return TRUE;
		}
		BEZIER(&point, t, p0, p1, p2, p3);
		d = DDIST(path->curve.points[k][2], path->curve.points[k1][2]);
		if(d == 0.0)
		{
			return TRUE;
		}
		d1 = DPARA(path->curve.points[k][2], path->curve.points[k1][2], point) / d;
		d2 = DPARA(path->curve.points[k][2], path->curve.points[k1][2], path->curve.vertex[k1]) / d;
		d2 *= 0.75 * path->curve.alpha[k1];
		if(d2 < 0)
		{
			d1 = - d1;
			d2 = - d2;
		}
		if(d1 < d2 - optimize_tolerance)
		{
			return TRUE;
		}
		if(d1 < d2)
		{
			result->penalty += SQ(d1 - d2);
		}
	}

	return FALSE;
}

static int OptimizeCurve(BITMAP_TRACE_PRIVATE_PATH* path, FLOAT_T optimize_tolerance)
{
	int m = path->curve.n;
	int *point;
	FLOAT_T *penalty;
	int *length;
	OPTIMIZE_CURVE *optimize;
	int om;
	int i, j, r;
	OPTIMIZE_CURVE o;
	BITMAP_TRACE_CURVE_FLOAT_POINT p0;
	int i1;
	FLOAT_T area;
	FLOAT_T alpha;
	FLOAT_T *s;
	FLOAT_T *t;

	int *convert_c;
	FLOAT_T *area_c;

	point = (int*)MEM_ALLOC_FUNC(sizeof(*point)*(m+1));
	penalty = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*penalty)*(m+1));
	length = (int*)MEM_ALLOC_FUNC(sizeof(*length)*(m+1));
	optimize = (OPTIMIZE_CURVE*)MEM_ALLOC_FUNC(sizeof(*optimize)*(m+1));
	convert_c = (int*)MEM_ALLOC_FUNC(sizeof(*convert_c)*m);
	area_c = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*area_c)*(m+1));

	for(i=0; i<m; i++)
	{
		if(path->curve.tag[i] == BITMAP_TRACE_TAG_CURVE_TO)
		{
			convert_c[i] = SIGN(DPARA(path->curve.vertex[MOD(i - 1, m)], path->curve.vertex[i], path->curve.vertex[MOD(i + 1, m)]));
		}
		else
		{
			convert_c[i] = 0;
		}
	}

	area = 0;
	area_c[0] = 0;
	p0 = path->curve.vertex[0];
	for(i=0; i<m; i++)
	{
		i1 = MOD(i + 1, m);
		if(path->curve.tag[i1] == BITMAP_TRACE_TAG_CURVE_TO)
		{
			alpha = path->curve.alpha[i1];
			area += 0.3*alpha*(4-alpha)*DPARA(path->curve.points[i][2], path->curve.vertex[i1], path->curve.points[i1][2]) / 2;
			area += DPARA(p0, path->curve.points[i][2], path->curve.points[i1][2]) / 2;
		}
		area_c[i+1] = area;
	}

	point[0] = -1;
	penalty[0] = 0;
	length[0] = 0;

	for(j=1; j<=m; j++)
	{
		point[j] = j-1;
		penalty[j] = penalty[j-1];
		length[j] = length[j-1]+1;

		for(i=j-2; i>=0; i--)
		{
			r = OptimizePenalty(path, i, MOD(j, m), &o, optimize_tolerance, convert_c, area_c);
			if(r != FALSE)
			{
				break;
			}
			if(length[j] > length[i] + 1 || (length[j] == length[i] + 1 && penalty[j] > penalty[i] + o.penalty))
			{
				point[j] = i;
				penalty[j] = penalty[i] + o.penalty;
				length[j] = length[i] + 1;
				optimize[j] = o;
			}
		}
	}
	om = length[m];
	if(PrivateCurveInitialize(&path->ocurve, om) == FALSE)
	{
		return FALSE;
	}
	s = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*s)*om);
	t = (FLOAT_T*)MEM_ALLOC_FUNC(sizeof(*t)*om);

	j = m;
	for(i=om-1; i>=0; i--)
	{
		int index = MOD(j, m);
		if(point[j] == j-1)
		{
			path->ocurve.tag[i] = path->curve.tag[index];
			path->ocurve.points[i][0] = path->curve.points[index][0];
			path->ocurve.points[i][1] = path->curve.points[index][1];
			path->ocurve.points[i][2] = path->curve.points[index][2];
			path->ocurve.vertex[i] = path->curve.vertex[index];
			path->ocurve.alpha[i] = path->curve.alpha[index];
			path->ocurve.alpha_start[i] = path->curve.alpha_start[index];
			path->ocurve.beta[i] = path->curve.beta[index];
			s[i] = t[i] = 1.0;
		}
		else
		{
			path->ocurve.tag[i] = BITMAP_TRACE_TAG_CURVE_TO;
			path->ocurve.points[i][0] = optimize[j].c[0];
			path->ocurve.points[i][1] = optimize[j].c[1];
			path->ocurve.points[i][2] = path->curve.points[index][2];
			INTERVAL(&path->ocurve.vertex[i], optimize[j].s, path->curve.points[index][2], path->curve.vertex[index]);
			path->ocurve.alpha[i] = optimize[j].alpha;
			path->ocurve.alpha_start[i] = optimize[j].alpha;
			s[i] = optimize[j].s;
			t[i] = optimize[j].t;
		}
		j = point[j];
	}

	for(i=0; i<om; i++)
	{
		i1 = MOD(i + 1, om);
		path->ocurve.beta[i] = s[i] / (s[i] + t[i1]);
	}
	path->ocurve.alpha_curve = 1;

	MEM_FREE_FUNC(point);
	MEM_FREE_FUNC(penalty);
	MEM_FREE_FUNC(length);
	MEM_FREE_FUNC(optimize);
	MEM_FREE_FUNC(s);
	MEM_FREE_FUNC(t);
	MEM_FREE_FUNC(convert_c);
	MEM_FREE_FUNC(area_c);

	return TRUE;
}

static void PrivateCurveToCurve(BITMAP_TRACE_PRIVATE_CURVE* priv, BITMAP_TRACE_CURVE* curve)
{
	curve->n = priv->n;
	curve->tag = priv->tag;
	curve->points = priv->points;
}

static int ProcessPath(BITMAP_TRACER* tracer, BITMAP_TRACE_PATH* path_list)
{
	BITMAP_TRACE_PATH *p;
	FLOAT_T nn = 0, cn = 0;

	for(p=path_list; p!=NULL; p=p->next)
	{
		if(CalcSums(&p->priv) == FALSE)
		{
			return FALSE;
		}
		if(CalcLongest(&p->priv) == FALSE)
		{
			return FALSE;
		}
		if(BestPolygon(&p->priv) == FALSE)
		{
			return FALSE;
		}
		if(AdjustVertices(&p->priv) == FALSE)
		{
			return FALSE;
		}
		if(p->sign == FALSE)
		{
			Reverse(&p->priv.curve);
		}
		SmoothCurve(&p->priv.curve, tracer->alpha_max);
		if(tracer->optimize_curve != FALSE)
		{
			if(OptimizeCurve(&p->priv, tracer->optimize_tolerance) == FALSE)
			{
				return FALSE;
			}
			p->priv.fcurve = &p->priv.ocurve;
		}
		else
		{
			p->priv.fcurve = &p->priv.curve;
		}
		PrivateCurveToCurve(p->priv.fcurve, &p->curve);
	}

	return TRUE;
}

static int BitmapTracerThresholdBlack(BITMAP_TRACER* tracer, uint8* pixels, int x, int y)
{
	return pixels[tracer->width*4*y + 4*x] < tracer->threshold_data.black_white_threshold
		&& pixels[tracer->width*4*y + 4*x + 3] >= tracer->threshold_data.opacity_threshold;
}

static int BitmapTracerDefaultSign(BITMAP_TRACER* tracer, uint8* pixels, int x, int y)
{
	return tracer->threshold(tracer, pixels, x, y);
}

static int BitmapTracerDefaultBlur(BITMAP_TRACER* tracer, uint8* pixels, int x, int y)
{
	int stride = tracer->width * 4;
	int blur = 0;
	int i, j;

	for(i=x; i < tracer->width && tracer->threshold(tracer, pixels, i, y) != FALSE; i++)
	{
		blur += pixels[y*stride + i*4+3];
	}

	for(i=y; i >= 0 && tracer->threshold(tracer, pixels, x, i) != FALSE; i--)
	{
		blur += pixels[i*stride + x*4+3];
	}

	for(i=x, j=y;
		i < tracer->width && j >= 0 && tracer->threshold(tracer, pixels, i, j) != FALSE; i++, j--)
	{
		blur += pixels[j*stride + j*4+3];
	}

	return blur;
}

static void BitmapTracerDefaultClear(BITMAP_TRACER* tracer, uint8* pixels, int x, int y)
{
	int stride = tracer->width * 4;
	int i, j;

	for(i=x; i < tracer->width && tracer->threshold(tracer, pixels, i, y) != FALSE; i++)
	{
		pixels[y*stride + i*4+0] = 0;
		pixels[y*stride + i*4+1] = 0;
		pixels[y*stride + i*4+2] = 0;
		pixels[y*stride + i*4+3] = 0;
	}

	for(i=y; i >= 0 && tracer->threshold(tracer, pixels, x, i) != FALSE; i--)
	{
		pixels[i*stride + x*4+0] = 0;
		pixels[i*stride + x*4+1] = 0;
		pixels[i*stride + x*4+2] = 0;
		pixels[i*stride + x*4+3] = 0;
	}

	for(i=x, j=y;
		i < tracer->width && j >= 0 && tracer->threshold(tracer, pixels, i, j) != FALSE; i++, j--)
	{
		pixels[j*stride + i*4+0] = 0;
		pixels[j*stride + i*4+1] = 0;
		pixels[j*stride + i*4+2] = 0;
		pixels[j*stride + i*4+3] = 0;
	}
}

static void DefaultProgress(void* data, FLOAT_T rate)
{
}

/*****************************************************
* InitializeBitmapTracer関数                         *
* ビットマップデータをトレースするデータを初期化する *
* 引数                                               *
* tracer	: トレーサーのデータ                     *
* pixels	: ビットマップデータ                     *
* width		: ビットマップデータの幅                 *
* height	: ビットマップデータの高さ               *
*****************************************************/
void InitializeBitmapTracer(BITMAP_TRACER* tracer, uint8* pixels, int width, int height)
{
	(void)memset(tracer, 0, sizeof(*tracer));
	tracer->trace_pixels = pixels;
	tracer->width = width;
	tracer->height = height;
	tracer->minimum_point_distance = DEFAULT_MINIMUM_DISTANCE * DEFAULT_MINIMUM_DISTANCE;
	tracer->line_color[3] = 0xFF;

	tracer->threshold_data.black_white_threshold = 128;
	tracer->threshold_data.opacity_threshold = 128;

	tracer->threshold = BitmapTracerThresholdBlack;
	tracer->sign = BitmapTracerDefaultSign;
	tracer->clear = BitmapTracerDefaultClear;
	tracer->blur = BitmapTracerDefaultBlur;
	tracer->progress_update = DefaultProgress;
}

/***************************************************
* ReleaseBitmapTracer関数                          *
* ビットマップデータをトレースするデータを開放する *
* 引数                                             *
* tracer	: トレーサーのデータ                   *
***************************************************/
void ReleaseBitmapTracer(BITMAP_TRACER* tracer)
{
	BITMAP_TRACE_PATH *path = (BITMAP_TRACE_PATH*)tracer->path;
	BITMAP_TRACE_PATH *p, *next;

	for(p=path; p!=NULL; p=next)
	{
		next = p->next;
		PathFree(p);
	}
}

/***********************************************
* TraceBitmap関数                              *
* ビットマップデータをベクトルデータに変換する *
* 引数                                         *
* tracer	: トレーサーのデータ               *
* 返り値                                       *
*	正常終了:TRUE	異常終了:FALSE             *
***********************************************/
int TraceBitmap(BITMAP_TRACER* tracer)
{
#define BITMAP_SCANLINE(BITMAP, Y, STRIDE) ((BITMAP) + (Y) * (STRIDE))
#define BITMAP_INDEX(BITMAP, X, Y, STRIDE) ((BITMAP_SCANLINCE(BITMAP, Y, STRIDE))[(X) * 4])
#define BITMAP_USET(BITMAP, X, Y, STRIDE, VALUE) (BITMAP_INDEX(BITMAP, X, Y, STRIDE) = (VALUE))
#define BITMAP_UPUT(BITMAP, X, Y, STRIDE, SET_FLAG) (((SET_FLAG) != 0) ? BITMAP_USET(BITMAP, X, Y, STRIDE, 0) : BITMAP_USET(BITMAP, X, Y, STRIDE, 1))

	BITMAP_TRACE_PATH *path_list;
	uint8 *trace_bitmap;
	int stride = tracer->width * 4;
	int x, y;

	trace_bitmap = MEM_ALLOC_FUNC(stride * tracer->height);
	(void)memset(trace_bitmap, 0, stride * tracer->height);

	switch(tracer->trace_mode)
	{
	case BITMAP_TRACE_MODE_GRAY_SCALE:
		for(y=0; y<tracer->height; y++)
		{
			for(x=0; x<tracer->width; x++)
			{
				trace_bitmap[y*stride+x*4+0] =
					(tracer->trace_pixels[y*stride+x*4+0] + tracer->trace_pixels[y*stride+x*4+1] + tracer->trace_pixels[y*stride+x*4+2]) / 3;
				trace_bitmap[y*stride+x*4+2] = trace_bitmap[y*stride+x*4+1] = trace_bitmap[y*stride+x*4+0];
				trace_bitmap[y*stride+x*4+3] = tracer->trace_pixels[y*stride+x*4+3];
			}
		}
		break;
	case BITMAP_TRACE_MODE_BRIGHTNESS:
		{
			HSV hsv;
			for(y=0; y<tracer->height; y++)
			{
				for(x=0; x<tracer->width; x++)
				{
					HSV2RGB_Pixel(&hsv, &tracer->trace_pixels[y*stride+x*4]);
					trace_bitmap[y*stride+x*4+0] = trace_bitmap[y*stride+x*4+1] =
						trace_bitmap[y*stride+x*4+2] = trace_bitmap[y*stride+x*4+3] = hsv.v;
				}
			}
		}
		break;
	case BITMAP_TRACE_MODE_COLOR:
		for(y=0; y<tracer->height; y++)
		{
			for(x=0; x<tracer->width; x++)
			{
				trace_bitmap[y*stride+x*4+0] = tracer->trace_pixels[y*stride+x*4+0];
				trace_bitmap[y*stride+x*4+1] = tracer->trace_pixels[y*stride+x*4+1];
				trace_bitmap[y*stride+x*4+2] = tracer->trace_pixels[y*stride+x*4+2];
				trace_bitmap[y*stride+x*4+3] = tracer->trace_pixels[y*stride+x*4+3];
			}
		}
		break;
	}

	if(BitmapToPathlist(tracer, trace_bitmap, &path_list) == FALSE)
	{
		return FALSE;
	}

	if(ProcessPath(tracer, path_list) == FALSE)
	{
		return FALSE;
	}

	tracer->path = (void*)path_list;

	MEM_FREE_FUNC(trace_bitmap);

	return TRUE;
}

static int CullPoints(BITMAP_TRACER* tracer, BITMAP_TRACE_CURVE_POINT* buff, BITMAP_TRACE_CURVE_POINT* points, int n)
{
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif
#define MINIMUM_ANGLE (M_PI/180)
	BITMAP_TRACE_CURVE_POINT *current = points;
	FLOAT_T angle;
	FLOAT_T this_angle;
	int index = 1;
	int m;
	int i;

	buff[0] = points[0];
	for(m=1; m<n-1; m++)
	{
		if(SQ(current->x - points[m].x) + SQ(current->y - points[m].y)
			> tracer->minimum_point_distance)
		{
			current = &points[m];
			angle = atan2(points->y-current->y, points->x-current->x);
			buff[index] = points[m];
			index++;
			break;
		}
	}

	for(i=m; i<n-1; i++)
	{
		if(SQ(current->x - points[i].x) + SQ(current->y - points[i].y)
			> tracer->minimum_point_distance
			&& (fabs((this_angle = atan2(points[i].y-current->y, points[i].x-current->x)) - angle) >= MINIMUM_ANGLE)
		)
		{
			current = &points[i];
			buff[index] = points[i];
			angle = this_angle;
			index++;
		}
	}

	if(SQ(current->x - points[n-1].x) + SQ(current->y - points[n-1].y)
		> tracer->minimum_point_distance
		&& (fabs((this_angle = atan2(points[n-1].y-current->y, points[n-1].x-current->x)) - angle) >= MINIMUM_ANGLE)
	)
	{
		buff[index] = points[n-1];
		index++;
	}
	else
	{
		buff[index-1] = points[n-1];
	}

	return index;
}

static void CullPath(BITMAP_TRACER* tracer)
{
	BITMAP_TRACE_PATH *path, *compare;
	BITMAP_TRACE_CURVE_POINT *buffer = NULL;
	size_t buffer_size = 0;
	int i, j;

	for(path = (BITMAP_TRACE_PATH*)tracer->path;
		path != NULL; path = path->next)
	{
		if (buffer_size < path->priv.length)
		{
			buffer_size = path->priv.length;
			buffer = (BITMAP_TRACE_CURVE_POINT*)MEM_REALLOC_FUNC(buffer, sizeof(*buffer)*buffer_size);
		}
		else if (path->priv.length == 0)
		{
			continue;
		}

		path->priv.length = CullPoints(tracer, buffer, path->priv.points, path->priv.length);

		(void)memcpy(path->priv.points, buffer, sizeof(*buffer)*path->priv.length);
	}

	for(path = (BITMAP_TRACE_PATH*)tracer->path;
		path != NULL; path = path->next)
	{
		for(compare = path->next;
			compare != NULL; compare = compare->next)
		{
			for(i=0; i<path->priv.length; i++)
			{
				for(j=0; j<compare->priv.length; j++)
				{
					if(SQ(path->priv.points[i].x - compare->priv.points[j].x) + SQ(path->priv.points[i].y - compare->priv.points[j].y)
					   < tracer->minimum_point_distance)
					{
						if(j < compare->priv.length - 1)
						{
							compare->priv.points[j] = compare->priv.points[j+1];
						}
						compare->priv.length--;
						j--;
					}
				}
			}
		}
	}

	MEM_FREE_FUNC(buffer);
}

/***********************************************
* AdoptTraceBitmapResult関数                   *
* トレースした結果をベクトルレイヤーに適用する *
* 引数                                         *
* tracer	: トレーサーのデータ               *
* layer		: 適用先のベクトルレイヤー         *
***********************************************/
void AdoptTraceBitmapResult(BITMAP_TRACER* tracer, void* layer)
{
	DRAW_WINDOW *window = ((LAYER*)layer)->window;
	VECTOR_LAYER *vector_layer = ((LAYER*)layer)->layer_data.vector_layer_p;
	VECTOR_LINE *line;
	BITMAP_TRACE_PATH *path;
	BITMAP_TRACE_CURVE_POINT *buffer;
	FLOAT_T line_width;
	int length;
	int i;

	CullPath(tracer);

	if((tracer->flags & BITMAP_TRACE_FALGS_USE_SPECIFY_LINE_COLOR) != 0)
	{
		for(path = (BITMAP_TRACE_PATH*)tracer->path;
			path != NULL; path = path->next)
		{
			buffer = path->priv.points;
			length = path->priv.length;

			for(i=0; i<length; i++)
			{
				buffer[i].color[0] = tracer->line_color[0];
				buffer[i].color[1] = tracer->line_color[1];
				buffer[i].color[2] = tracer->line_color[2];
				buffer[i].color[3] = tracer->line_color[3];
			}
		}
	}

	for(path = (BITMAP_TRACE_PATH*)tracer->path;
		path != NULL; path = path->next)
	{
		buffer = path->priv.points;
		length = path->priv.length;
		if(length <= 0)
		{
			continue;
		}
		line = CreateVectorLine((VECTOR_LINE*)vector_layer->top_data, NULL);
		line->base_data.vector_type = VECTOR_TYPE_BEZIER_OPEN;
		line->num_points = length;
		line->outline_hardness = 100.0;
		line->buff_size = ((line->num_points / VECTOR_LINE_BUFFER_SIZE) + 1) * VECTOR_LINE_BUFFER_SIZE;
		line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*line->buff_size);
		(void)memset(line->points, 0, sizeof(*line->points)*line->buff_size);

		for(i=0, line_width=0; i<line->num_points; i++)
		{
			line_width += buffer[i].line_width;
		}
		line_width /= line->num_points;
		if(line_width < 1)
		{
			line_width = 1;
		}
		line->blur = path->priv.blur;

		for(i=0; i<line->num_points; i++)
		{
			line->points[i].vector_type = VECTOR_TYPE_BEZIER_OPEN;
			line->points[i].flags = 0;
			line->points[i].color[0] = buffer[i].color[0];
			line->points[i].color[1] = buffer[i].color[1];
			line->points[i].color[2] = buffer[i].color[2];
			line->points[i].color[3] = buffer[i].color[3];
			line->points[i].pressure = (buffer[i].line_width / line_width) * 100;
			line->points[i].size = line_width;
			line->points[i].x = buffer[i].x;
			line->points[i].y = buffer[i].y;
		}

		vector_layer->top_data = (void*)line;
		vector_layer->num_lines++;
	}

	vector_layer->flags |= VECTOR_LAYER_RASTERIZE_ALL | VECTOR_LAYER_FIX_LINE;
	RasterizeVectorLayer(window, (LAYER*)layer, vector_layer);
}

