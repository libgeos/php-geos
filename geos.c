/***********************************************************************
 *
 *    GEOS - Geometry Engine Open Source
 *    http://trac.osgeo.org/geos
 *
 *    Copyright (C) 2010 Sandro Santilli <strk@kbt.io>
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2.1 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor,
 *    Boston, MA  02110-1301  USA
 *
 ***********************************************************************/

/* PHP stuff */
#include "php.h"
#include "ext/standard/info.h" /* for php_info_... */
#include "Zend/zend_exceptions.h" /* for zend_throw_exception_object */

/* GEOS stuff */
#include "geos_c.h"

/* Own stuff */
#include "php_geos.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_void, 0, 0, 0)
ZEND_END_ARG_INFO()

static ZEND_DECLARE_MODULE_GLOBALS(geos);
static PHP_GINIT_FUNCTION(geos);

PHP_MINIT_FUNCTION(geos);
PHP_MSHUTDOWN_FUNCTION(geos);
PHP_RINIT_FUNCTION(geos);
PHP_RSHUTDOWN_FUNCTION(geos);
PHP_MINFO_FUNCTION(geos);
PHP_FUNCTION(GEOSVersion);
PHP_FUNCTION(GEOSPolygonize);
PHP_FUNCTION(GEOSLineMerge);

#ifdef HAVE_GEOS_SHARED_PATHS
PHP_FUNCTION(GEOSSharedPaths);
#endif

#ifdef HAVE_GEOS_RELATE_PATTERN_MATCH
PHP_FUNCTION(GEOSRelateMatch);
#endif

#if PHP_VERSION_ID < 50399
#define zend_function_entry function_entry
#endif

#if PHP_VERSION_ID >= 70000
# define GEOS_PHP_DTOR_OBJECT zend_object
# define zend_object_value zend_object *
# define zend_uint size_t
# define MAKE_STD_ZVAL(x) x = emalloc(sizeof(zval))
# define GEOS_PHP_RETURN_STRING(x) { RETVAL_STRING((x)); efree((x)); return; }
# define GEOS_PHP_RETURN_STRINGL(x,s) { RETVAL_STRINGL((x),(s)); efree((x)); return; }
# define GEOS_PHP_ADD_ASSOC_ARRAY(a,k,v) { add_assoc_string((a), (k), (v)); efree((v)); }
# define GEOS_PHP_ADD_ASSOC_ZVAL(a,k,v) { add_assoc_zval((a), (k), (v)); efree((v)); }
# define GEOS_PHP_HASH_GET_CUR_KEY(s,k,i) zend_hash_get_current_key((s), (k), (i))
# define GEOS_PHP_HASH_GET_CUR_DATA(h,d) ( d = zend_hash_get_current_data((h)) )
# define GEOS_PHP_ZVAL zval *
#else /* PHP_VERSION_ID < 70000 */
# define GEOS_PHP_DTOR_OBJECT void
# define GEOS_PHP_RETURN_STRING(x) RETURN_STRING((x),0)
# define GEOS_PHP_RETURN_STRINGL(x,s) RETURN_STRINGL((x),(s),0)
# define GEOS_PHP_ADD_ASSOC_ARRAY(a,k,v) add_assoc_string((a), (k), (v), 0)
# define GEOS_PHP_ADD_ASSOC_ZVAL(a,k,v) add_assoc_zval((a), (k), (v))
# define GEOS_PHP_HASH_GET_CUR_KEY(s,k,i) zend_hash_get_current_key((s), (k), (i), 0)
# define zend_string char
# define ZSTR_VAL(x) (x)
# define GEOS_PHP_HASH_GET_CUR_DATA(h,d) zend_hash_get_current_data((h),(void**)&(d))
# define GEOS_PHP_ZVAL zval **
#endif


static zend_function_entry geos_functions[] = {
    PHP_FE(GEOSVersion, arginfo_void)
    PHP_FE(GEOSPolygonize, arginfo_void)
    PHP_FE(GEOSLineMerge, arginfo_void)

#   ifdef HAVE_GEOS_SHARED_PATHS
    PHP_FE(GEOSSharedPaths, arginfo_void)
#   endif

#   ifdef HAVE_GEOS_RELATE_PATTERN_MATCH
    PHP_FE(GEOSRelateMatch, arginfo_void)
#   endif
    {NULL, NULL, NULL}
};

zend_module_entry geos_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_GEOS_EXTNAME,
    geos_functions,
    PHP_MINIT(geos),              /* module init function */
    PHP_MSHUTDOWN(geos),          /* module shutdown function */
    PHP_RINIT(geos),              /* request init function */
    PHP_RSHUTDOWN(geos),          /* request shutdown function */
    PHP_MINFO(geos),              /* module info function */
    PHP_GEOS_VERSION,
    PHP_MODULE_GLOBALS(geos),     /* globals descriptor */
    PHP_GINIT(geos),              /* globals ctor */
    NULL,                         /* globals dtor */
    NULL,                         /* post deactivate */
    STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_GEOS
ZEND_GET_MODULE(geos)
#endif

/* -- Utility functions ---------------------- */

static void noticeHandler(const char *fmt, ...)
{
    
    char message[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message) - 1, fmt, args);
    va_end(args);

    php_error_docref(NULL, E_NOTICE, "%s", message);
}

static void errorHandler(const char *fmt, ...)
{
    
    char message[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message) - 1, fmt, args);
    va_end(args);

    /* TODO: use a GEOSException ? */
    zend_throw_exception_ex(zend_exception_get_default(),
        1, "%s", message);

}

typedef struct Proxy_t {
#if PHP_VERSION_ID >= 70000
    int id;
    void* relay;
    zend_object std;
#else
    zend_object std;
    void* relay;
#endif
} Proxy;

#if PHP_VERSION_ID >= 70000
static inline Proxy *php_geos_fetch_object(zend_object *obj) {
  return (Proxy *)((char *) obj - XtOffsetOf(Proxy, std));
}
# define Z_GEOS_OBJ_P(zv) (Proxy *)((char *) (Z_OBJ_P(zv)) - XtOffsetOf(Proxy, std))
#else
# ifdef Z_OBJ
#  define Z_GEOS_OBJ_P(zv) (Proxy*)Z_OBJ(*val)
# else
#  define Z_GEOS_OBJ_P(zv) (Proxy*)zend_object_store_get_object(val)
# endif
#endif

static void
setRelay(zval* val, void* obj) {
    

    Proxy* proxy = Z_GEOS_OBJ_P(val);

    proxy->relay = obj;
}

static inline void *
getRelay(zval* val, zend_class_entry* ce) {
    

    Proxy* proxy = Z_GEOS_OBJ_P(val);

    if ( proxy->std.ce != ce ) {
        php_error_docref(NULL, E_ERROR,
#if PHP_VERSION_ID >= 70000
            "Relay object is not an %s", ZSTR_VAL(ce->name));
#else
            "Relay object is not an %s", ce->name);
#endif
    }
    if ( ! proxy->relay ) {
            php_error_docref(NULL, E_ERROR,
#if PHP_VERSION_ID >= 70000
            "Relay object for object of type %s is not set", ZSTR_VAL(ce->name));
#else
            "Relay object for object of type %s is not set", ce->name);
#endif
    }
    return proxy->relay;
}

static long getZvalAsLong(GEOS_PHP_ZVAL val)
{
    long ret;
    zval tmp;

#if PHP_VERSION_ID >= 70000
    tmp = *val;
#else
    tmp = **val;
#endif
    zval_copy_ctor(&tmp);
    convert_to_long(&tmp);
    ret = Z_LVAL(tmp);
    zval_dtor(&tmp);
    return ret;
}

static long getZvalAsDouble(GEOS_PHP_ZVAL val)
{
    double ret;
    zval tmp;

#if PHP_VERSION_ID >= 70000
    tmp = *val;
#else
    tmp = **val;
#endif
    zval_copy_ctor(&tmp);
    convert_to_double(&tmp);
    ret = Z_DVAL(tmp);
    zval_dtor(&tmp);
    return ret;
}

static zend_object_value
Gen_create_obj (zend_class_entry *type,
    void (*dtor)(GEOS_PHP_DTOR_OBJECT *object),
    zend_object_handlers* handlers)
{
    

#if PHP_VERSION_ID >= 70000

    Proxy *obj = (Proxy *) ecalloc(1, sizeof(Proxy) + zend_object_properties_size(type));

    zend_object_std_init(&obj->std, type);
    object_properties_init(&obj->std, type);

    obj->std.handlers = handlers;

    /* TODO: install the destructor ? (dtor) ! */
    /* TODO: do not allocate a full Proxy if we're going to use an object */

    return &obj->std;

#else /* PHP_VERSION_ID < 70000 */

    zend_object_value retval;

    Proxy *obj = (Proxy *)ecalloc(1, sizeof(Proxy));

    obj->std.ce = type;

    ALLOC_HASHTABLE(obj->std.properties);
    zend_hash_init(obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, NULL, sizeof(zval *));
#else
    object_properties_init(&(obj->std), type);
#endif

    retval.handle = zend_objects_store_put(obj, NULL, dtor, NULL);
    retval.handlers = handlers;

    return retval;

#endif /* PHP_VERSION_ID < 70000 */
}


/* -- class GEOSGeometry -------------------- */

PHP_METHOD(Geometry, __construct);
PHP_METHOD(Geometry, __toString);
PHP_METHOD(Geometry, project);
PHP_METHOD(Geometry, interpolate);
PHP_METHOD(Geometry, buffer);

#ifdef HAVE_GEOS_OFFSET_CURVE
PHP_METHOD(Geometry, offsetCurve);
#endif

PHP_METHOD(Geometry, envelope);
PHP_METHOD(Geometry, intersection);
PHP_METHOD(Geometry, convexHull);
PHP_METHOD(Geometry, difference);
PHP_METHOD(Geometry, symDifference);
PHP_METHOD(Geometry, boundary);
PHP_METHOD(Geometry, union); /* also does union cascaded */
PHP_METHOD(Geometry, pointOnSurface);
PHP_METHOD(Geometry, centroid);
PHP_METHOD(Geometry, relate);

#ifdef HAVE_GEOS_RELATE_BOUNDARY_NODE_RULE
PHP_METHOD(Geometry, relateBoundaryNodeRule);
#endif

PHP_METHOD(Geometry, simplify); /* also does topology-preserving */
PHP_METHOD(Geometry, normalize);

#ifdef HAVE_GEOS_GEOM_SET_PRECISION
PHP_METHOD(Geometry, setPrecision);
#endif

#ifdef HAVE_GEOS_GEOM_GET_PRECISION
PHP_METHOD(Geometry, getPrecision);
#endif

