--
-- This file contains some examples on how to use postgis-t.
-- Note: take care with the commands below!
--

DROP DATABASE IF EXISTS pg_postgist;

CREATE DATABASE pg_postgist;

\c  pg_postgist

CREATE EXTENSION postgist CASCADE;

SELECT spatiotemporal_make('ST_TRAJECTORY(2015-05-18 10:00:00;2015-05-19 11:00:00;
                           POINT(12 4), 2015-05-18 10:00:00;
                           POINT(13 5), 2015-05-18 20:00:00; 
                           POINT(15 10), 2015-05-19 11:00:00;)');

SELECT get_start_time(spatiotemporal_make('ST_TRAJECTORY(2015-05-18 10:00:00;2015-05-19 11:00:00;
                           POINT(12 4), 2015-05-18 10:00:00;
                           POINT(13 5), 2015-05-18 20:00:00; 
                           POINT(15 10), 2015-05-19 11:00:00;)'));


SELECT to_str(spatiotemporal_make('ST_TRAJECTORY(2015-05-18 10:00:00;2015-05-19 11:00:00;
                           POINT(12 4), 2015-05-18 10:00:00;
                           POINT(13 5), 2015-05-18 20:00:00; 
                           POINT(15 10), 2015-05-19 11:00:00;)'));
