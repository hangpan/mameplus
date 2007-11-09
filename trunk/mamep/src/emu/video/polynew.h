/***************************************************************************

    polynew.h

    New polygon helper routines.

****************************************************************************

    Pixel model:

    (0.0,0.0)       (1.0,0.0)       (2.0,0.0)       (3.0,0.0)
        +---------------+---------------+---------------+
        |               |               |               |
        |               |               |               |
        |   (0.5,0.5)   |   (1.5,0.5)   |   (2.5,0.5)   |
        |       *       |       *       |       *       |
        |               |               |               |
        |               |               |               |
    (0.0,1.0)       (1.0,1.0)       (2.0,1.0)       (3.0,1.0)
        +---------------+---------------+---------------+
        |               |               |               |
        |               |               |               |
        |   (0.5,1.5)   |   (1.5,1.5)   |   (2.5,1.5)   |
        |       *       |       *       |       *       |
        |               |               |               |
        |               |               |               |
        |               |               |               |
        +---------------+---------------+---------------+
    (0.0,2.0)       (1.0,2.0)       (2.0,2.0)       (3.0,2.0)

***************************************************************************/

#pragma once

#ifndef __POLYNEW_H__
#define __POLYNEW_H__

#include "mamecore.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_VERTEX_PARAMS					6

#define POLYFLAG_INCLUDE_BOTTOM_EDGE		0x01
#define POLYFLAG_INCLUDE_RIGHT_EDGE			0x02
#define POLYFLAG_NO_WORK_QUEUE				0x04
#define POLYFLAG_ALLOW_QUADS				0x08



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* opaque reference to the poly manager */
typedef struct _poly_manager poly_manager;


/* input vertex data */
typedef struct _poly_vertex poly_vertex;
struct _poly_vertex
{
	float		x;							/* X coordinate */
	float		y;							/* Y coordinate */
	float		p[MAX_VERTEX_PARAMS];		/* interpolated parameter values */
};


/* tri_extent describes start/end points for a scanline */
typedef struct _tri_extent tri_extent;
struct _tri_extent
{
	INT16		startx;						/* starting X coordinate (inclusive) */
	INT16		stopx;						/* ending X coordinate (exclusive) */
};


/* poly_param_extent describes information for a single parameter in an extent */
typedef struct _poly_param_extent poly_param_extent;
struct _poly_param_extent
{
	float		start;						/* parameter value at starting X,Y */
	float		dpdx;						/* dp/dx relative to starting X */
};


/* quad_extent describes start/end points for a scanline, along with per-scanline parameters */
typedef struct _quad_extent quad_extent;
struct _quad_extent
{
	INT16		startx;						/* starting X coordinate (inclusive) */
	INT16		stopx;						/* ending X coordinate (exclusive) */
	poly_param_extent param[MAX_VERTEX_PARAMS];	/* starting and dx values for each parameter */
};


/* single set of polygon per-parameter data */
typedef struct _poly_param poly_param;
struct _poly_param
{
	float		start;						/* parameter value at starting X,Y */
	float		dpdx;						/* dp/dx relative to starting X */
	float		dpdy;						/* dp/dy relative to starting Y */
};


/* polygon constant data */
typedef struct _poly_params poly_params;
struct _poly_params
{
	INT32		xorigin;					/* X origin for all parameters */
	INT32		yorigin;					/* Y origin for all parameters */
	poly_param	param[MAX_VERTEX_PARAMS];	/* array of parameter data */
};


/* callback routine to process a batch of scanlines in a triangle */
typedef void (*poly_draw_tri_scanline)(void *dest, INT32 scanline, const tri_extent *extent, const poly_params *poly, const void *extradata, int threadid);

/* callback routine to process a batch of scanlines in a triangle */
typedef void (*poly_draw_quad_scanline)(void *dest, INT32 scanline, const quad_extent *extent, const void *extradata, int threadid);



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/


/* ----- initialization/teardown ----- */

/* allocate a new poly manager that can render triangles */
poly_manager *poly_alloc(int max_polys, size_t extra_data_size, UINT8 flags);

/* free a poly manager */
void poly_free(poly_manager *poly);



/* ----- common functions ----- */

/* wait until all polygons in the queue have been rendered */
void poly_wait(poly_manager *poly, const char *debug_reason);

/* get a pointer to the extra data for the next polygon */
void *poly_get_extra_data(poly_manager *poly);



/* ----- core triangle rendering ----- */

/* render a single triangle given 3 vertexes */
UINT32 poly_render_triangle(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_tri_scanline callback, int paramcount, const poly_vertex *v1, const poly_vertex *v2, const poly_vertex *v3);

/* render a set of triangles in a fan */
UINT32 poly_render_triangle_fan(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_tri_scanline callback, int paramcount, int numverts, const poly_vertex *v);

/* perform a custom render of an object, given specific extents */
UINT32 poly_render_triangle_custom(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_tri_scanline callback, int startscanline, int numscanlines, const tri_extent *extents);



/* ----- core quad rendering ----- */

/* render a single quad given 4 vertexes */
UINT32 poly_render_quad(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_quad_scanline callback, int paramcount, const poly_vertex *v1, const poly_vertex *v2, const poly_vertex *v3, const poly_vertex *v4);

/* render a set of quads in a fan */
UINT32 poly_render_quad_fan(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_quad_scanline callback, int paramcount, int numverts, const poly_vertex *v);



/* ----- clipping ----- */

/* zclip (assumes p[0] == z) a polygon */
int poly_zclip_if_less(int numverts, const poly_vertex *v, poly_vertex *outv, int paramcount, float clipval);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    poly_param_tri_value - return parameter value
    at specified X,Y coordinate
-------------------------------------------------*/

INLINE float poly_param_tri_value(INT32 x, INT32 y, int whichparam, const poly_params *data)
{
	return data->param[whichparam].start + (float)(x - data->xorigin) * data->param[whichparam].dpdx + (float)(y - data->yorigin) * data->param[whichparam].dpdy;
}


#endif	/* __POLY_H__ */