#ifdef HAVE_GEOS_GEOM_EXTRACT_UNIQUE_POINTS
PHP_METHOD(Geometry, extractUniquePoints);
#endif

PHP_METHOD(Geometry, disjoint);
PHP_METHOD(Geometry, touches);
PHP_METHOD(Geometry, intersects);
PHP_METHOD(Geometry, crosses);
PHP_METHOD(Geometry, within);
PHP_METHOD(Geometry, contains);
PHP_METHOD(Geometry, overlaps);

#ifdef HAVE_GEOS_COVERS
PHP_METHOD(Geometry, covers);
#endif

#ifdef HAVE_GEOS_COVERED_BY
PHP_METHOD(Geometry, coveredBy);
#endif

PHP_METHOD(Geometry, equals);
PHP_METHOD(Geometry, equalsExact);
PHP_METHOD(Geometry, isEmpty);

#ifdef HAVE_GEOS_IS_VALID_DETAIL
PHP_METHOD(Geometry, checkValidity);
#endif

PHP_METHOD(Geometry, isSimple);
PHP_METHOD(Geometry, isRing);
PHP_METHOD(Geometry, hasZ);

#ifdef HAVE_GEOS_IS_CLOSED
PHP_METHOD(Geometry, isClosed);
#endif

PHP_METHOD(Geometry, typeName);
PHP_METHOD(Geometry, typeId);
PHP_METHOD(Geometry, getSRID);
PHP_METHOD(Geometry, setSRID);
PHP_METHOD(Geometry, numGeometries);
PHP_METHOD(Geometry, geometryN);
PHP_METHOD(Geometry, numInteriorRings);

#ifdef HAVE_GEOS_GEOM_GET_NUM_POINTS
PHP_METHOD(Geometry, numPoints);
#endif

#ifdef HAVE_GEOS_GEOM_GET_X
PHP_METHOD(Geometry, getX);
#endif

#ifdef HAVE_GEOS_GEOM_GET_Y
PHP_METHOD(Geometry, getY);
#endif

PHP_METHOD(Geometry, interiorRingN);
PHP_METHOD(Geometry, exteriorRing);
PHP_METHOD(Geometry, numCoordinates);
PHP_METHOD(Geometry, dimension);

#ifdef HAVE_GEOS_GEOM_GET_COORDINATE_DIMENSION
PHP_METHOD(Geometry, coordinateDimension);
#endif

#ifdef HAVE_GEOS_GEOM_GET_POINT_N
PHP_METHOD(Geometry, pointN);
#endif

#ifdef HAVE_GEOS_GEOM_GET_START_POINT
PHP_METHOD(Geometry, startPoint);
#endif

#ifdef HAVE_GEOS_GEOM_GET_END_POINT
PHP_METHOD(Geometry, endPoint);
#endif

PHP_METHOD(Geometry, area);
PHP_METHOD(Geometry, length);
PHP_METHOD(Geometry, distance);
PHP_METHOD(Geometry, hausdorffDistance);

#ifdef HAVE_GEOS_SNAP
PHP_METHOD(Geometry, snapTo);
#endif

#ifdef HAVE_GEOS_NODE
PHP_METHOD(Geometry, node);
#endif

#ifdef HAVE_GEOS_DELAUNAY_TRIANGULATION
PHP_METHOD(Geometry, delaunayTriangulation);
#endif

#ifdef HAVE_GEOS_VORONOI_DIAGRAM
PHP_METHOD(Geometry, voronoiDiagram);
#endif

#ifdef HAVE_GEOS_CLIP_BY_RECT
PHP_METHOD(Geometry, clipByRect);
#endif

