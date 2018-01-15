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
 * \file postgist/wkt.C
 *
 * \brief Conversion routine between Well-Kown Text respresentation and geometric objects.
 *
 * \author Gilberto Ribeiro de Queiroz
 * \author Fabiana Zioti
 *
 * \date 2017
 *
 * \copyright GNU Lesser Public License version 3
 *
 */

/*PostGIS-T extension*/
#include "wkt.h"

/* PostgreSQL */
#include <libpq/pqformat.h>
#include <utils/builtins.h>


/* C Standard Library */
#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

/*
 * WKT delimiters for input/output
 */
#define ST_WKT_TOKEN "ST_"
#define ST_WKT_TOKEN_LEN 3

#define TRAJECTORY_WKT_TOKEN "TRAJECTORY"
#define TRAJECTORY_WKT_TOKEN_LEN 10


#define POINT_WKT_TOKEN "POINT"
#define POINT_WKT_TOKEN_LEN 5


#define LDELIM '('
#define RDELIM ')'
#define COLLECTION_DELIM ';'

#define LWGEOM_SIZE_POINT 24


inline
int coord_count(char *s)
{
	int ndelim = 0;

	while ((s = strchr(s, COLLECTION_DELIM)) != NULL)
	{
	 ++ndelim;
	 ++s;
	}

	return ndelim;
}

inline
LWGEOM *position_decode(char **cp)
{
	int srid = 0;

	LWGEOM *lwgeom;

	LWGEOM_PARSER_RESULT lwg_parser_result;

	char *position;

	char *p;

	int index;

	lwgeom_parser_result_init(&lwg_parser_result);

	/*get string of location*/

	p = strchr(*cp, ',');

	index = (int)(p - (*cp));

	position = (char*) palloc(index + 1);

	memset(position, '\0', index + 1);

	strncpy(position, *cp, index);

	*cp += index;

	elog(INFO, "POSITION %s ", position);

	// code from POSTGIS

	if(lwgeom_parse_wkt(&lwg_parser_result, position, LW_PARSER_CHECK_ALL) == LW_FAILURE )
	{
			elog(INFO, "PASSER ERROR ");
			return NULL;
	}

	lwgeom = lwgeom_clone_deep(lwg_parser_result.geom);

	if ( lwgeom_needs_bbox(lwgeom) )
		lwgeom_add_bbox(lwgeom);

	lwgeom_parser_result_free(&lwg_parser_result);

	if ( lwgeom_is_empty(lwgeom))
	{
		elog(INFO, "geometry is empty");
    	lwgeom_free(lwgeom);
    	return NULL;
  	}

	return lwgeom;
}

inline
Timestamp timestamp_decode(char **cp)
{
	char *time;

	char *t;

	int index;

	t = strchr(*cp, ';');

	index = (int)(t - (*cp));

	time = (char*) palloc(index + 1);

	memset(time, '\0', index + 1);

	strncpy(time, *cp, index);

	*cp += index;

	/*elog(INFO, "time %s ", time);*/

	return DatumGetTimestamp(DirectFunctionCall3(timestamp_in, PointerGetDatum(time), PointerGetDatum(1114), PointerGetDatum(-1)));

}

