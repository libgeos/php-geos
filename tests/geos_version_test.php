<?php

define('GEOS_VERSION', getGeosVersion());
define('GEOS_USE_BRACKETED_MULTIPOINT', version_compare(GEOS_VERSION, '3.12', '>=')); // GH-903
define('GEOS_CHANGE_DIMENSION', version_compare(GEOS_VERSION, '3.12', '>='));  // GH-908
define('GEOS_CHANGE_VALUE', version_compare(GEOS_VERSION, '3.12', '>=')); 
//turn trim on(true), see : https://github.com/libgeos/geos/pull/915

function getGeosVersion()
{
    // Call GEOSVersion() function to get the GEOS version
    $geosVersionString = GEOSVersion();

    // Use a regular expression to extract the GEOS version
    if (preg_match('/(\d+\.\d+\.\d+)/', $geosVersionString, $matches)) {
        return $matches[1];
    } else {
        return null;
    }
}