static zend_function_entry Geometry_methods[] = {
    PHP_ME(Geometry, __construct, arginfo_void, 0)
    PHP_ME(Geometry, __toString, arginfo_void, 0)
    PHP_ME(Geometry, project, arginfo_void, 0)
    PHP_ME(Geometry, interpolate, arginfo_void, 0)
    PHP_ME(Geometry, buffer, arginfo_void, 0)

#   ifdef HAVE_GEOS_OFFSET_CURVE
    PHP_ME(Geometry, offsetCurve, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, envelope, arginfo_void, 0)
    PHP_ME(Geometry, intersection, arginfo_void, 0)
    PHP_ME(Geometry, convexHull, arginfo_void, 0)
    PHP_ME(Geometry, difference, arginfo_void, 0)
    PHP_ME(Geometry, symDifference, arginfo_void, 0)
    PHP_ME(Geometry, boundary, arginfo_void, 0)
    PHP_ME(Geometry, union, arginfo_void, 0)
    PHP_ME(Geometry, pointOnSurface, arginfo_void, 0)
    PHP_ME(Geometry, centroid, arginfo_void, 0)
    PHP_ME(Geometry, relate, arginfo_void, 0)

#   ifdef HAVE_GEOS_RELATE_BOUNDARY_NODE_RULE
    PHP_ME(Geometry, relateBoundaryNodeRule, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, simplify, arginfo_void, 0)
    PHP_ME(Geometry, normalize, arginfo_void, 0)

#   ifdef HAVE_GEOS_GEOM_SET_PRECISION
    PHP_ME(Geometry, setPrecision, arginfo_void, 0)
#   endif

#   if HAVE_GEOS_GEOM_GET_PRECISION
    PHP_ME(Geometry, getPrecision, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_GEOM_EXTRACT_UNIQUE_POINTS
    PHP_ME(Geometry, extractUniquePoints, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, disjoint, arginfo_void, 0)
    PHP_ME(Geometry, touches, arginfo_void, 0)
    PHP_ME(Geometry, intersects, arginfo_void, 0)
    PHP_ME(Geometry, crosses, arginfo_void, 0)
    PHP_ME(Geometry, within, arginfo_void, 0)
    PHP_ME(Geometry, contains, arginfo_void, 0)
    PHP_ME(Geometry, overlaps, arginfo_void, 0)

#   ifdef HAVE_GEOS_COVERS
    PHP_ME(Geometry, covers, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_COVERED_BY
    PHP_ME(Geometry, coveredBy, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, equals, arginfo_void, 0)
    PHP_ME(Geometry, equalsExact, arginfo_void, 0)
    PHP_ME(Geometry, isEmpty, arginfo_void, 0)

#   ifdef HAVE_GEOS_IS_VALID_DETAIL
    PHP_ME(Geometry, checkValidity, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, isSimple, arginfo_void, 0)
    PHP_ME(Geometry, isRing, arginfo_void, 0)
    PHP_ME(Geometry, hasZ, arginfo_void, 0)

#   ifdef HAVE_GEOS_IS_CLOSED
    PHP_ME(Geometry, isClosed, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, typeName, arginfo_void, 0)
    PHP_ME(Geometry, typeId, arginfo_void, 0)
    PHP_ME(Geometry, getSRID, arginfo_void, 0)
    PHP_ME(Geometry, setSRID, arginfo_void, 0)
    PHP_ME(Geometry, numGeometries, arginfo_void, 0)
    PHP_ME(Geometry, geometryN, arginfo_void, 0)
    PHP_ME(Geometry, numInteriorRings, arginfo_void, 0)

#   ifdef HAVE_GEOS_GEOM_GET_NUM_POINTS
    PHP_ME(Geometry, numPoints, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_GEOM_GET_X
    PHP_ME(Geometry, getX, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_GEOM_GET_Y
    PHP_ME(Geometry, getY, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, interiorRingN, arginfo_void, 0)
    PHP_ME(Geometry, exteriorRing, arginfo_void, 0)
    PHP_ME(Geometry, numCoordinates, arginfo_void, 0)
    PHP_ME(Geometry, dimension, arginfo_void, 0)

#   ifdef HAVE_GEOS_GEOM_GET_COORDINATE_DIMENSION
    PHP_ME(Geometry, coordinateDimension, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_GEOM_GET_POINT_N
    PHP_ME(Geometry, pointN, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_GEOM_GET_START_POINT
    PHP_ME(Geometry, startPoint, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_GEOM_GET_END_POINT
    PHP_ME(Geometry, endPoint, arginfo_void, 0)
#   endif

    PHP_ME(Geometry, area, arginfo_void, 0)
    PHP_ME(Geometry, length, arginfo_void, 0)
    PHP_ME(Geometry, distance, arginfo_void, 0)
    PHP_ME(Geometry, hausdorffDistance, arginfo_void, 0)

#   if HAVE_GEOS_SNAP
    PHP_ME(Geometry, snapTo, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_NODE
    PHP_ME(Geometry, node, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_DELAUNAY_TRIANGULATION
    PHP_ME(Geometry, delaunayTriangulation, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_VORONOI_DIAGRAM
    PHP_ME(Geometry, voronoiDiagram, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_CLIP_BY_RECT
    PHP_ME(Geometry, clipByRect, arginfo_void, 0)
#   endif

    {NULL, NULL, NULL}
};

static zend_class_entry *Geometry_ce_ptr;

static zend_object_handlers Geometry_object_handlers;

/* Geometry serializer */

static GEOSWKBWriter* Geometry_serializer = 0;

static GEOSWKBWriter* getGeometrySerializer()
{
    

    if ( ! Geometry_serializer ) {
        Geometry_serializer = GEOSWKBWriter_create_r(GEOS_G(handle));
        GEOSWKBWriter_setIncludeSRID_r(GEOS_G(handle), Geometry_serializer, 1);
        GEOSWKBWriter_setOutputDimension_r(GEOS_G(handle), Geometry_serializer, 3);
    }
    return Geometry_serializer;
}

static void delGeometrySerializer()
{
    

    if ( Geometry_serializer ) {
        GEOSWKBWriter_destroy_r(GEOS_G(handle), Geometry_serializer);
    }
}

/* Geometry deserializer */

static GEOSWKBReader* Geometry_deserializer = 0;

static GEOSWKBReader* getGeometryDeserializer()
{
    

    if ( ! Geometry_deserializer ) {
        Geometry_deserializer = GEOSWKBReader_create_r(GEOS_G(handle));
    }
    return Geometry_deserializer;
}

static void delGeometryDeserializer()
{
    

    if ( Geometry_deserializer ) {
        GEOSWKBReader_destroy_r(GEOS_G(handle), Geometry_deserializer);
    }
}

/* Serializer function for GEOSGeometry */

static int
Geometry_serialize(zval *object, unsigned char **buffer, zend_uint *buf_len,
        zend_serialize_data *data)
{
    GEOSWKBWriter *serializer;
    GEOSGeometry *geom;
    char* ret;
    size_t retsize;


    serializer = getGeometrySerializer();
    geom = (GEOSGeometry*)getRelay(object, Geometry_ce_ptr);

    /* NOTE: we might be fine using binary here */
    ret = (char*)GEOSWKBWriter_writeHEX_r(GEOS_G(handle), serializer, geom, &retsize);
    /* we'll probably get an exception if ret is null */
    if ( ! ret ) return FAILURE;

    *buffer = (unsigned char*)estrndup(ret, retsize);
    GEOSFree_r(GEOS_G(handle), ret);

    *buf_len = retsize;

    return SUCCESS;
}

static int
Geometry_deserialize(GEOS_PHP_ZVAL object, zend_class_entry *ce, const unsigned char *buf,
        zend_uint buf_len, zend_unserialize_data *data)
{
    GEOSWKBReader* deserializer;
    GEOSGeometry* geom;

    deserializer = getGeometryDeserializer();
    geom = GEOSWKBReader_readHEX_r(GEOS_G(handle), deserializer, buf, buf_len);

    /* check zend_class_entry being what we expect! */
    if ( ce != Geometry_ce_ptr ) {
        php_error_docref(NULL, E_ERROR,
                "Geometry_deserialize called with unexpected zend_class_entry");
        return FAILURE;
    }
#if PHP_VERSION_ID >= 70000
    object_init_ex(object, ce);
    setRelay(object, geom);
#else
    object_init_ex(*object, ce);
    setRelay(*object, geom);
#endif

    return SUCCESS;
}

/*
 * Push components of the given geometry
 * to the given array zval.
 * Components geometries are cloned.
 * NOTE: collection components are not descended into
 */
static void
dumpGeometry(GEOSGeometry* g, zval* array)
{
    
    int ngeoms, i;

    ngeoms = GEOSGetNumGeometries_r(GEOS_G(handle), g);
    for (i=0; i<ngeoms; ++i)
    {
        zval *tmp;
        GEOSGeometry* cc;
        const GEOSGeometry* c = GEOSGetGeometryN_r(GEOS_G(handle), g, i);
        if ( ! c ) continue; /* should get an exception */
        /* we _need_ to clone as this one is owned by 'g' */
        cc = GEOSGeom_clone_r(GEOS_G(handle), c);
        if ( ! cc ) continue; /* should get an exception */

        MAKE_STD_ZVAL(tmp);
        object_init_ex(tmp, Geometry_ce_ptr);
        setRelay(tmp, cc);
        add_next_index_zval(array, tmp);
#if PHP_VERSION_ID >= 70000
        efree(tmp);
#endif
    }
}


static void
Geometry_dtor (GEOS_PHP_DTOR_OBJECT *object)
{
#if PHP_VERSION_ID < 70000
    Proxy *obj = (Proxy *)object;
#else
    Proxy *obj = php_geos_fetch_object(object);
#endif

    GEOSGeom_destroy_r(GEOS_G(handle), (GEOSGeometry*)obj->relay);

#if PHP_VERSION_ID >= 70000
    //zend_object_std_dtor(&obj->std);
#else
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);

    efree(obj);
#endif
}

static zend_object_value
Geometry_create_obj (zend_class_entry *type)
{
    return Gen_create_obj(type, Geometry_dtor, &Geometry_object_handlers);
}


PHP_METHOD(Geometry, __construct)
{
    php_error_docref(NULL, E_ERROR,
            "GEOSGeometry can't be constructed using new, check WKTReader");

}

PHP_METHOD(Geometry, __toString)
{
    GEOSGeometry *geom;
    GEOSWKTWriter *writer;
    char *wkt;
    char *ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);
    writer = GEOSWKTWriter_create_r(GEOS_G(handle));
    /* NOTE: if we get an exception before reaching
     *       GEOSWKTWriter_destory below we'll be leaking memory.
     *       One fix could be storing the object in a refcounted
     *       zval.
     */
#   ifdef HAVE_GEOS_WKT_WRITER_SET_TRIM
    GEOSWKTWriter_setTrim_r(GEOS_G(handle), writer, 1);
#   endif

    wkt = GEOSWKTWriter_write_r(GEOS_G(handle), writer, geom);
    /* we'll probably get an exception if wkt is null */
    if ( ! wkt ) RETURN_NULL();

    GEOSWKTWriter_destroy_r(GEOS_G(handle), writer);


    ret = estrdup(wkt);
    GEOSFree_r(GEOS_G(handle), wkt);

    GEOS_PHP_RETURN_STRING(ret);
}

PHP_METHOD(Geometry, project)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    zval *zobj;
    zend_bool normalized = 0;
    double ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o|b", &zobj,
            &normalized) == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    if ( normalized ) {
        ret = GEOSProjectNormalized_r(GEOS_G(handle), this, other);
    } else {
        ret = GEOSProject_r(GEOS_G(handle), this, other);
    }
    if ( ret < 0 ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(ret);
}

PHP_METHOD(Geometry, interpolate)
{
    GEOSGeometry *this;
    double dist;
    GEOSGeometry *ret;
    zend_bool normalized = 0;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "d|b",
            &dist, &normalized) == FAILURE) {
        RETURN_NULL();
    }

    if ( normalized ) {
        ret = GEOSInterpolateNormalized_r(GEOS_G(handle), this, dist);
    } else {
        ret = GEOSInterpolate_r(GEOS_G(handle), this, dist);
    }
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry::buffer(dist, [<styleArray>])
 *
 * styleArray keys supported:
 *  'quad_segs'
 *       Type: int
 *       Number of segments used to approximate
 *       a quarter circle (defaults to 8).
 *  'endcap'
 *       Type: long
 *       Endcap style (defaults to GEOSBUF_CAP_ROUND)
 *  'join'
 *       Type: long
 *       Join style (defaults to GEOSBUF_JOIN_ROUND)
 *  'mitre_limit'
 *       Type: double
 *       mitre ratio limit (only affects joins with GEOSBUF_JOIN_MITRE style)
 *       'miter_limit' is also accepted as a synonym for 'mitre_limit'.
 *  'single_sided'
 *       Type: bool
 *       If true buffer lines only on one side, so that the input line
 *       will be a portion of the boundary of the returned polygon.
 *       Only applies to lineal input. Defaults to false.
 */
PHP_METHOD(Geometry, buffer)
{
    GEOSGeometry *this;
    double dist;
    GEOSGeometry *ret;
    GEOSBufferParams *params;
    static const double default_mitreLimit = 5.0;
    static const int default_endCapStyle = GEOSBUF_CAP_ROUND;
    static const int default_joinStyle = GEOSBUF_JOIN_ROUND;
    static const int default_quadSegs = 8;
    long int quadSegs = default_quadSegs;
    long int endCapStyle = default_endCapStyle;
    long int joinStyle = default_joinStyle;
    double mitreLimit = default_mitreLimit;
    long singleSided = 0;
    zval *style_val = NULL;
    GEOS_PHP_ZVAL data;
    HashTable *style;
    zend_string *key;
    zend_ulong index;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "d|a",
            &dist, &style_val) == FAILURE) {
        RETURN_NULL();
    }

    params = GEOSBufferParams_create_r(GEOS_G(handle));

    if ( style_val )
    {
        style = HASH_OF(style_val);
        while(GEOS_PHP_HASH_GET_CUR_KEY(style, &key, &index)
              == HASH_KEY_IS_STRING)
        {
            if(!strcmp(ZSTR_VAL(key), "quad_segs"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                quadSegs = getZvalAsLong(data);
                GEOSBufferParams_setQuadrantSegments_r(GEOS_G(handle), params, quadSegs);
            }
            else if(!strcmp(ZSTR_VAL(key), "endcap"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                endCapStyle = getZvalAsLong(data);
                GEOSBufferParams_setEndCapStyle_r(GEOS_G(handle), params, endCapStyle);
            }
            else if(!strcmp(ZSTR_VAL(key), "join"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                joinStyle = getZvalAsLong(data);
                GEOSBufferParams_setJoinStyle_r(GEOS_G(handle), params, joinStyle);
            }
            else if(!strcmp(ZSTR_VAL(key), "mitre_limit"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                mitreLimit = getZvalAsDouble(data);
                GEOSBufferParams_setMitreLimit_r(GEOS_G(handle), params, mitreLimit);
            }
            else if(!strcmp(ZSTR_VAL(key), "single_sided"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                singleSided = getZvalAsLong(data);
                GEOSBufferParams_setSingleSided_r(GEOS_G(handle), params, singleSided);
            }

            zend_hash_move_forward(style);
        }
    }

    ret = GEOSBufferWithParams_r(GEOS_G(handle), this, params, dist);
    GEOSBufferParams_destroy_r(GEOS_G(handle), params);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry::offsetCurve(dist, [<styleArray>])
 *
 * styleArray keys supported:
 *  'quad_segs'
 *       Type: int
 *       Number of segments used to approximate
 *       a quarter circle (defaults to 8).
 *  'join'
 *       Type: long
 *       Join style (defaults to GEOSBUF_JOIN_ROUND)
 *  'mitre_limit'
 *       Type: double
 *       mitre ratio limit (only affects joins with GEOSBUF_JOIN_MITRE style)
 *       'miter_limit' is also accepted as a synonym for 'mitre_limit'.
 */
#ifdef HAVE_GEOS_OFFSET_CURVE
PHP_METHOD(Geometry, offsetCurve)
{
    GEOSGeometry *this;
    double dist;
    GEOSGeometry *ret;
    static const double default_mitreLimit = 5.0;
    static const int default_joinStyle = GEOSBUF_JOIN_ROUND;
    static const int default_quadSegs = 8;
    long int quadSegs = default_quadSegs;
    long int joinStyle = default_joinStyle;
    double mitreLimit = default_mitreLimit;
    zval *style_val = NULL;
    GEOS_PHP_ZVAL data;
    HashTable *style;
    zend_string *key;
    zend_ulong index;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "d|a",
            &dist, &style_val) == FAILURE) {
        RETURN_NULL();
    }

    if ( style_val )
    {
        style = HASH_OF(style_val);
        while(GEOS_PHP_HASH_GET_CUR_KEY(style, &key, &index)
              == HASH_KEY_IS_STRING)
        {
            if(!strcmp(ZSTR_VAL(key), "quad_segs"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                quadSegs = getZvalAsLong(data);
            }
            else if(!strcmp(ZSTR_VAL(key), "join"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                joinStyle = getZvalAsLong(data);
            }
            else if(!strcmp(ZSTR_VAL(key), "mitre_limit"))
            {
                GEOS_PHP_HASH_GET_CUR_DATA(style, data);
                mitreLimit = getZvalAsDouble(data);
            }

            zend_hash_move_forward(style);
        }
    }

    ret = GEOSOffsetCurve_r(GEOS_G(handle), this, dist, quadSegs, joinStyle, mitreLimit);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif

PHP_METHOD(Geometry, envelope)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSEnvelope_r(GEOS_G(handle), this);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

PHP_METHOD(Geometry, intersection)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    GEOSGeometry *ret;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSIntersection_r(GEOS_G(handle), this, other);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry GEOSGeometry::clipByRect(xmin,ymin,xmax,ymax)
 */
#ifdef HAVE_GEOS_CLIP_BY_RECT
PHP_METHOD(Geometry, clipByRect)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;
    double xmin,ymin,xmax,ymax;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "dddd",
            &xmin, &ymin, &xmax, &ymax) == FAILURE) {
        RETURN_NULL();
    }

    ret = GEOSClipByRect_r(GEOS_G(handle), this, xmin, ymin, xmax, ymax);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif

PHP_METHOD(Geometry, convexHull)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSConvexHull_r(GEOS_G(handle), this);
    if ( ret == NULL ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

PHP_METHOD(Geometry, difference)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    GEOSGeometry *ret;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSDifference_r(GEOS_G(handle), this, other);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

PHP_METHOD(Geometry, symDifference)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    GEOSGeometry *ret;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSSymDifference_r(GEOS_G(handle), this, other);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

PHP_METHOD(Geometry, boundary)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSBoundary_r(GEOS_G(handle), this);
    if ( ret == NULL ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry::union(otherGeom)
 * GEOSGeometry::union()
 */
PHP_METHOD(Geometry, union)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    GEOSGeometry *ret;
    zval *zobj = NULL;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }

    if ( zobj ) {
        other = getRelay(zobj, Geometry_ce_ptr);
        ret = GEOSUnion_r(GEOS_G(handle), this, other);
    } else {
#       ifdef HAVE_GEOS_UNARY_UNION
        ret = GEOSUnaryUnion_r(GEOS_G(handle), this);
#       else
        ret = GEOSUnionCascaded_r(GEOS_G(handle), this);
#       endif
    }

    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry::pointOnSurface()
 */
PHP_METHOD(Geometry, pointOnSurface)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSPointOnSurface_r(GEOS_G(handle), this);
    if ( ret == NULL ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry::centroid()
 */
PHP_METHOD(Geometry, centroid)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGetCentroid_r(GEOS_G(handle), this);
    if ( ret == NULL ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry::relate(otherGeom)
 * GEOSGeometry::relate(otherGeom, pattern)
 */
PHP_METHOD(Geometry, relate)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    zval *zobj;
    char* pat = NULL;
#if PHP_VERSION_ID >= 70000
    size_t patlen;
#else
    int patlen;
#endif
    int retInt;
    zend_bool retBool;
    char* retStr;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o|s",
        &zobj, &pat, &patlen) == FAILURE)
    {
        RETURN_NULL();
    }

    other = getRelay(zobj, Geometry_ce_ptr);

    if ( ! pat ) {
        /* we'll compute it */
        pat = GEOSRelate_r(GEOS_G(handle), this, other);
        if ( ! pat ) RETURN_NULL(); /* should get an exception first */
        retStr = estrdup(pat);
        GEOSFree_r(GEOS_G(handle), pat);
        GEOS_PHP_RETURN_STRING(retStr);
    } else {
        retInt = GEOSRelatePattern_r(GEOS_G(handle), this, other, pat);
        if ( retInt == 2 ) RETURN_NULL(); /* should get an exception first */
        retBool = retInt;
        RETURN_BOOL(retBool);
    }

}

/**
 * GEOSGeometry::relateBoundaryNodeRule(otherGeom, rule)
 */
#ifdef HAVE_GEOS_RELATE_BOUNDARY_NODE_RULE
PHP_METHOD(Geometry, relateBoundaryNodeRule)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    zval *zobj;
    char* pat;
    long int bnr = GEOSRELATE_BNR_OGC;
    char* retStr;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ol",
        &zobj, &bnr) == FAILURE)
    {
        RETURN_NULL();
    }

    other = getRelay(zobj, Geometry_ce_ptr);

    /* we'll compute it */
    pat = GEOSRelateBoundaryNodeRule_r(GEOS_G(handle), this, other, bnr);
    if ( ! pat ) RETURN_NULL(); /* should get an exception first */
    retStr = estrdup(pat);
    GEOSFree_r(GEOS_G(handle), pat);
    GEOS_PHP_RETURN_STRING(retStr);
}
#endif

/**
 * GEOSGeometry GEOSGeometry::simplify(tolerance)
 * GEOSGeometry GEOSGeometry::simplify(tolerance, preserveTopology)
 */
PHP_METHOD(Geometry, simplify)
{
    GEOSGeometry *this;
    double tolerance;
    zend_bool preserveTopology = 0;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "d|b",
            &tolerance, &preserveTopology) == FAILURE) {
        RETURN_NULL();
    }

    if ( preserveTopology ) {
        ret = GEOSTopologyPreserveSimplify_r(GEOS_G(handle), this, tolerance);
    } else {
        ret = GEOSSimplify_r(GEOS_G(handle), this, tolerance);
    }

    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry GEOSGeometry::setPrecision(gridsize, [flags])
 */
#ifdef HAVE_GEOS_GEOM_SET_PRECISION
PHP_METHOD(Geometry, setPrecision)
{
    GEOSGeometry *this;
    double gridSize;
    long int flags = 0;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "d|l",
            &gridSize, &flags) == FAILURE) {
        RETURN_NULL();
    }

    ret = GEOSGeom_setPrecision_r(GEOS_G(handle), this, gridSize, flags);

    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif

/**
 * double GEOSGeometry::getPrecision()
 */
#ifdef HAVE_GEOS_GEOM_GET_PRECISION
PHP_METHOD(Geometry, getPrecision)
{
    GEOSGeometry *geom;
    double prec;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    prec = GEOSGeom_getPrecision_r(GEOS_G(handle), geom);
    if ( prec < 0 ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(prec);
}
#endif

/**
 * GEOSGeometry GEOSGeometry::normalize()
 */
PHP_METHOD(Geometry, normalize)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGeom_clone_r(GEOS_G(handle), this);

    if ( ! ret ) RETURN_NULL();

    GEOSNormalize_r(GEOS_G(handle), ret); /* exception should be gotten automatically */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}

/**
 * GEOSGeometry GEOSGeometry::extractUniquePoints()
 */
#ifdef HAVE_GEOS_GEOM_EXTRACT_UNIQUE_POINTS
PHP_METHOD(Geometry, extractUniquePoints)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGeom_extractUniquePoints_r(GEOS_G(handle), this);
    if ( ret == NULL ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif

/**
 * bool GEOSGeometry::disjoint(GEOSGeometry)
 */
PHP_METHOD(Geometry, disjoint)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSDisjoint_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::touches(GEOSGeometry)
 */
PHP_METHOD(Geometry, touches)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSTouches_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::intersects(GEOSGeometry)
 */
PHP_METHOD(Geometry, intersects)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSIntersects_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::crosses(GEOSGeometry)
 */
PHP_METHOD(Geometry, crosses)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSCrosses_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::within(GEOSGeometry)
 */
PHP_METHOD(Geometry, within)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSWithin_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::contains(GEOSGeometry)
 */
PHP_METHOD(Geometry, contains)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSContains_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::overlaps(GEOSGeometry)
 */
PHP_METHOD(Geometry, overlaps)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSOverlaps_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::covers(GEOSGeometry)
 */
#ifdef HAVE_GEOS_COVERS
PHP_METHOD(Geometry, covers)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSCovers_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}
#endif

/**
 * bool GEOSGeometry::coveredBy(GEOSGeometry)
 */
#ifdef HAVE_GEOS_COVERED_BY
PHP_METHOD(Geometry, coveredBy)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
            == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSCoveredBy_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}
#endif

/**
 * bool GEOSGeometry::equals(GEOSGeometry)
 */
PHP_METHOD(Geometry, equals)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o",
        &zobj) == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSEquals_r(GEOS_G(handle), this, other);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::equalsExact(GEOSGeometry)
 * bool GEOSGeometry::equalsExact(GEOSGeometry, double tolerance)
 */
PHP_METHOD(Geometry, equalsExact)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    int ret;
    double tolerance = 0;
    zend_bool retBool;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o|d",
        &zobj, &tolerance) == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSEqualsExact_r(GEOS_G(handle), this, other, tolerance);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::isEmpty()
 */
PHP_METHOD(Geometry, isEmpty)
{
    GEOSGeometry *this;
    int ret;
    zend_bool retBool;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSisEmpty_r(GEOS_G(handle), this);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * array GEOSGeometry::checkValidity()
 */
#ifdef HAVE_GEOS_IS_VALID_DETAIL
PHP_METHOD(Geometry, checkValidity)
{
    GEOSGeometry *this;
    GEOSGeometry *location = NULL;
    int ret;
    char *reason = NULL;
    zend_bool retBool;
    char *reasonVal = NULL;
    zval *locationVal = NULL;
    long int flags = 0;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|l",
        &flags) == FAILURE) {
        RETURN_NULL();
    }

    ret = GEOSisValidDetail_r(GEOS_G(handle), this, flags, &reason, &location);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    if ( reason ) {
        reasonVal = estrdup(reason);
        GEOSFree_r(GEOS_G(handle), reason);
    }

    if ( location ) {
        MAKE_STD_ZVAL(locationVal);
        object_init_ex(locationVal, Geometry_ce_ptr);
        setRelay(locationVal, location);
    }

    retBool = ret;

    /* return value is an array */
    array_init(return_value);
    add_assoc_bool(return_value, "valid", retBool);
    if ( reasonVal ) GEOS_PHP_ADD_ASSOC_ARRAY(return_value, "reason", reasonVal);
    if ( locationVal ) GEOS_PHP_ADD_ASSOC_ZVAL(return_value, "location", locationVal);

}
#endif

/**
 * bool GEOSGeometry::isSimple()
 */
PHP_METHOD(Geometry, isSimple)
{
    GEOSGeometry *this;
    int ret;
    zend_bool retBool;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSisSimple_r(GEOS_G(handle), this);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::isRing()
 */
PHP_METHOD(Geometry, isRing)
{
    GEOSGeometry *this;
    int ret;
    zend_bool retBool;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSisRing_r(GEOS_G(handle), this);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::hasZ()
 */
PHP_METHOD(Geometry, hasZ)
{
    GEOSGeometry *this;
    int ret;
    zend_bool retBool;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSHasZ_r(GEOS_G(handle), this);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}

/**
 * bool GEOSGeometry::isClosed()
 */
#ifdef HAVE_GEOS_IS_CLOSED
PHP_METHOD(Geometry, isClosed)
{
    GEOSGeometry *this;
    int ret;
    zend_bool retBool;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSisClosed_r(GEOS_G(handle), this);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}
#endif

/**
 * string GEOSGeometry::typeName()
 */
PHP_METHOD(Geometry, typeName)
{
    GEOSGeometry *this;
    char *typ;
    char *typVal;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    /* TODO: define constant strings instead... */

    typ = GEOSGeomType_r(GEOS_G(handle), this);
    if ( ! typ ) RETURN_NULL(); /* should get an exception first */

    typVal = estrdup(typ);
    GEOSFree_r(GEOS_G(handle), typ);

    GEOS_PHP_RETURN_STRING(typVal);
}

/**
 * long GEOSGeometry::typeId()
 */
PHP_METHOD(Geometry, typeId)
{
    GEOSGeometry *this;
    long typ;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    /* TODO: define constant strings instead... */

    typ = GEOSGeomTypeId_r(GEOS_G(handle), this);
    if ( typ == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_LONG(typ);
}

/**
 * long GEOSGeometry::getSRID()
 */
PHP_METHOD(Geometry, getSRID)
{
    GEOSGeometry *geom;
    long int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGetSRID_r(GEOS_G(handle), geom);

    RETURN_LONG(ret);
}

/**
 * void GEOSGeometry::setSRID(long)
 */
PHP_METHOD(Geometry, setSRID)
{
    GEOSGeometry *geom;
    long int srid;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l",
        &srid) == FAILURE) {
        RETURN_NULL();
    }

    GEOSSetSRID_r(GEOS_G(handle), geom, srid);
}

/**
 * long GEOSGeometry::numGeometries()
 */
PHP_METHOD(Geometry, numGeometries)
{
    GEOSGeometry *geom;
    long int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGetNumGeometries_r(GEOS_G(handle), geom);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_LONG(ret);
}

/**
 * GEOSGeometry GEOSGeometry::geometryN()
 */
PHP_METHOD(Geometry, geometryN)
{
    GEOSGeometry *geom;
    const GEOSGeometry *c;
    GEOSGeometry *cc;
    long int num;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l",
        &num) == FAILURE) {
        RETURN_NULL();
    }

    if ( num >= GEOSGetNumGeometries_r(GEOS_G(handle), geom) ) RETURN_NULL();
    c = GEOSGetGeometryN_r(GEOS_G(handle), geom, num);
    if ( ! c ) RETURN_NULL(); /* should get an exception first */
    cc = GEOSGeom_clone_r(GEOS_G(handle), c);
    if ( ! cc ) RETURN_NULL(); /* should get an exception first */

    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, cc);
}

