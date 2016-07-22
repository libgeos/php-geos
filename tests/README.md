Tests can be run manually from the top-level build dir using:

    php -n -d extension=modules/geos.so tests/001_Geometry.phpt

If you want to use valgrind, it is recommended to disable Zend
memory management:

    export USE_ZEND_ALLOC=0
