# PHP module for GEOS

[![build status](https://dronie.osgeo.org/api/badges/geos/php-geos/status.svg?branch=master)](https://github.com/libgeos/php-geos/tree/master)

The code in this directory provides a PHP module to make use
of functionalities of the [GEOS library](http://geos.osgeo.org).

The bindings are linked to the C-API, for betters stability.

## Building (UNIX)

### Requirements

You need:

- PHP development files
  ( often found in packages named something like 'php5-dev' )

- GEOS development files
  ( often found in packages named something like 'libgeos-dev' )

### Procedure

    git clone https://github.com/libgeos/php-geos.git
    cd php-geos
    ./autogen.sh
    ./configure
    make # generates modules/geos.so

## Building (Windows)

### Requirements

- PHP source files:  download [zip](http://windows.php.net/download/) or clone from [github](https://github.com/php/php-src)
- PHP SDK ( https://github.com/Microsoft/php-sdk-binary-tools )
- Visual C++:
  * Visual C++ 14.0 (Visual Studio 2015) for PHP 7.0 or PHP 7.1.
  * Visual C++ 15.0 (Visual Studio 2017) for PHP 7.2, PHP 7.3 or PHP 7.4.
  * Visual C++ 16.0 (Visual Studio 2019) for master. 
- GEOS Windows binaries (binary dll, includes and lib):
    * As part of [OSGeo4W](http://osgeo4w.osgeo.org/) as part of the geos package.
    * As part of [MS4W](https://ms4w.com/)

### Configure

Official instructions for compiling PHP extensions: https://wiki.php.net/internals/windows/stepbystepbuild_sdk_2

1. Install OSGeo4w in `C:\OSGeo4W64` and PHP-SDK in `C:/php-sdk.` 
2. Invoke the starter script, for example for Visual Studio 2015 64-bit build, invoke phpsdk-vc15-x64.bat
3. Run the phpsdk_buildtree batch script which will create the desired directory structure:
   
        phpsdk_buildtree phpdev
   
    The phpsdk_buildtree script will create the path according to the currently VC++ version used and switch into the newly created directory
4. Extract the PHP source code to `C:\php-sdk\phpdev\vX##\x##`, where:

    vX## is the compiler version you are using (eq vc15 or vs16)
    x## is your architecture (x86 or x64)
    For example: `C:\php-sdk\phpdev\vc15\x64\php-7.4.11-src`
    In the same directory where you extracted the PHP source there is a deps directory. 
    For example: `C:\php-sdk\phpdev\vc15\x64\deps`

5. Copy GEOS binaries from `C:\OSGeo4W64` (`bin`, `include` and `lib` subdir) to `C:\php-sdk\phpdev\vc15\x64\deps\`
6. Copy (clone) code from this repository to `C:\php-sdk\phpdev\vc15\x64\php-7.4.11-src\ext\geos`
 
 
### Compile

Run next commands to compile ([see official php instruction](https://wiki.php.net/internals/windows/stepbystepbuild_sdk_2)):
    
    c:\php-sdk\phpsdk-vc15-x64.bat
    cd C:\php-sdk\phpdev\vc15\x64\php-7.4.11-src
    buildconf
    configure --disable-all --enable-cli --with-geos
    nmake
   


## Testing

Automated testing is executed on 'make check'.

You'll need phpunit installed for this to work. To install:

    pear install --force --alldeps phpunit/phpunit

## Installing

### Unix

As root (or owner with write access to the directory
returned by php-config --extension-dir), run:

    make install

### Window
Copy php_geos.dll to extension directory and enable it in php.ini

## Using

 ... TODO ...
 (reference some automatically built doc?)