/**
 * long GEOSGeometry::numInteriorRings()
 */
PHP_METHOD(Geometry, numInteriorRings)
{
    GEOSGeometry *geom;
    long int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGetNumInteriorRings_r(GEOS_G(handle), geom);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_LONG(ret);
}

/**
 * long GEOSGeometry::numPoints()
 */
#ifdef HAVE_GEOS_GEOM_GET_NUM_POINTS
PHP_METHOD(Geometry, numPoints)
{
    GEOSGeometry *geom;
    long int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGeomGetNumPoints_r(GEOS_G(handle), geom);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_LONG(ret);
}
#endif

/**
 * double GEOSGeometry::getX()
 */
#ifdef HAVE_GEOS_GEOM_GET_X
PHP_METHOD(Geometry, getX)
{
    GEOSGeometry *geom;
    int ret;
    double x;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGeomGetX_r(GEOS_G(handle), geom, &x);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(x);
}
#endif

/**
 * double GEOSGeometry::getY()
 */
#ifdef HAVE_GEOS_GEOM_GET_Y
PHP_METHOD(Geometry, getY)
{
    GEOSGeometry *geom;
    int ret;
    double y;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGeomGetY_r(GEOS_G(handle), geom, &y);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(y);
}
#endif

/**
 * GEOSGeometry GEOSGeometry::interiorRingN()
 */
