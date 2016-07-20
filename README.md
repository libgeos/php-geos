PHP module for GEOS
===================

[![status](https://drone.osgeo.kbt.io/api/badges/geos/php-geos/status.svg?branch=svn-trunk)]
(https://drone.osgeo.kbt.io/geos/php-geos?branch=svn-trunk, alt=status)

The code in this directory provides a PHP module to make use
of functionalities of the [GEOS library] (http://geos.osgeo.org).

The bindings are linked to the C-API, for betters stability.

# Requirements

You need php5 development tools, often found in
packages named something like 'php5-dev'. 

# Building

Just add --enable-php to your GEOS configure line
and run 'make'.

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