/*uint8_t *sequence_decode(char *str, int ncoords, char **endptr)*/
static inline
uint8_t * sequence_decode(char *str, int ncoords, char **endptr, size_t *size)
{
	uint8_t *data = NULL;

	uint8_t *ptr = NULL;

	size_t expected_size = 0;

	size_t return_size = 0;

	/*lwgeom e position_time estao sendo substituidos toda hora*/
	/*como colocar no data ?*/

	/*expected_size = (2 * sizeof(Timestamp))+(2*sizeof(int32)) + (ncoords * (sizeof(Timestamp) + LWGEOM_SIZE_POINT));*/
	expected_size = (2*sizeof(int32)) + (2 * sizeof(Timestamp)) + (ncoords * (2 * sizeof(double)));

	elog(INFO, " ---  expected_size ---  %zd", expected_size);

	data = lwalloc(expected_size);

	ptr = data;

	/*pass size times*/
	ptr += (2 * sizeof(int32) );
	ptr += (2 * sizeof(Timestamp));


	for(int i = 0; i < ncoords; i++)
	{
		LWGEOM *lwgeom = NULL;

		Timestamp position_time = 0;

		lwgeom = position_decode(&str);

		if (lwgeom->type == POINTTYPE) {
    		/* teste para ver se o lwgeom esta correto */
    		elog(INFO, "****** geom_elem is POINTTYPE *****");
    		elog(INFO, "sizeof %lu", sizeof(lwgeom));
  	}

		/*skip , */
		++str;

		while(isspace(*str))
				str++;

		/*must have value too ?*/

		position_time = timestamp_decode(&str);


		while(isspace(*str))
				str++;

		/*skip ;*/
	  if(*str == COLLECTION_DELIM)
	      ++str;

	  while(isspace(*str))
				++str;


		/* Write in the serialized form of the gbox, if necessary. */
		//if ( geom->bbox )
		//	ptr += gserialized_from_gbox(geom->bbox, ptr);

		/* Write in the serialized form of the geometry. */
		// ptr += gserialized_from_lwgeom_point(lwgeom, ptr);

		Point *point;
		LWPOINT *lwpoint;


		lwpoint = lwgeom_as_lwpoint(lwgeom);

		point = (Point*)palloc(sizeof(Point));
		point->x = lwpoint_get_x(lwpoint);
		elog(INFO , "point->x %lf ", point->x);
		point->y = lwpoint_get_y(lwpoint);
		elog(INFO , "point->y %lf ", point->y);

		memcpy(ptr, &point->x, sizeof(double));
		ptr += sizeof(double);

		memcpy(ptr, &point->y, sizeof(double));
		ptr += sizeof(double);

		/*s serialized Time */
		// memcpy(ptr, &position_time, sizeof(Timestamp));
		// ptr += sizeof(Timestamp);

		//lwpoint_free(lwpoint);

	}

	*endptr = str;

	/* Calculate size as returned by data processing functions. */
	return_size = ptr - data;

	if ( expected_size != return_size ) /* Uh oh! */
	{
		elog(INFO, "Return size (%lu) not equal to expected size (%lu)!", return_size, expected_size);
	}

	if ( size ) /* Return the output size to the caller if necessary. */
		*size = return_size;


	return data;

	// g = gserialized_from_lwgeom(lwgeom, &ret_size);
	// if ( ! g ) lwpgerror("Unable to serialize lwgeom.");

	// SET_VARSIZE(g, ret_size);

}

struct spatiotemporal *spatiotemporal_decode(char *str)
{
	char *cp = str;

	Timestamp start_time = 0;

	Timestamp end_time = 0;

	int number_coords;

	struct spatiotemporal *st = NULL;

	size_t ret_size = 0;


	while(isspace(*cp))
		cp++;

	if(strncasecmp(cp, ST_WKT_TOKEN, ST_WKT_TOKEN_LEN) == 0)
	{
		cp += ST_WKT_TOKEN_LEN;

		/*elog(INFO, "cp %s", cp);*/

		if(strncasecmp(cp, TRAJECTORY_WKT_TOKEN, TRAJECTORY_WKT_TOKEN_LEN) == 0)
		{
			cp += TRAJECTORY_WKT_TOKEN_LEN;

			while(isspace(*cp))
				++cp;

			if(*cp !=  LDELIM)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						errmsg("invalid input syntax for type ( not found")));

			/*skip RLELIM*/
			++cp;

			while(isspace(*cp))
				++cp;

			start_time = timestamp_decode(&cp);

			/*skip ;*/
			++cp;

			end_time = timestamp_decode(&cp);

			/*skip ;*/
			++cp;

			while(isspace(*cp))
				cp++;

			// st->start_time = start_time;

			// st->end_time = end_time;



			while(isspace(*cp))
				cp++;

			/* get sequence - only for point*/
			number_coords = coord_count(cp);

			if(strncasecmp(cp, POINT_WKT_TOKEN, POINT_WKT_TOKEN_LEN) == 0)
			{
				uint8_t *data=  sequence_decode(cp, number_coords, &cp, &ret_size);

				st = (struct spatiotemporal*)data;

				if ( ! st ){
					elog(INFO, "Unable to serialize lwgeom.");
				}
				else {

					st->dummy = 0;

					st->start_time = start_time;

					st->end_time = end_time;

					SET_VARSIZE(st, ret_size);

					char *timestamp = palloc(2 * sizeof(Timestamp) + 1);

				  	timestamp = DatumGetCString(DirectFunctionCall1(timestamp_out, PointerGetDatum(st->start_time)));

				  	elog(INFO, " timestamp SERIALIZED: (%s)", timestamp);

				}


			}

			if (*cp != RDELIM)
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						errmsg("invalid input syntax for type ) not found ")));

		/* skip the ')' */
			++cp;

		/* skip spaces, if any */
			while (*cp != '\0' && isspace((unsigned char) *cp))
				++cp;

		/* if we still have characters, the WKT is invalid */
			if(*cp != '\0')
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						errmsg("invalid input syntax for type string fail")));

		}

		return st;

	}

	else
		ereport(ERROR,
			(errmsg("Invalide input for SPATIOTEMPORAL type")));


}