PHP_METHOD(Geometry, interiorRingN)
{
    GEOSGeometry *geom;
    const GEOSGeometry *c;
    GEOSGeometry *cc;
    long int num;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l",
        &num) == FAILURE) {
        RETURN_NULL();
    }

    if ( num >= GEOSGetNumInteriorRings_r(GEOS_G(handle), geom) ) RETURN_NULL();
    c = GEOSGetInteriorRingN_r(GEOS_G(handle), geom, num);
    if ( ! c ) RETURN_NULL(); /* should get an exception first */
    cc = GEOSGeom_clone_r(GEOS_G(handle), c);
    if ( ! cc ) RETURN_NULL(); /* should get an exception first */

    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, cc);
}

/**
 * GEOSGeometry GEOSGeometry::exteriorRing()
 */
PHP_METHOD(Geometry, exteriorRing)
{
    GEOSGeometry *geom;
    const GEOSGeometry *c;
    GEOSGeometry *cc;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    c = GEOSGetExteriorRing_r(GEOS_G(handle), geom);
    if ( ! c ) RETURN_NULL(); /* should get an exception first */
    cc = GEOSGeom_clone_r(GEOS_G(handle), c);
    if ( ! cc ) RETURN_NULL(); /* should get an exception first */

    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, cc);
}

/**
 * long GEOSGeometry::numCoordinates()
 */
PHP_METHOD(Geometry, numCoordinates)
{
    GEOSGeometry *geom;
    long int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGetNumCoordinates_r(GEOS_G(handle), geom);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_LONG(ret);
}

/**
 * long GEOSGeometry::dimension()
 * 0:puntual 1:lineal 2:areal
 */
PHP_METHOD(Geometry, dimension)
{
    GEOSGeometry *geom;
    long int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGeom_getDimensions_r(GEOS_G(handle), geom);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_LONG(ret);
}

/**
 * long GEOSGeometry::coordinateDimension()
 */
#ifdef HAVE_GEOS_GEOM_GET_COORDINATE_DIMENSION
PHP_METHOD(Geometry, coordinateDimension)
{
    GEOSGeometry *geom;
    long int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSGeom_getCoordinateDimension_r(GEOS_G(handle), geom);
    if ( ret == -1 ) RETURN_NULL(); /* should get an exception first */

    RETURN_LONG(ret);
}
#endif

/**
 * GEOSGeometry GEOSGeometry::pointN()
 */
#ifdef HAVE_GEOS_GEOM_GET_POINT_N
PHP_METHOD(Geometry, pointN)
{
    GEOSGeometry *geom;
    GEOSGeometry *c;
    long int num;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l",
        &num) == FAILURE) {
        RETURN_NULL();
    }

    if ( num >= GEOSGeomGetNumPoints_r(GEOS_G(handle), geom) ) RETURN_NULL();
    c = GEOSGeomGetPointN_r(GEOS_G(handle), geom, num);
    if ( ! c ) RETURN_NULL(); /* should get an exception first */

    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, c);
}
#endif

/**
 * GEOSGeometry GEOSGeometry::startPoint()
 */
PHP_METHOD(Geometry, startPoint)
{
    GEOSGeometry *geom;
    GEOSGeometry *c;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    c = GEOSGeomGetStartPoint_r(GEOS_G(handle), geom);
    if ( ! c ) RETURN_NULL(); /* should get an exception first */

    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, c);
}

/**
 * GEOSGeometry GEOSGeometry::endPoint()
 */
PHP_METHOD(Geometry, endPoint)
{
    GEOSGeometry *geom;
    GEOSGeometry *c;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    c = GEOSGeomGetEndPoint_r(GEOS_G(handle), geom);
    if ( ! c ) RETURN_NULL(); /* should get an exception first */

    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, c);
}

/**
 * double GEOSGeometry::area()
 */
PHP_METHOD(Geometry, area)
{
    GEOSGeometry *geom;
    double area;
    int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSArea_r(GEOS_G(handle), geom, &area);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(area);
}

/**
 * double GEOSGeometry::length()
 */
PHP_METHOD(Geometry, length)
{
    GEOSGeometry *geom;
    double length;
    int ret;

    geom = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSLength_r(GEOS_G(handle), geom, &length);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(length);
}

/**
 * double GEOSGeometry::distance(GEOSGeometry)
 */
PHP_METHOD(Geometry, distance)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    zval *zobj;
    double dist;
    int ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o",
        &zobj) == FAILURE)
    {
        RETURN_NULL();
    }

    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSDistance_r(GEOS_G(handle), this, other, &dist);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(dist);
}

/**
 * double GEOSGeometry::hausdorffDistance(GEOSGeometry)
 */
PHP_METHOD(Geometry, hausdorffDistance)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    zval *zobj;
    double dist;
    int ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o",
        &zobj) == FAILURE)
    {
        RETURN_NULL();
    }

    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSHausdorffDistance_r(GEOS_G(handle), this, other, &dist);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    RETURN_DOUBLE(dist);
}

#ifdef HAVE_GEOS_SNAP
PHP_METHOD(Geometry, snapTo)
{
    GEOSGeometry *this;
    GEOSGeometry *other;
    GEOSGeometry *ret;
    double tolerance;
    zval *zobj;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "od", &zobj,
            &tolerance) == FAILURE) {
        RETURN_NULL();
    }
    other = getRelay(zobj, Geometry_ce_ptr);

    ret = GEOSSnap_r(GEOS_G(handle), this, other, tolerance);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif

#ifdef HAVE_GEOS_NODE
PHP_METHOD(Geometry, node)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    ret = GEOSNode_r(GEOS_G(handle), this);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif



/* -- class GEOSWKTReader -------------------- */

PHP_METHOD(WKTReader, __construct);
PHP_METHOD(WKTReader, read);

static zend_function_entry WKTReader_methods[] = {
    PHP_ME(WKTReader, __construct, arginfo_void, 0)
    PHP_ME(WKTReader, read, arginfo_void, 0)
    {NULL, NULL, NULL}
};

static zend_class_entry *WKTReader_ce_ptr;

static zend_object_handlers WKTReader_object_handlers;

static void
WKTReader_dtor (GEOS_PHP_DTOR_OBJECT *object)
{
#if PHP_VERSION_ID < 70000
    Proxy *obj = (Proxy *)object;
#else
    Proxy *obj = php_geos_fetch_object(object);
#endif

    GEOSWKTReader *reader = (GEOSWKTReader*)obj->relay;
    if (reader) {
        GEOSWKTReader_destroy_r(GEOS_G(handle), reader);
    }

#if PHP_VERSION_ID < 70000
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);

    efree(obj);
#endif
}

static zend_object_value
WKTReader_create_obj (zend_class_entry *type)
{
    return Gen_create_obj(type, WKTReader_dtor, &WKTReader_object_handlers);
}


