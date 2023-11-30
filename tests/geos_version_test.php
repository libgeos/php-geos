<?php

define('GEOS_VERSION', getGeosVersion());
define('GEOS_USE_BRACKETED_MULTIPOINT', version_compare(GEOS_VERSION, '3.12', '>=')); // GH-903
define('GEOS_WKB_DEFAULT_DIMENSIONS', version_compare(GEOS_VERSION, '3.12', '>=') ? 4 : 2);  // Change WKBWriter default output dimension to 4 GH-908
define('GEOS_CHANGE_VALUE', version_compare(GEOS_VERSION, '3.12', '>=')); 
define('GEOS_CORRECT_NEGATIVE_ZERO', version_compare(GEOS_VERSION, '3.12', '>=')); // Update since 3.12.1 see: https://github.com/libgeos/geos/blob/ddba88a37bd8edb0acc08c9753a1a0e28de3baee/tests/unit/capi/GEOSEqualsIdenticalTest.cpp#L203
define('GEOS_CORRECT_VALUE', version_compare(GEOS_VERSION, '3.11', '>=')); 
define('GEOS_DEFAULT_EMPTY', version_compare(GEOS_VERSION, '3.11', '>=')); // The bad mix appears to be with GEOS 3.10 and POLYGON EMPTY GH-501
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
