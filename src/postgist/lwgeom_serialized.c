/*
  Copyright (C) 2017 National Institute For Space Research (INPE) - Brazil.

  postgis-t is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.

  postgis-t is distributed  "AS-IS" in the hope that it will be useful,
  but WITHOUT ANY WARRANTY OF ANY KIND; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with postgis-t. See LICENSE. If not, write to
  Gilberto Ribeiro de Queiroz at <gribeiro@dpi.inpe.br>.

 */

/*!
*
* \file postgist/lwgeom_serialized.c
*
* \brief 
*
* \author Gilberto Ribeiro de Queiroz
* \author Fabiana Zioti
*
* \date 2017
*
* \copyright GNU Lesser Public License version 3
*
*/

/* PostgreSQL */
#include <postgres.h>
#include <liblwgeom.h>
#include <lwgeom_pg.h>
#include <utils/timestamp.h>

/* PostGIS-T */
#include "spatiotemporal.h"
#include "wkt.h"

/* C Standard Library */
#include <assert.h>


/***********************************************************************
* Calculate the GSERIALIZED size for an LWGEOM.
*	This are functions of POSTGIS
*/

/* Private functions */

static size_t gserialized_from_any_size(const LWGEOM *geom); /* Local prototype */

static size_t gserialized_from_lwpoint_size(const LWPOINT *point)
{
	size_t size = 4; /* Type number. */

	assert(point);

	size += 4; /* Number of points (one or zero (empty)). */
	elog(INFO, "flag %d", FLAGS_NDIMS(point->flags));
	size += point->point->npoints * FLAGS_NDIMS(point->flags) * sizeof(double);

  elog(INFO, "point size = %zd", size);

	return size;
}

static size_t gserialized_from_any_size(const LWGEOM *geom)
{
	elog(INFO, "Input type: %s", lwtype_name(geom->type));

	switch (geom->type)
	{
	case POINTTYPE:
		return gserialized_from_lwpoint_size((LWPOINT *)geom);
	case LINETYPE:
	case POLYGONTYPE:
	case TRIANGLETYPE:
	case CIRCSTRINGTYPE:
	case CURVEPOLYTYPE:
	case COMPOUNDTYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTICURVETYPE:
	case MULTIPOLYGONTYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
	default:
		elog(INFO, "Unknown geometry type: %d - %s", geom->type, lwtype_name(geom->type));
		return 0;
	}
}

/***********************************************************************
* Serialize an LWGEOM into GSERIALIZED.
*/

/* Private functions */

static size_t gserialized_from_lwpoint(const LWPOINT *point, uint8_t *buf)
{
	uint8_t *loc;
	int ptsize = ptarray_point_size(point->point);
	int type = POINTTYPE;

	assert(point);
	assert(buf);

	if ( FLAGS_GET_ZM(point->flags) != FLAGS_GET_ZM(point->point->flags) )
		elog(INFO, "Dimensions mismatch in lwpoint");

	elog(INFO, "lwpoint_to_gserialized(%p, %p) called", point, buf);
	elog(INFO, "FLAGS_NDIMS(%d) ", FLAGS_NDIMS(point->flags));
	elog(INFO, "FLAGS_GET_Z(%d), FLAGS_GET_M(%d)", FLAGS_GET_Z(point->flags), FLAGS_GET_M(point->flags));
	elog(INFO, "FLAGS_GET_Z(%d), FLAGS_GET_M(%d)", FLAGS_GET_Z(point->flags), FLAGS_GET_M(point->flags)); /* Empty point */
	elog(INFO, "flags %d ", point->flags);
	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32_t));
	loc += sizeof(uint32_t);
	/* Write in the number of points (0 => empty). */
	memcpy(loc, &(point->point->npoints), sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Copy in the ordinates. */
	if ( point->point->npoints > 0 )
	{
		memcpy(loc, getPoint_internal(point->point, 0), ptsize);
		loc += ptsize;
	}

	return (size_t)(loc - buf);
}

/***********************************************************************
* De-serialize GSERIALIZED into an LWGEOM.
*/

static LWPOINT* lwpoint_from_gserialized_buffer(uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
	uint8_t *start_ptr = data_ptr;
	LWPOINT *point;
	uint32_t npoints = 0;

	// assert(data_ptr);

	elog(INFO, "alloc point start");

	point = (LWPOINT*)lwalloc(sizeof(LWPOINT));
	point->srid = SRID_UNKNOWN; /* Default */
	point->bbox = NULL;
	point->type = POINTTYPE;
	point->flags = g_flags;

	elog(INFO, "alloc point");

	data_ptr += 4; /* Skip past the type. */
	/* Zero => empty geometry */
	npoints =  2;
	 
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
	{
		elog(INFO, "npoints > 0 ");
		point->point = ptarray_construct_reference_data(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), 1, data_ptr);
	}
	else
	{
		elog(INFO, "else");
		point->point = ptarray_construct(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), 0); /* Empty point */
	}


	elog(INFO, "point %lf" , lwpoint_get_x(point));

	data_ptr += npoints * FLAGS_NDIMS(g_flags) * sizeof(double);

	if ( g_size )
		*g_size = data_ptr - start_ptr;

	return point;
}



/* Public function */

size_t gserialized_from_lwgeom_size(const LWGEOM *geom)
{
	size_t size = 8; /* Header overhead. */
	assert(geom);

	// if ( geom->bbox )
	// 	size += gbox_serialized_size(geom->flags);

	size += gserialized_from_any_size(geom);

  elog(INFO, "g_serialize size = %zd", size);

	return size;
}


/*PostGIS-T functions */


size_t lwgeom_size(const LWGEOM *lwgeom)
{
	elog(INFO, "trajectory_construct CALL");

	return gserialized_from_lwgeom_size(lwgeom);
}

size_t gserialized_from_lwgeom_point(const LWGEOM *geom, uint8_t *buf)
{
	switch (geom->type)
	{
	case POINTTYPE:
		return gserialized_from_lwpoint((LWPOINT *)geom, buf);
	default:
		elog(INFO,"Unknown geometry type: %d - %s", geom->type, lwtype_name(geom->type));
		return 0;
	}
}

LWGEOM* lwgeom_from_gserialized_buffer(uint8_t *data_ptr)
{
	uint8_t g_flags = 0;
	size_t *g_size = 0;

	elog(INFO, "ate aqui");

	return (LWGEOM *)lwpoint_from_gserialized_buffer(data_ptr, g_flags, g_size);

}