PHP_METHOD(WKTReader, __construct)
{
    GEOSWKTReader* obj;
    zval *object = getThis();

    obj = GEOSWKTReader_create_r(GEOS_G(handle));
    if ( ! obj ) {
        php_error_docref(NULL, E_ERROR,
                "GEOSWKTReader_create() failed (didn't initGEOS?)");
    }

    setRelay(object, obj);
}

PHP_METHOD(WKTReader, read)
{
    GEOSWKTReader *reader;
    GEOSGeometry *geom;
    zend_string *wkt;
#if PHP_VERSION_ID < 70000
    int wktlen;
#endif

    reader = (GEOSWKTReader*)getRelay(getThis(), WKTReader_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(),
#if PHP_VERSION_ID >= 70000
            "S", &wkt
#else
            "s", &wkt, &wktlen
#endif
       ) == FAILURE)
    {
        RETURN_NULL();
    }

    geom = GEOSWKTReader_read_r(GEOS_G(handle), reader, ZSTR_VAL(wkt));
    /* we'll probably get an exception if geom is null */
    if ( ! geom ) RETURN_NULL();

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, geom);

}

/* -- class GEOSWKTWriter -------------------- */

PHP_METHOD(WKTWriter, __construct);
PHP_METHOD(WKTWriter, write);

#ifdef HAVE_GEOS_WKT_WRITER_SET_TRIM
PHP_METHOD(WKTWriter, setTrim);
#endif

#ifdef HAVE_GEOS_WKT_WRITER_SET_ROUNDING_PRECISION
PHP_METHOD(WKTWriter, setRoundingPrecision);
#endif

#ifdef HAVE_GEOS_WKT_WRITER_SET_OUTPUT_DIMENSION
PHP_METHOD(WKTWriter, setOutputDimension);
#endif

#ifdef HAVE_GEOS_WKT_WRITER_GET_OUTPUT_DIMENSION
PHP_METHOD(WKTWriter, getOutputDimension);
#endif

#ifdef HAVE_GEOS_WKT_WRITER_SET_OLD_3D
PHP_METHOD(WKTWriter, setOld3D);
#endif

static zend_function_entry WKTWriter_methods[] = {
    PHP_ME(WKTWriter, __construct, arginfo_void, 0)
    PHP_ME(WKTWriter, write, arginfo_void, 0)

#   ifdef HAVE_GEOS_WKT_WRITER_SET_TRIM
    PHP_ME(WKTWriter, setTrim, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_WKT_WRITER_SET_ROUNDING_PRECISION
    PHP_ME(WKTWriter, setRoundingPrecision, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_WKT_WRITER_SET_OUTPUT_DIMENSION
    PHP_ME(WKTWriter, setOutputDimension, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_WKT_WRITER_GET_OUTPUT_DIMENSION
    PHP_ME(WKTWriter, getOutputDimension, arginfo_void, 0)
#   endif

#   ifdef HAVE_GEOS_WKT_WRITER_SET_OLD_3D
    PHP_ME(WKTWriter, setOld3D, arginfo_void, 0)
#   endif

    {NULL, NULL, NULL}
};

static zend_class_entry *WKTWriter_ce_ptr;

static zend_object_handlers WKTWriter_object_handlers;

static void
WKTWriter_dtor (GEOS_PHP_DTOR_OBJECT *object)
{
#if PHP_VERSION_ID < 70000
    Proxy *obj = (Proxy *)object;
#else
    Proxy *obj = php_geos_fetch_object(object);
#endif

    GEOSWKTWriter_destroy_r(GEOS_G(handle), (GEOSWKTWriter*)obj->relay);

#if PHP_VERSION_ID >= 70000
    //zend_object_std_dtor(&obj->std);
#else
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);

    efree(obj);
#endif
}

static zend_object_value
WKTWriter_create_obj (zend_class_entry *type)
{
    return Gen_create_obj(type, WKTWriter_dtor, &WKTWriter_object_handlers);
}

PHP_METHOD(WKTWriter, __construct)
{
    GEOSWKTWriter* obj;
    zval *object = getThis();

    obj = GEOSWKTWriter_create_r(GEOS_G(handle));
    if ( ! obj ) {
        php_error_docref(NULL, E_ERROR,
                "GEOSWKTWriter_create() failed (didn't initGEOS?)");
    }

    setRelay(object, obj);
}

PHP_METHOD(WKTWriter, write)
{
    GEOSWKTWriter *writer;
    zval *zobj;
    GEOSGeometry *geom;
    char* wkt;
    char* retstr;

    writer = (GEOSWKTWriter*)getRelay(getThis(), WKTWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
        == FAILURE)
    {
        RETURN_NULL();
    }

    geom = getRelay(zobj, Geometry_ce_ptr);

    wkt = GEOSWKTWriter_write_r(GEOS_G(handle), writer, geom);
    /* we'll probably get an exception if wkt is null */
    if ( ! wkt ) RETURN_NULL();

    retstr = estrdup(wkt);
    GEOSFree_r(GEOS_G(handle), wkt);

    GEOS_PHP_RETURN_STRING(retstr);
}

#ifdef HAVE_GEOS_WKT_WRITER_SET_TRIM
PHP_METHOD(WKTWriter, setTrim)
{
    GEOSWKTWriter *writer;
    zend_bool trimval;
    char trim;

    writer = (GEOSWKTWriter*)getRelay(getThis(), WKTWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "b", &trimval)
        == FAILURE)
    {
        RETURN_NULL();
    }

    trim = trimval;
    GEOSWKTWriter_setTrim_r(GEOS_G(handle), writer, trim);
}
#endif

#ifdef HAVE_GEOS_WKT_WRITER_SET_ROUNDING_PRECISION
PHP_METHOD(WKTWriter, setRoundingPrecision)
{
    GEOSWKTWriter *writer;
    long int prec;

    writer = (GEOSWKTWriter*)getRelay(getThis(), WKTWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &prec)
        == FAILURE)
    {
        RETURN_NULL();
    }

    GEOSWKTWriter_setRoundingPrecision_r(GEOS_G(handle), writer, prec);
}
#endif

/**
 * void GEOSWKTWriter::setOutputDimension()
 */
#ifdef HAVE_GEOS_WKT_WRITER_SET_OUTPUT_DIMENSION
PHP_METHOD(WKTWriter, setOutputDimension)
{
    GEOSWKTWriter *writer;
    long int dim;

    writer = (GEOSWKTWriter*)getRelay(getThis(), WKTWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &dim)
        == FAILURE)
    {
        RETURN_NULL();
    }

    GEOSWKTWriter_setOutputDimension_r(GEOS_G(handle), writer, dim);
}
#endif

/**
 * long GEOSWKTWriter::getOutputDimension()
 */
#ifdef HAVE_GEOS_WKT_WRITER_GET_OUTPUT_DIMENSION
PHP_METHOD(WKTWriter, getOutputDimension)
{
    GEOSWKTWriter *writer;
    long int ret;

    writer = (GEOSWKTWriter*)getRelay(getThis(), WKTWriter_ce_ptr);

    ret = GEOSWKTWriter_getOutputDimension_r(GEOS_G(handle), writer);

    RETURN_LONG(ret);
}
#endif

#ifdef HAVE_GEOS_WKT_WRITER_SET_OLD_3D
PHP_METHOD(WKTWriter, setOld3D)
{
    GEOSWKTWriter *writer;
    zend_bool bval;
    int val;

    writer = (GEOSWKTWriter*)getRelay(getThis(), WKTWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "b", &bval)
        == FAILURE)
    {
        RETURN_NULL();
    }

    val = bval;
    GEOSWKTWriter_setOld3D_r(GEOS_G(handle), writer, val);
}
#endif

/* -- class GEOSWKBWriter -------------------- */

PHP_METHOD(WKBWriter, __construct);
PHP_METHOD(WKBWriter, getOutputDimension);
PHP_METHOD(WKBWriter, setOutputDimension);
PHP_METHOD(WKBWriter, getByteOrder);
PHP_METHOD(WKBWriter, setByteOrder);
PHP_METHOD(WKBWriter, setIncludeSRID);
PHP_METHOD(WKBWriter, getIncludeSRID);
PHP_METHOD(WKBWriter, write);
PHP_METHOD(WKBWriter, writeHEX);

static zend_function_entry WKBWriter_methods[] = {
    PHP_ME(WKBWriter, __construct, arginfo_void, 0)
    PHP_ME(WKBWriter, getOutputDimension, arginfo_void, 0)
    PHP_ME(WKBWriter, setOutputDimension, arginfo_void, 0)
    PHP_ME(WKBWriter, getByteOrder, arginfo_void, 0)
    PHP_ME(WKBWriter, setByteOrder, arginfo_void, 0)
    PHP_ME(WKBWriter, getIncludeSRID, arginfo_void, 0)
    PHP_ME(WKBWriter, setIncludeSRID, arginfo_void, 0)
    PHP_ME(WKBWriter, write, arginfo_void, 0)
    PHP_ME(WKBWriter, writeHEX, arginfo_void, 0)
    {NULL, NULL, NULL}
};

static zend_class_entry *WKBWriter_ce_ptr;

static zend_object_handlers WKBWriter_object_handlers;

static void
WKBWriter_dtor (GEOS_PHP_DTOR_OBJECT *object)
{
#if PHP_VERSION_ID < 70000
    Proxy *obj = (Proxy *)object;
#else
    Proxy *obj = php_geos_fetch_object(object);
#endif

    GEOSWKBWriter_destroy_r(GEOS_G(handle), (GEOSWKBWriter*)obj->relay);

#if PHP_VERSION_ID >= 70000
    //zend_object_std_dtor(&obj->std);
#else
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);

    efree(obj);
#endif
}

static zend_object_value
WKBWriter_create_obj (zend_class_entry *type)
{
    return Gen_create_obj(type, WKBWriter_dtor, &WKBWriter_object_handlers);
}

/**
 * GEOSWKBWriter w = new GEOSWKBWriter()
 */
PHP_METHOD(WKBWriter, __construct)
{
    GEOSWKBWriter* obj;
    zval *object = getThis();

    obj = GEOSWKBWriter_create_r(GEOS_G(handle));
    if ( ! obj ) {
        php_error_docref(NULL, E_ERROR,
                "GEOSWKBWriter_create() failed (didn't initGEOS?)");
    }

    setRelay(object, obj);
}

/**
 * long GEOSWKBWriter::getOutputDimension();
 */
PHP_METHOD(WKBWriter, getOutputDimension)
{
    GEOSWKBWriter *writer;
    long int ret;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    ret = GEOSWKBWriter_getOutputDimension_r(GEOS_G(handle), writer);

    RETURN_LONG(ret);
}

