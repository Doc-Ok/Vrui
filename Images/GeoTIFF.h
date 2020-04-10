/***********************************************************************
GeoTIFF - Definitions of important GeoTIFF TIFF tags and GeoTIFF GeoKeys
and related codes.
Numerical tag, key, and code values from GeoTIFF specification at
http://web.archive.org/web/20160814115308/http://www.remotesensing.org:80/geotiff/spec/geotiffhome.html
Copyright (c) 2018 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GEOTIFF_INCLUDED
#define GEOTIFF_INCLUDED

/***********************************************************************
Definitions of GeoTIFF tags:
***********************************************************************/

#define TIFFTAG_GEOPIXELSCALE 33550
#define TIFFTAG_GEOTIEPOINTS 33922
#define TIFFTAG_GEOTRANSMATRIX 34264
#define TIFFTAG_GEOKEYDIRECTORY 34735
#define TIFFTAG_GEODOUBLEPARAMS 34736
#define TIFFTAG_GEOASCIIPARAMS 34737
#define TIFFTAG_GDAL_METADATA 42112
#define TIFFTAG_GDAL_NODATA 42113

/***********************************************************************
Definitions of GeoTIFF keys and associated enumerated values:
***********************************************************************/

/* Common codes: */
#define GEOTIFFCODE_UNDEFINED 0
#define GEOTIFFCODE_USERDEFINED 32767

/* Shared codes: */

/* Linear units: */
#define GEOTIFFCODE_Linear_Meter 9001
#define GEOTIFFCODE_Linear_Foot 9002
#define GEOTIFFCODE_Linear_Foot_US_Survey 9003
#define GEOTIFFCODE_Linear_Foot_Modified_American 9004
#define GEOTIFFCODE_Linear_Foot_Clarke 9005
#define GEOTIFFCODE_Linear_Foot_Indian 9006
#define GEOTIFFCODE_Linear_Link 9007
#define GEOTIFFCODE_Linear_Link_Benoit 9008
#define GEOTIFFCODE_Linear_Link_Sears 9009
#define GEOTIFFCODE_Linear_Chain_Benoit 9010
#define GEOTIFFCODE_Linear_Chain_Sears 9011
#define GEOTIFFCODE_Linear_Yard_Sears 9012
#define GEOTIFFCODE_Linear_Yard_Indian 9013
#define GEOTIFFCODE_Linear_Fathom 9014
#define GEOTIFFCODE_Linear_Mile_International_Nautical 9015

/* Angular units: */
#define GEOTIFFCODE_Angular_Radian 9101
#define GEOTIFFCODE_Angular_Degree 9102
#define GEOTIFFCODE_Angular_Arc_Minute 9103
#define GEOTIFFCODE_Angular_Arc_Second 9104
#define GEOTIFFCODE_Angular_Grad 9105
#define GEOTIFFCODE_Angular_Gon 9106
#define GEOTIFFCODE_Angular_DMS 9107
#define GEOTIFFCODE_Angular_DMS_Hemisphere 9108

/* GeoTIFF configuration GeoKeys: */
#define GEOTIFFKEY_MODELTYPE 1024
	#define GEOTIFFCODE_MODELTYPEPROJECTED 1
	#define GEOTIFFCODE_MODELTYPEPGEOGRAPHIC 2
	#define GEOTIFFCODE_MODELTYPEPGEOCENTRIC 3
#define GEOTIFFKEY_RASTERTYPE 1025
	#define GEOTIFFCODE_RASTERPIXELISAREA 1
	#define GEOTIFFCODE_RASTERPIXELISPOINT 2
#define GEOTIFFKEY_CITATION 1026

/* GeoTIFF geographic CS parameter GeoKeys:*/
#define GEOTIFFKEY_GEOGRAPHICTYPE 2048
	#define GEOTIFFCODE_GCSE_WGS84 4030
	#define GEOTIFFCODE_GCSE_Clarke1880 4034
	#define GEOTIFFCODE_GCSE_Sphere 4035
	#define GEOTIFFCODE_GCS_NAD27 4267
	#define GEOTIFFCODE_GCS_NAD83 4269
#define GEOTIFFKEY_GEOGCITATION 2049
#define GEOTIFFKEY_GEOGGEODETICDATUM 2050
	#define GEOTIFFCODE_DatumE_GRS1980 6019
	#define GEOTIFFCODE_DatumE_WGS84 6030
	#define GEOTIFFCODE_DatumE_Clarke1880 6034
	#define GEOTIFFCODE_DatumE_Sphere 6035
	#define GEOTIFFCODE_Datum_North_American_Datum_1927 6267
	#define GEOTIFFCODE_Datum_North_American_Datum_1983 6269
#define GEOTIFFKEY_GEOGPRIMEMERIDIAN 2051
	#define GEOTIFFCODE_PM_Greenwich 8901
#define GEOTIFFKEY_GEOGPRIMEMEDIDIANLONG 2061
#define GEOTIFFKEY_GEOGLINEARUNITS 2052
#define GEOTIFFKEY_GEOGLINEARUNITSIZE 2053
#define GEOTIFFKEY_GEOGANGULARUNITS 2054
#define GEOTIFFKEY_GEOGANGULARUNITSIZE 2055
#define GEOTIFFKEY_GEOGELLIPSOID 2056
	#define GEOTIFFCODE_Ellipse_Clarke1866 7008
	#define GEOTIFFCODE_Ellipse_GRS1980 7019
	#define GEOTIFFCODE_Ellipse_WGS84 7030
	#define GEOTIFFCODE_Ellipse_Clarke1880 7034
	#define GEOTIFFCODE_Ellipse_Sphere 7035
