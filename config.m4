dnl
dnl $ Id: $
dnl vim:se ts=2 sw=2 et:

PHP_ARG_ENABLE(geos, whether to enable geos support,
[  --enable-geos               Enable geos support])

PHP_ARG_WITH(geos-config, for geos-config,
[  --with-geos-config[=DIR]    Use geos-config])

if test -z "$PHP_DEBUG"; then
  AC_ARG_ENABLE(debug,
  [  --enable-debug          compile with debugging symbols],[
    PHP_DEBUG=$enableval
  ],[    PHP_DEBUG=no
  ])
fi

if test "$PHP_GEOS" != "no"; then
  if test "$PHP_GEOS_CONFIG" != "yes"; then
    if test -x "$PHP_GEOS_CONFIG"; then
      GEOS_CONFIG="$PHP_GEOS_CONFIG"
    fi
  else
    for i in /opt/local/bin /opt/bin /usr/local/bin /usr/bin ""; do
      if test -x $i/geos-config; then
        GEOS_CONFIG="$i/geos-config"
        break;
      fi
    done
  fi

  if test -n "$GEOS_CONFIG"; then
    GEOS_VERSION=`$GEOS_CONFIG --version`
    GEOS_INCLUDE=`$GEOS_CONFIG --includes`
    GEOS_LDFLAGS=`$GEOS_CONFIG --ldflags`
    AC_MSG_RESULT([Using GEOS version $GEOS_VERSION])
    AC_DEFINE(HAVE_GEOS_C_H,1,[Whether to have geos_c.h])
  fi

  if test -z "$GEOS_INCLUDE"; then
    AC_MSG_ERROR(Cannot find geos_c.h. Please specify correct GEOS installation path)
  fi

  if test -z "$GEOS_LDFLAGS"; then
    AC_MSG_ERROR(Cannot find geos_c.so. Please specify correct GEOS installation path)
  fi

  old_CFLAGS=$CFLAGS
  CFLAGS="-I$GEOS_INCLUDE $CFLAGS"

  old_LDFLAGS=$LDFLAGS
  LDFLAGS="$GEOS_LDFLAGS $LDFLAGS"

  AC_CHECK_HEADER(geos_c.h,, AC_MSG_ERROR(Can't find GEOS includes))
  AC_CHECK_LIB(geos_c, initGEOS_r,, AC_MSG_ERROR([Unable to build the GEOS: a newer libgeos is required]))
  AC_CHECK_LIB(geos_c, finishGEOS_r,, AC_MSG_ERROR([Unable to build the GEOS: a newer libgeos is required]))
  AC_CHECK_LIB(geos_c, GEOSClipByRect_r, AC_DEFINE(HAVE_GEOS_CLIP_BY_RECT,1,[Whether we have GEOSClipByRect_r]))
  AC_CHECK_LIB(geos_c, GEOSCoveredBy_r, AC_DEFINE(HAVE_GEOS_COVERED_BY,1,[Whether we have GEOSCoveredBy_r]))
  AC_CHECK_LIB(geos_c, GEOSCovers_r, AC_DEFINE(HAVE_GEOS_COVERS,1,[Whether we have GEOSCovers_r]))
  AC_CHECK_LIB(geos_c, GEOSDelaunayTriangulation_r, AC_DEFINE(HAVE_GEOS_DELAUNAY_TRIANGULATION,1,[Whether we have GEOSDelaunayTriangulation_r]))
  AC_CHECK_LIB(geos_c, GEOSGeomGetEndPoint_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_END_POINT,1,[Whether we have GEOSGeomGetEndPoint_r]))
  AC_CHECK_LIB(geos_c, GEOSGeomGetNumPoints_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_NUM_POINTS,1,[Whether we have GEOSGeomGetNumPoints_r]))
  AC_CHECK_LIB(geos_c, GEOSGeomGetPointN_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_POINT_N,1,[Whether we have GEOSGeomGetPointN_r]))
  AC_CHECK_LIB(geos_c, GEOSGeomGetStartPoint_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_START_POINT,1,[Whether we have GEOSGeomGetStartPoint_r]))
  AC_CHECK_LIB(geos_c, GEOSGeomGetX_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_X,1,[Whether we have GEOSGeomGetX_r]))
  AC_CHECK_LIB(geos_c, GEOSGeomGetY_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_Y,1,[Whether we have GEOSGeomGetY_r]))
  AC_CHECK_LIB(geos_c, GEOSGeom_extractUniquePoints_r, AC_DEFINE(HAVE_GEOS_GEOM_EXTRACT_UNIQUE_POINTS,1,[Whether we have GEOSGeom_extractUniquePoints_r]))
  AC_CHECK_LIB(geos_c, GEOSGeom_getCoordinateDimension_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_COORDINATE_DIMENSION,1,[Whether we have GEOSGeom_getCoordinateDimension_r]))
  AC_CHECK_LIB(geos_c, GEOSNode_r, AC_DEFINE(HAVE_GEOS_NODE,1,[Whether we have GEOSNode_r]))
  AC_CHECK_LIB(geos_c, GEOSOffsetCurve_r, AC_DEFINE(HAVE_GEOS_OFFSET_CURVE,1,[Whether we have GEOSOffsetCurve_r]))
  AC_CHECK_LIB(geos_c, GEOSPolygonize_full_r, AC_DEFINE(HAVE_GEOS_POLYGONIZE_FULL,1,[Whether we have GEOSPolygonize_full_r]))
  AC_CHECK_LIB(geos_c, GEOSRelateBoundaryNodeRule_r, AC_DEFINE(HAVE_GEOS_RELATE_BOUNDARY_NODE_RULE,1,[Whether we have GEOSRelateBoundaryNodeRule_r]))
  AC_CHECK_LIB(geos_c, GEOSRelatePatternMatch_r, AC_DEFINE(HAVE_GEOS_RELATE_PATTERN_MATCH,1,[Whether we have GEOSRelatePatternMatch_r]))
  AC_CHECK_LIB(geos_c, GEOSSharedPaths_r, AC_DEFINE(HAVE_GEOS_SHARED_PATHS,1,[Whether we have GEOSSharedPaths_r]))
  AC_CHECK_LIB(geos_c, GEOSSnap_r, AC_DEFINE(HAVE_GEOS_SNAP,1,[Whether we have GEOSSnap_r]))
  AC_CHECK_LIB(geos_c, GEOSUnaryUnion_r, AC_DEFINE(HAVE_GEOS_UNARY_UNION,1,[Whether we have GEOSUnaryUnion_r]))
  AC_CHECK_LIB(geos_c, GEOSVoronoiDiagram_r, AC_DEFINE(HAVE_GEOS_VORONOI_DIAGRAM,1,[Whether we have GEOSVoronoiDiagram_r]))
  AC_CHECK_LIB(geos_c, GEOSisClosed_r, AC_DEFINE(HAVE_GEOS_IS_CLOSED,1,[Whether we have GEOSisClosed_r]))
  AC_CHECK_LIB(geos_c, GEOSisValidDetail_r, AC_DEFINE(HAVE_GEOS_IS_VALID_DETAIL,1,[Whether we have GEOSisValidDetail_r]))
  AC_CHECK_LIB(geos_c, GEOSGeom_setPrecision_r, AC_DEFINE(HAVE_GEOS_GEOM_SET_PRECISION,1,[Whether we have GEOSGeom_setPrecision_r]))
  AC_CHECK_LIB(geos_c, GEOSGeom_getPrecision_r, AC_DEFINE(HAVE_GEOS_GEOM_GET_PRECISION,1,[Whether we have GEOSGeom_getPrecision_r]))

  AC_CHECK_LIB(geos_c, GEOSWKTWriter_setTrim_r, AC_DEFINE(HAVE_GEOS_WKT_WRITER_SET_TRIM,1,[Whether we have GEOSWKTWriter_setTrim_r]))
  AC_CHECK_LIB(geos_c, GEOSWKTWriter_setRoundingPrecision_r, AC_DEFINE(HAVE_GEOS_WKT_WRITER_SET_ROUNDING_PRECISION,1,[Whether we have GEOSWKTWriter_setRoundingPrecision_r]))
  AC_CHECK_LIB(geos_c, GEOSWKTWriter_setOutputDimension_r, AC_DEFINE(HAVE_GEOS_WKT_WRITER_SET_OUTPUT_DIMENSION,1,[Whether we have GEOSWKTWriter_setOutputDimension_r]))
  AC_CHECK_LIB(geos_c, GEOSWKTWriter_getOutputDimension_r, AC_DEFINE(HAVE_GEOS_WKT_WRITER_GET_OUTPUT_DIMENSION,1,[Whether we have GEOSWKTWriter_getOutputDimension_r]))
  AC_CHECK_LIB(geos_c, GEOSWKTWriter_setOld3D_r, AC_DEFINE(HAVE_GEOS_WKT_WRITER_SET_OLD_3D,1,[Whether we have GEOSWKTWriter_setOld3D_r]))

  AC_TRY_COMPILE(geos_c.h, GEOS_PREC_NO_TOPO, AC_DEFINE(HAVE_GEOS_PREC_NO_TOPO,1,[Whether we have GEOS_PREC_NO_TOPO]))
  AC_TRY_COMPILE(geos_c.h, GEOS_PREC_KEEP_COLLAPSED, AC_DEFINE(HAVE_GEOS_PREC_KEEP_COLLAPSED,1,[Whether we have GEOS_PREC_KEEP_COLLAPSED]))

  CFLAGS=$old_CFLAGS
  LDFLAGS=$old_LDFLAGS

  PHP_ADD_LIBRARY(geos_c, 1, GEOS_SHARED_LIBADD)
  PHP_ADD_MAKEFILE_FRAGMENT(Makefile.frag)
  PHP_SUBST(GEOS_SHARED_LIBADD)
  PHP_ADD_INCLUDE($GEOS_INCLUDE, 1)
  PHP_NEW_EXTENSION(geos, geos.c, $ext_shared,,)
fi