/**
 * void GEOSWKBWriter::setOutputDimension(dims);
 */
PHP_METHOD(WKBWriter, setOutputDimension)
{
    GEOSWKBWriter *writer;
    long int dim;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &dim)
        == FAILURE)
    {
        RETURN_NULL();
    }

    GEOSWKBWriter_setOutputDimension_r(GEOS_G(handle), writer, dim);

}

/**
 * string GEOSWKBWriter::write(GEOSGeometry)
 */
PHP_METHOD(WKBWriter, write)
{
    GEOSWKBWriter *writer;
    zval *zobj;
    GEOSGeometry *geom;
    char *ret;
    size_t retsize;
    char* retstr;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
        == FAILURE)
    {
        RETURN_NULL();
    }

    geom = getRelay(zobj, Geometry_ce_ptr);

    ret = (char*)GEOSWKBWriter_write_r(GEOS_G(handle), writer, geom, &retsize);
    /* we'll probably get an exception if ret is null */
    if ( ! ret ) RETURN_NULL();

    retstr = estrndup(ret, retsize);
    GEOSFree_r(GEOS_G(handle), ret);

    GEOS_PHP_RETURN_STRINGL(retstr, retsize);
}

/**
 * string GEOSWKBWriter::writeHEX(GEOSGeometry)
 */
PHP_METHOD(WKBWriter, writeHEX)
{
    GEOSWKBWriter *writer;
    zval *zobj;
    GEOSGeometry *geom;
    char *ret;
    size_t retsize; /* useless... */
    char* retstr;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
        == FAILURE)
    {
        RETURN_NULL();
    }

    geom = getRelay(zobj, Geometry_ce_ptr);

    ret = (char*)GEOSWKBWriter_writeHEX_r(GEOS_G(handle), writer, geom, &retsize);
    /* we'll probably get an exception if ret is null */
    if ( ! ret ) RETURN_NULL();

    retstr = estrndup(ret, retsize);
    GEOSFree_r(GEOS_G(handle), ret);

    GEOS_PHP_RETURN_STRING(retstr);
}

/**
 * long GEOSWKBWriter::getByteOrder();
 */
PHP_METHOD(WKBWriter, getByteOrder)
{
    GEOSWKBWriter *writer;
    long int ret;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    ret = GEOSWKBWriter_getByteOrder_r(GEOS_G(handle), writer);

    RETURN_LONG(ret);
}

/**
 * void GEOSWKBWriter::setByteOrder(dims);
 */
PHP_METHOD(WKBWriter, setByteOrder)
{
    GEOSWKBWriter *writer;
    long int dim;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &dim)
        == FAILURE)
    {
        RETURN_NULL();
    }

    GEOSWKBWriter_setByteOrder_r(GEOS_G(handle), writer, dim);

}

/**
 * bool GEOSWKBWriter::getIncludeSRID();
 */
PHP_METHOD(WKBWriter, getIncludeSRID)
{
    GEOSWKBWriter *writer;
    int ret;
    zend_bool retBool;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    ret = GEOSWKBWriter_getIncludeSRID_r(GEOS_G(handle), writer);
    retBool = ret;

    RETURN_BOOL(retBool);
}

/**
 * void GEOSWKBWriter::setIncludeSRID(bool);
 */
PHP_METHOD(WKBWriter, setIncludeSRID)
{
    GEOSWKBWriter *writer;
    int inc;
    zend_bool incVal;

    writer = (GEOSWKBWriter*)getRelay(getThis(), WKBWriter_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "b", &incVal)
        == FAILURE)
    {
        RETURN_NULL();
    }

    inc = incVal;
    GEOSWKBWriter_setIncludeSRID_r(GEOS_G(handle), writer, inc);
}

/* -- class GEOSWKBReader -------------------- */

PHP_METHOD(WKBReader, __construct);
PHP_METHOD(WKBReader, read);
PHP_METHOD(WKBReader, readHEX);

static zend_function_entry WKBReader_methods[] = {
    PHP_ME(WKBReader, __construct, arginfo_void, 0)
    PHP_ME(WKBReader, read, arginfo_void, 0)
    PHP_ME(WKBReader, readHEX, arginfo_void, 0)
    {NULL, NULL, NULL}
};

static zend_class_entry *WKBReader_ce_ptr;

static zend_object_handlers WKBReader_object_handlers;

static void
WKBReader_dtor (GEOS_PHP_DTOR_OBJECT *object)
{
#if PHP_VERSION_ID < 70000
    Proxy *obj = (Proxy *)object;
#else
    Proxy *obj = php_geos_fetch_object(object);
#endif

    GEOSWKBReader_destroy_r(GEOS_G(handle), (GEOSWKBReader*)obj->relay);

#if PHP_VERSION_ID >= 70000
    //zend_object_std_dtor(&obj->std);
#else
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);

    efree(obj);
#endif
}

static zend_object_value
WKBReader_create_obj (zend_class_entry *type)
{
    return Gen_create_obj(type, WKBReader_dtor, &WKBReader_object_handlers);
}


PHP_METHOD(WKBReader, __construct)
{
    GEOSWKBReader* obj;
    zval *object = getThis();

    obj = GEOSWKBReader_create_r(GEOS_G(handle));
    if ( ! obj ) {
        php_error_docref(NULL, E_ERROR,
                "GEOSWKBReader_create() failed (didn't initGEOS?)");
    }

    setRelay(object, obj);
}

PHP_METHOD(WKBReader, read)
{
    GEOSWKBReader *reader;
    GEOSGeometry *geom;
    zend_string* wkb;
    int wkblen;

    reader = (GEOSWKBReader*)getRelay(getThis(), WKBReader_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(),
#if PHP_VERSION_ID >= 70000
            "S", &wkb
#else
            "s", &wkb, &wkblen
#endif
       ) == FAILURE)
    {
        RETURN_NULL();
    }

#if PHP_VERSION_ID >= 70000
    wkblen = ZSTR_LEN(wkb);
#endif

    geom = GEOSWKBReader_read_r(GEOS_G(handle), reader, (unsigned char*)ZSTR_VAL(wkb), wkblen);
    /* we'll probably get an exception if geom is null */
    if ( ! geom ) RETURN_NULL();

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, geom);

}

PHP_METHOD(WKBReader, readHEX)
{
    GEOSWKBReader *reader;
    GEOSGeometry *geom;
    unsigned char* wkb;
#if PHP_VERSION_ID >= 70000
    size_t wkblen;
#else
    int wkblen;
#endif

    reader = (GEOSWKBReader*)getRelay(getThis(), WKBReader_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s",
        &wkb, &wkblen) == FAILURE)
    {
        RETURN_NULL();
    }

    geom = GEOSWKBReader_readHEX_r(GEOS_G(handle), reader, wkb, wkblen);
    /* we'll probably get an exception if geom is null */
    if ( ! geom ) RETURN_NULL();

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, geom);

}


/* -- Free functions ------------------------- */

/**
 * string GEOSVersion()
 */
PHP_FUNCTION(GEOSVersion)
{
    char *str;

    str = estrdup(GEOSversion());
    GEOS_PHP_RETURN_STRING(str);
}

/**
 * array GEOSPolygonize(GEOSGeometry $geom)
 *
 * The returned array contains the following elements:
 *
 *  - 'rings'
 *      Type: array of GEOSGeometry
 *      Rings that can be formed by the costituent
 *      linework of geometry.
 *  - 'cut_edges' (optional)
 *      Type: array of GEOSGeometry
 *      Edges which are connected at both ends but
 *      which do not form part of polygon.
 *  - 'dangles'
 *      Type: array of GEOSGeometry
 *      Edges which have one or both ends which are
 *      not incident on another edge endpoint
 *  - 'invalid_rings'
 *      Type: array of GEOSGeometry
 *      Edges which form rings which are invalid
 *      (e.g. the component lines contain a self-intersection)
 *
 */
PHP_FUNCTION(GEOSPolygonize)
{
    GEOSGeometry *this;
    GEOSGeometry *rings;
    GEOSGeometry *cut_edges;
    GEOSGeometry *dangles;
    GEOSGeometry *invalid_rings;
    zval *array_elem;
    zval *zobj;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
        == FAILURE)
    {
        RETURN_NULL();
    }
    this = getRelay(zobj, Geometry_ce_ptr);

    rings = GEOSPolygonize_full_r(GEOS_G(handle), this, &cut_edges, &dangles, &invalid_rings);
    if ( ! rings ) RETURN_NULL(); /* should get an exception first */

    /* return value should be an array */
    array_init(return_value);

    MAKE_STD_ZVAL(array_elem);
    array_init(array_elem);
    dumpGeometry(rings, array_elem);
    GEOSGeom_destroy_r(GEOS_G(handle), rings);
    GEOS_PHP_ADD_ASSOC_ZVAL(return_value, "rings", array_elem);

    MAKE_STD_ZVAL(array_elem);
    array_init(array_elem);
    dumpGeometry(cut_edges, array_elem);
    GEOSGeom_destroy_r(GEOS_G(handle), cut_edges);
    GEOS_PHP_ADD_ASSOC_ZVAL(return_value, "cut_edges", array_elem);

    MAKE_STD_ZVAL(array_elem);
    array_init(array_elem);
    dumpGeometry(dangles, array_elem);
    GEOSGeom_destroy_r(GEOS_G(handle), dangles);
    GEOS_PHP_ADD_ASSOC_ZVAL(return_value, "dangles", array_elem);

    MAKE_STD_ZVAL(array_elem);
    array_init(array_elem);
    dumpGeometry(invalid_rings, array_elem);
    GEOSGeom_destroy_r(GEOS_G(handle), invalid_rings);
    GEOS_PHP_ADD_ASSOC_ZVAL(return_value, "invalid_rings", array_elem);

}

/**
 * array GEOSLineMerge(GEOSGeometry $geom)
 */
PHP_FUNCTION(GEOSLineMerge)
{
    GEOSGeometry *geom_in;
    GEOSGeometry *geom_out;
    zval *zobj;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &zobj)
        == FAILURE)
    {
        RETURN_NULL();
    }
    geom_in = getRelay(zobj, Geometry_ce_ptr);

    geom_out = GEOSLineMerge_r(GEOS_G(handle), geom_in);
    if ( ! geom_out ) RETURN_NULL(); /* should get an exception first */

    /* return value should be an array */
    array_init(return_value);
    dumpGeometry(geom_out, return_value);
    GEOSGeom_destroy_r(GEOS_G(handle), geom_out);
}

/**
 * GEOSGeometry GEOSSharedPaths(GEOSGeometry $geom1, GEOSGeometry *geom2)
 */
