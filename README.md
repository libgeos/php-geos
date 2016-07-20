PHP module for GEOS
===================

[![build status]
(https://drone.osgeo.kbt.io/api/badges/geos/php-geos/status.svg?branch=master)]
(https://drone.osgeo.kbt.io/geos/php-geos?branch=master, alt=build status)

The code in this directory provides a PHP module to make use
of functionalities of the [GEOS library] (http://geos.osgeo.org).

The bindings are linked to the C-API, for betters stability.

# Building

## Requirements

You need:

  - PHP development files
    often found in packages named something like 'php5-dev'

  - GEOS development files
    often found in packages named something like 'libgeos-dev'

# Testing

Automated testing should be executed on 'make check'
togheter with the rest of the GEOS testsuite.
You can still run 'make check' from this dir for
localized (php-only) testing.

Note that you'll need phpunit installed for this to work.
To install:

  sudo pear install --force --alldeps phpunit/phpunit

# Installing

As root (or owner with write access to the directory
returned by php-config --extension-dir),
run 'make install'

# Using

 ... TODO ...
 (reference some automatically built doc?)

