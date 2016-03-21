--TEST--
General tests
--SKIPIF--
<?php if (!extension_loaded('geos')) print 'skip'; ?>
--FILE--
<?php

require './tests/TestHelper.php';

class GeneralTest extends GEOSTest
{
    public function testGEOSVersion()
    {
        $this->assertContains('-CAPI-', GEOSVersion());
    }

    public function testConstants()
    {
        $this->assertEquals(1, GEOSBUF_CAP_ROUND);
        $this->assertEquals(2, GEOSBUF_CAP_FLAT);
        $this->assertEquals(3, GEOSBUF_CAP_SQUARE);

        $this->assertEquals(1, GEOSBUF_JOIN_ROUND);
        $this->assertEquals(2, GEOSBUF_JOIN_MITRE);
        $this->assertEquals(3, GEOSBUF_JOIN_BEVEL);

        $this->assertEquals(0, GEOS_POINT);
        $this->assertEquals(1, GEOS_LINESTRING);
        $this->assertEquals(2, GEOS_LINEARRING);
        $this->assertEquals(3, GEOS_POLYGON);
        $this->assertEquals(4, GEOS_MULTIPOINT);
        $this->assertEquals(5, GEOS_MULTILINESTRING);
        $this->assertEquals(6, GEOS_MULTIPOLYGON);
        $this->assertEquals(7, GEOS_GEOMETRYCOLLECTION);

        $this->assertEquals(1, GEOSVALID_ALLOW_SELFTOUCHING_RING_FORMING_HOLE);

        $this->assertEquals(1, GEOSRELATE_BNR_MOD2);
        $this->assertEquals(1, GEOSRELATE_BNR_OGC);
        $this->assertEquals(2, GEOSRELATE_BNR_ENDPOINT);
        $this->assertEquals(3, GEOSRELATE_BNR_MULTIVALENT_ENDPOINT);
        $this->assertEquals(4, GEOSRELATE_BNR_MONOVALENT_ENDPOINT);
    }
}

GeneralTest::run();

?>
--EXPECT--
GeneralTest->testGEOSVersion	OK
GeneralTest->testConstants	OK