#ifdef HAVE_GEOS_SHARED_PATHS
PHP_FUNCTION(GEOSSharedPaths)
{
    GEOSGeometry *geom_in_1;
    GEOSGeometry *geom_in_2;
    GEOSGeometry *geom_out;
    zval *zobj1, *zobj2;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "oo", &zobj1, &zobj2)
        == FAILURE)
    {
        RETURN_NULL();
    }
    geom_in_1 = getRelay(zobj1, Geometry_ce_ptr);
    geom_in_2 = getRelay(zobj2, Geometry_ce_ptr);

    geom_out = GEOSSharedPaths_r(GEOS_G(handle), geom_in_1, geom_in_2);
    if ( ! geom_out ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, geom_out);
}
#endif

/**
 * GEOSGeometry::delaunayTriangulation([<tolerance>], [<onlyEdges>])
 *
 *  'tolerance'
 *       Type: double
 *       snapping tolerance to use for improved robustness
 *  'onlyEdges'
 *       Type: boolean
 *       if true will return a MULTILINESTRING, otherwise (the default)
 *       it will return a GEOMETRYCOLLECTION containing triangular POLYGONs.
 */
#ifdef HAVE_GEOS_DELAUNAY_TRIANGULATION
PHP_METHOD(Geometry, delaunayTriangulation)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;
    double tolerance = 0.0;
    zend_bool edgeonly = 0;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|db",
            &tolerance, &edgeonly) == FAILURE) {
        RETURN_NULL();
    }

    ret = GEOSDelaunayTriangulation_r(GEOS_G(handle), this, tolerance, edgeonly ? 1 : 0);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif

/**
 * GEOSGeometry::voronoiDiagram([<tolerance>], [<onlyEdges>], [<extent>])
 *
 *  'tolerance'
 *       Type: double
 *       snapping tolerance to use for improved robustness
 *  'onlyEdges'
 *       Type: boolean
 *       if true will return a MULTILINESTRING, otherwise (the default)
 *       it will return a GEOMETRYCOLLECTION containing POLYGONs.
 *  'extent'
 *       Type: geometry
 *       Clip returned diagram by the extent of the given geometry
 */
#ifdef HAVE_GEOS_VORONOI_DIAGRAM
PHP_METHOD(Geometry, voronoiDiagram)
{
    GEOSGeometry *this;
    GEOSGeometry *ret;
    zval *zobj = 0;
    GEOSGeometry *env = 0;
    double tolerance = 0.0;
    zend_bool edgeonly = 0;

    this = (GEOSGeometry*)getRelay(getThis(), Geometry_ce_ptr);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|dbo",
            &tolerance, &edgeonly, &zobj) == FAILURE) {
        RETURN_NULL();
    }

    if ( zobj ) env = getRelay(zobj, Geometry_ce_ptr);
    ret = GEOSVoronoiDiagram_r(GEOS_G(handle), this, env, tolerance, edgeonly ? 1 : 0);
    if ( ! ret ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    object_init_ex(return_value, Geometry_ce_ptr);
    setRelay(return_value, ret);
}
#endif

/**
 * bool GEOSRelateMatch(string matrix, string pattern)
 */
#ifdef HAVE_GEOS_RELATE_PATTERN_MATCH
PHP_FUNCTION(GEOSRelateMatch)
{
    char* mat = NULL;
    char* pat = NULL;
#if PHP_VERSION_ID >= 70000
    size_t matlen;
    size_t patlen;
#else
    int matlen;
    int patlen;
#endif
    int ret;
    zend_bool retBool;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss",
        &mat, &matlen, &pat, &patlen) == FAILURE)
    {
        RETURN_NULL();
    }

    ret = GEOSRelatePatternMatch_r(GEOS_G(handle), mat, pat);
    if ( ret == 2 ) RETURN_NULL(); /* should get an exception first */

    /* return_value is a zval */
    retBool = ret;
    RETURN_BOOL(retBool);
}
#endif

/* ------ Initialization / Deinitialization / Meta ------------------ */

/* per-module initialization */
PHP_MINIT_FUNCTION(geos)
{
    zend_class_entry ce;

    /* WKTReader */
    INIT_CLASS_ENTRY(ce, "GEOSWKTReader", WKTReader_methods);
    WKTReader_ce_ptr = zend_register_internal_class(&ce);
    WKTReader_ce_ptr->create_object = WKTReader_create_obj;
    memcpy(&WKTReader_object_handlers,
        zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    WKTReader_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 70000
    WKTReader_object_handlers.offset = XtOffsetOf(Proxy, std);
    WKTReader_object_handlers.free_obj = WKTReader_dtor;
#endif

    /* WKTWriter */
    INIT_CLASS_ENTRY(ce, "GEOSWKTWriter", WKTWriter_methods);
    WKTWriter_ce_ptr = zend_register_internal_class(&ce);
    WKTWriter_ce_ptr->create_object = WKTWriter_create_obj;
    memcpy(&WKTWriter_object_handlers,
        zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    WKTWriter_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 70000
    WKTWriter_object_handlers.offset = XtOffsetOf(Proxy, std);
    WKTWriter_object_handlers.free_obj = WKTWriter_dtor;
#endif

    /* Geometry */
    INIT_CLASS_ENTRY(ce, "GEOSGeometry", Geometry_methods);
    Geometry_ce_ptr = zend_register_internal_class(&ce);
    Geometry_ce_ptr->create_object = Geometry_create_obj;
    memcpy(&Geometry_object_handlers,
        zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    Geometry_object_handlers.clone_obj = NULL;
    /* Geometry serialization */
    Geometry_ce_ptr->serialize = Geometry_serialize;
    Geometry_ce_ptr->unserialize = Geometry_deserialize;
#if PHP_VERSION_ID >= 70000
    Geometry_object_handlers.offset = XtOffsetOf(Proxy, std);
    Geometry_object_handlers.free_obj = Geometry_dtor;
#endif

    /* WKBWriter */
    INIT_CLASS_ENTRY(ce, "GEOSWKBWriter", WKBWriter_methods);
    WKBWriter_ce_ptr = zend_register_internal_class(&ce);
    WKBWriter_ce_ptr->create_object = WKBWriter_create_obj;
    memcpy(&WKBWriter_object_handlers,
        zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    WKBWriter_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 70000
    WKBWriter_object_handlers.offset = XtOffsetOf(Proxy, std);
    WKBWriter_object_handlers.free_obj = WKBWriter_dtor;
#endif

    /* WKBReader */
    INIT_CLASS_ENTRY(ce, "GEOSWKBReader", WKBReader_methods);
    WKBReader_ce_ptr = zend_register_internal_class(&ce);
    WKBReader_ce_ptr->create_object = WKBReader_create_obj;
    memcpy(&WKBReader_object_handlers,
        zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    WKBReader_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 70000
    WKBReader_object_handlers.offset = XtOffsetOf(Proxy, std);
    WKBReader_object_handlers.free_obj = WKBReader_dtor;
#endif


    /* Constants */
    REGISTER_LONG_CONSTANT("GEOSBUF_CAP_ROUND",  GEOSBUF_CAP_ROUND,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSBUF_CAP_FLAT",   GEOSBUF_CAP_FLAT,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSBUF_CAP_SQUARE", GEOSBUF_CAP_SQUARE,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSBUF_JOIN_ROUND", GEOSBUF_JOIN_ROUND,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSBUF_JOIN_MITRE", GEOSBUF_JOIN_MITRE,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSBUF_JOIN_BEVEL", GEOSBUF_JOIN_BEVEL,
        CONST_CS|CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("GEOS_POINT", GEOS_POINT,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOS_LINESTRING", GEOS_LINESTRING,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOS_LINEARRING", GEOS_LINEARRING,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOS_POLYGON", GEOS_POLYGON,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOS_MULTIPOINT", GEOS_MULTIPOINT,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOS_MULTILINESTRING", GEOS_MULTILINESTRING,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOS_MULTIPOLYGON", GEOS_MULTIPOLYGON,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOS_GEOMETRYCOLLECTION", GEOS_GEOMETRYCOLLECTION,
        CONST_CS|CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("GEOSVALID_ALLOW_SELFTOUCHING_RING_FORMING_HOLE",
        GEOSVALID_ALLOW_SELFTOUCHING_RING_FORMING_HOLE,
        CONST_CS|CONST_PERSISTENT);

#   ifdef HAVE_GEOS_PREC_NO_TOPO
    REGISTER_LONG_CONSTANT("GEOS_PREC_NO_TOPO", GEOS_PREC_NO_TOPO,
        CONST_CS|CONST_PERSISTENT);
#   endif

#   ifdef HAVE_GEOS_PREC_KEEP_COLLAPSED
    REGISTER_LONG_CONSTANT("GEOS_PREC_KEEP_COLLAPSED", GEOS_PREC_KEEP_COLLAPSED,
        CONST_CS|CONST_PERSISTENT);
#   endif

    REGISTER_LONG_CONSTANT("GEOSRELATE_BNR_MOD2", GEOSRELATE_BNR_MOD2,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSRELATE_BNR_OGC", GEOSRELATE_BNR_OGC,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSRELATE_BNR_ENDPOINT", GEOSRELATE_BNR_ENDPOINT,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSRELATE_BNR_MULTIVALENT_ENDPOINT",
        GEOSRELATE_BNR_MULTIVALENT_ENDPOINT,
        CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("GEOSRELATE_BNR_MONOVALENT_ENDPOINT",
        GEOSRELATE_BNR_MONOVALENT_ENDPOINT,
        CONST_CS|CONST_PERSISTENT);

    return SUCCESS;
}

/* per-module shutdown */
PHP_MSHUTDOWN_FUNCTION(geos)
{
    delGeometrySerializer();
    delGeometryDeserializer();
    return SUCCESS;
}

/* per-request initialization */
PHP_RINIT_FUNCTION(geos)
{
    GEOS_G(handle) = initGEOS_r(noticeHandler, errorHandler);
    return SUCCESS;
}

/* pre-request destruction */
PHP_RSHUTDOWN_FUNCTION(geos)
{
    finishGEOS_r(GEOS_G(handle));
    return SUCCESS;
}

/* global initialization */
PHP_GINIT_FUNCTION(geos)
{
    geos_globals->handle = NULL;
}

/* module info */
PHP_MINFO_FUNCTION(geos)
{
    php_info_print_table_start();
    php_info_print_table_row(2,
        "GEOS - Geometry Engine Open Source", "enabled");
    php_info_print_table_row(2,
        "Version", PHP_GEOS_VERSION);
    php_info_print_table_row(2,
        "GEOS Version", GEOSversion());
    php_info_print_table_end();
}
