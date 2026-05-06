PHP module for GEOS
===================

[![status-badge](https://woodie.osgeo.org/api/badges/132/status.svg)](https://woodie.osgeo.org/repos/132)

The code in this directory provides a PHP module to make use
of functionalities of the [GEOS library] (http://geos.osgeo.org).

The bindings are linked to the C-API, for betters stability.

# Building

## Requirements

You need:

  - PHP development files
    ( often found in packages named something like 'php5-dev' )

  - GEOS development files
    ( often found in packages named something like 'libgeos-dev' )

## Procedure

    git clone https://git.osgeo.org/gitea/geos/php-geos.git
    cd php-geos
    ./autogen.sh
    ./configure
    make # generates modules/geos.so

# Testing

Automated testing is executed on 'make check'.

You'll need phpunit installed for this to work. To install:

    pear install --force --alldeps phpunit/phpunit

# Installing

As root (or owner with write access to the directory
returned by php-config --extension-dir), run:

    make install

# Using

 ... TODO ...
 (reference some automatically built doc?)

