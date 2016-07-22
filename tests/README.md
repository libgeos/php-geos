Tests can be run manually from the top-level build dir using:

    php -n -d extension=modules/geos.so tests/001_Geometry.phpt

If you want to use valgrind, it is recommended to disable Zend
memory management:

    export USE_ZEND_ALLOC=0

And avoid unload of modules:

    export ZEND_DONT_UNLOAD_MODULES=1

Read more on https://bugs.php.net/bugs-getting-valgrind-log.php