#define GEOTIFFKEY_GEOGSEMIMAJORAXIS 2057
#define GEOTIFFKEY_GEOGSEMIMINORAXIS 2058
#define GEOTIFFKEY_GEOGINVFLATTENING 2059
#define GEOTIFFKEY_GEOGAZIMUTHUNITS 2060

/* GeoTIFF projected CS parameter GeoKeys: */
#define GEOTIFFKEY_PROJECTEDCSTYPE 3072
	#define GEOTIFFCODE_PCS_NAD83_UTM_BASE 26900
	#define GEOTIFFCODE_PCS_WGS84_UTM_BASE 32600
	#define GEOTIFFCODE_PCS_WGS84_UTM_NORTH_BASE 32600
	#define GEOTIFFCODE_PCS_WGS84_UTM_SOUTH_BASE 32700
#define GEOTIFFKEY_PCSCITATION 3073
#define GEOTIFFKEY_PROJECTION 3074
	#define GEOTIFFCODE_Proj_UTM_BASE 16000
	#define GEOTIFFCODE_Proj_UTM_NORTH_BASE 16000
	#define GEOTIFFCODE_Proj_UTM_SOUTH_BASE 17000
#define GEOTIFFKEY_PROJCOORDTRANS 3075
	#define GEOTIFFCODE_CT_TransverseMercator 1
	#define GEOTIFFCODE_CT_TransvMercator_Modified_Alaska 2
	#define GEOTIFFCODE_CT_ObliqueMercator 3
	#define GEOTIFFCODE_CT_ObliqueMercator_Laborde 4
	#define GEOTIFFCODE_CT_ObliqueMercator_Rosenmund 5
	#define GEOTIFFCODE_CT_ObliqueMercator_Spherical 6
	#define GEOTIFFCODE_CT_Mercator 7
	#define GEOTIFFCODE_CT_LambertConfConic_2SP 8
	#define GEOTIFFCODE_CT_LambertConfConic_Helmert 9
	#define GEOTIFFCODE_CT_LambertAzimEqualArea 10
	#define GEOTIFFCODE_CT_AlbersEqualArea 11
	#define GEOTIFFCODE_CT_AzimuthalEquidistant 12
	#define GEOTIFFCODE_CT_EquidistantConic 13
	#define GEOTIFFCODE_CT_Stereographic 14
	#define GEOTIFFCODE_CT_PolarStereographic 15
	#define GEOTIFFCODE_CT_ObliqueStereographic 16
	#define GEOTIFFCODE_CT_Equirectangular 17
	#define GEOTIFFCODE_CT_CassiniSoldner 18
	#define GEOTIFFCODE_CT_Gnomonic 19
	#define GEOTIFFCODE_CT_MillerCylindrical 20
	#define GEOTIFFCODE_CT_Orthographic 21
	#define GEOTIFFCODE_CT_Polyconic 22
	#define GEOTIFFCODE_CT_Robinson 23
	#define GEOTIFFCODE_CT_Sinusoidal 24
	#define GEOTIFFCODE_CT_VanDerGrinten 25
	#define GEOTIFFCODE_CT_NewZealandMapGrid 26
	#define GEOTIFFCODE_CT_TransvMercator_SouthOriented 27
#define GEOTIFFKEY_PROJLINEARUNITS 3076
#define GEOTIFFKEY_PROJLINEARUNITSIZE 3077
#define GEOTIFFKEY_PROJSTDPARALLEL1 3078
#define GEOTIFFKEY_PROJSTDPARALLEL2 3079
#define GEOTIFFKEY_PROJNATORIGINLONG 3080
#define GEOTIFFKEY_PROJNATORIGINLAT 3081
#define GEOTIFFKEY_PROJFALSEEASTING 3082
#define GEOTIFFKEY_PROJFALSENORTHING 3083
#define GEOTIFFKEY_PROJFALSEORIGINLONG 3084
#define GEOTIFFKEY_PROJFALSEORIGINLAT 3085
#define GEOTIFFKEY_PROJFALSEORIGINEASTING 3086
#define GEOTIFFKEY_PROJFALSEORIGINNORTHING 3087
#define GEOTIFFKEY_PROJCENTERLONG 3088
#define GEOTIFFKEY_PROJCENTERLAT 3089
#define GEOTIFFKEY_PROJCENTEREASTING 3090
#define GEOTIFFKEY_PROJCENTERNORTHING 3091
#define GEOTIFFKEY_PROJSCALEATNATORIGIN 3092
#define GEOTIFFKEY_PROJSCALEATCENTER 3093
#define GEOTIFFKEY_PROJAZIMUTHANGLE 3094
#define GEOTIFFKEY_PROJSTRAIGHTVERTPOLELONG 3095

/* GeoTIFF vertical CS parameter GeoKeys: */
#define GEOTIFFKEY_VERTICALCSTYPE 4096
#define GEOTIFFKEY_VERTICALCITATION 4097
#define GEOTIFFKEY_VERTICALDATUM 4098
#define GEOTIFFKEY_VERTICALUNITS 4099

#endif
