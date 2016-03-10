--TEST--
Geometry tests
--SKIPIF--
<?php if (!extension_loaded('geos')) print 'skip'; ?>
--FILE--
<?php

require './tests/TestHelper.php';

class GeometryTest extends GEOSTest
{
    public function testGeometry_serialization()
    {
        $reader = new GEOSWKTReader();

        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setTrim')) {
            $writer->setTrim(TRUE);
        }

        if (method_exists(GEOSWKTWriter::class, 'setOutputDimension')) {
            $writer->setOutputDimension(3);
        }

        $g = $reader->read('POINT(6 7 8)');
        $g->setSRID(54);

        $a = array('geom' => $g, 'name' => 'test geometry');
        $srl = serialize($a);
        $a2 = unserialize($srl);

        $this->assertEquals('POINT Z (6 7 8)', $writer->write($a['geom']));

    }

    public function testGeometry_project()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(1 2)');
        $g2 = $reader->read('POINT(3 4)');

        /* The method only accept lineal geometries */
        try {
            $prj = $g->project($g2);
            $this->assertTrue(FALSE); # this is just to fail if we get here
        } catch (Exception $e) {
            $this->assertContains('lineal', $e->getMessage());
        }

        $g = $reader->read('LINESTRING(0 0, 10 0)');

        $g2 = $reader->read('POINT(0 0)');
        $prj = $g->project($g2);
        $this->assertEquals(0, $prj);
        $prj = $g->project($g2, TRUE);
        $this->assertEquals(0, $prj);

        $g2 = $reader->read('POINT(10 0)');
        $prj = $g->project($g2);
        $this->assertEquals(10, $prj);
        $prj = $g->project($g2, TRUE);
        $this->assertEquals(1, $prj);

        $g2 = $reader->read('POINT(5 0)');
        $prj = $g->project($g2);
        $this->assertEquals(5, $prj);
        $prj = $g->project($g2, TRUE);
        $this->assertEquals(0.5, $prj);

        $g = $reader->read('MULTILINESTRING((0 0, 10 0),(20 10, 20 20))');

        $g2 = $reader->read('POINT(20 0)');
        $prj = $g->project($g2);
        $this->assertEquals(10, $prj);
        $prj = $g->project($g2, TRUE);
        $this->assertEquals(0.5, $prj);

        $g2 = $reader->read('POINT(20 5)');
        $prj = $g->project($g2);
        $this->assertEquals(10, $prj);
        $prj = $g->project($g2, TRUE);
        $this->assertEquals(0.5, $prj);


    }

    public function testGeometry_interpolate()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setTrim')) {
            $writer->setTrim(TRUE);
        }

        /* The method only accept LineString geometries */
        $g = $reader->read('POINT(1 2)');
        try {
            $prj = $g->interpolate(0);
            $this->assertTrue(FALSE); # this is just to fail if we get here
        } catch (Exception $e) {
            $this->assertContains('LineString', $e->getMessage());
        }

        $g = $reader->read('LINESTRING(0 0, 10 0)');

        $prj = $g->interpolate(0);
        $this->assertNotNull($prj);
        $this->assertEquals('POINT (0 0)', $writer->write($prj));
        $prj = $g->interpolate(0, TRUE);
        $this->assertNotNull($prj);
        $this->assertEquals('POINT (0 0)', $writer->write($prj));

        $prj = $g->interpolate(5);
        $this->assertNotNull($prj);
        $this->assertEquals('POINT (5 0)', $writer->write($prj));
        $prj = $g->interpolate(0.5, TRUE);
        $this->assertNotNull($prj);
        $this->assertEquals('POINT (5 0)', $writer->write($prj));

        /* return closest on longer distance */
        $prj = $g->interpolate(20);
        $this->assertNotNull($prj);
        $this->assertEquals('POINT (10 0)', $writer->write($prj));
        $prj = $g->interpolate(2, TRUE);
        $this->assertNotNull($prj);
        $this->assertEquals('POINT (10 0)', $writer->write($prj));

    }

    public function testGeometry_buffer()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $b = $g->buffer(0);
        $this->assertEquals('POLYGON EMPTY', $writer->write($b));

        $b = $g->buffer(10);
        $this->assertEquals('POLYGON ((10 0, 10 -2, 9 -4, 8 -6, 7 -7, 6 -8, 4 -9, 2 -10, 0 -10, -2 -10, -4 -9, -6 -8, -7 -7, -8 -6, -9 -4, -10 -2, -10 -0, -10 2, -9 4, -8 6, -7 7, -6 8, -4 9, -2 10, -0 10, 2 10, 4 9, 6 8, 7 7, 8 6, 9 4, 10 2, 10 0))', $writer->write($b));

        # One segment per quadrant
        $b = $g->buffer(10, array('quad_segs' => 1));
        $this->assertEquals('POLYGON ((10 0, 0 -10, -10 -0, -0 10, 10 0))', $writer->write($b));

        /* End cap styles */

        $g = $reader->read('LINESTRING(0 0, 100 0)');

        $b = $g->buffer(10, array(
            'quad_segs' => 1,
            'endcap' => GEOSBUF_CAP_ROUND
        ));
        $this->assertEquals('POLYGON ((100 10, 110 0, 100 -10, 0 -10, -10 0, 0 10, 100 10))', $writer->write($b));

        $b = $g->buffer(10, array(
            'quad_segs' => 1,
            'endcap' => GEOSBUF_CAP_FLAT
        ));
        $this->assertEquals('POLYGON ((100 10, 100 -10, 0 -10, 0 10, 100 10))', $writer->write($b));

        $b = $g->buffer(10, array(
            'quad_segs' => 1,
            'endcap' => GEOSBUF_CAP_SQUARE
        ));
        $this->assertEquals('POLYGON ((100 10, 110 10, 110 -10, 0 -10, -10 -10, -10 10, 100 10))', $writer->write($b));

        /* Join styles */

        $g = $reader->read('LINESTRING(0 0, 100 0, 100 100)');

        $b = $g->buffer(10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_ROUND
        ));
        $this->assertEquals('POLYGON ((90 10, 90 100, 93 107, 100 110, 107 107, 110 100, 110 0, 107 -7, 100 -10, 0 -10, -7 -7, -10 0, -7 7, 0 10, 90 10))', $writer->write($b));

        $b = $g->buffer(10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_BEVEL
        ));
        $this->assertEquals('POLYGON ((90 10, 90 100, 93 107, 100 110, 107 107, 110 100, 110 0, 100 -10, 0 -10, -7 -7, -10 0, -7 7, 0 10, 90 10))', $writer->write($b));

        $b = $g->buffer(10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_MITRE
        ));
        $this->assertEquals('POLYGON ((90 10, 90 100, 93 107, 100 110, 107 107, 110 100, 110 -10, 0 -10, -7 -7, -10 0, -7 7, 0 10, 90 10))', $writer->write($b));

        $b = $g->buffer(10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_MITRE,
            'mitre_limit' => 1.0
        ));
        $this->assertEquals('POLYGON ((90 10, 90 100, 93 107, 100 110, 107 107, 110 100, 109 -5, 105 -9, 0 -10, -7 -7, -10 0, -7 7, 0 10, 90 10))', $writer->write($b));

        /* Check that elements of the passed style array are not
         * type-converted (buffer op will need to type-convert
         * internally)
         */
        $ary = array('a' => 1);

        $myStyle = array(
            'quad_segs' => "a string",
            'join' => "1",
            'endcap' => $ary,
            'mitre_limit' => 2 /* an int.. */
        );
        $this->assertEquals('string', gettype($myStyle['quad_segs']));
        $this->assertEquals('string', gettype($myStyle['join']));
        $this->assertEquals('array', gettype($myStyle['endcap']));
        $this->assertEquals('integer', gettype($myStyle['mitre_limit']));
        $b = $g->buffer(10, $myStyle);
        $this->assertEquals('string', gettype($myStyle['quad_segs']));
        $this->assertEquals('string', gettype($myStyle['join']));
        $this->assertEquals('array', gettype($myStyle['endcap']));
        $this->assertEquals('integer', gettype($myStyle['mitre_limit']));

        /* Single-sided buffering */

        $g = $reader->read('LINESTRING(0 0, 100 0)');

        $b = $g->buffer(10, array(
            'single_sided' => true
        ));
        $this->assertEquals('POLYGON ((100 0, 0 0, 0 10, 100 10, 100 0))', $writer->write($b));

        $b = $g->buffer(-10, array(
            'single_sided' => true
        ));
        $this->assertEquals('POLYGON ((0 0, 100 0, 100 -10, 0 -10, 0 0))', $writer->write($b));
    }

    public function testGeometry_offsetCurve()
    {
        if (!method_exists(GEOSGeometry::class, 'offsetCurve')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        $writer->setRoundingPrecision(0);

        /* Join styles */

        $g = $reader->read('LINESTRING(0 0, 100 0, 100 100)');

        /* left, round join */
        $b = $g->offsetCurve(10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_ROUND
        ));
        $this->assertEquals('LINESTRING (0 10, 90 10, 90 100)', $writer->write($b));

        /* right, round join */
        $b = $g->offsetCurve(-10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_ROUND
        ));
        $this->assertEquals('LINESTRING (110 100, 110 0, 107 -7, 100 -10, 0 -10)', $writer->write($b));

        /* left, bevel join */
        $b = $g->offsetCurve(10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_BEVEL
        ));
        $this->assertEquals('LINESTRING (0 10, 90 10, 90 100)', $writer->write($b));

        /* right, bevel join */
        $b = $g->offsetCurve(-10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_BEVEL
        ));
        $this->assertEquals('LINESTRING (110 100, 110 0, 100 -10, 0 -10)', $writer->write($b));

          /* left, mitre join */
        $b = $g->offsetCurve(10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_MITRE
        ));
        $this->assertEquals('LINESTRING (0 10, 90 10, 90 100)', $writer->write($b));

        /* right, mitre join */
        $b = $g->offsetCurve(-10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_MITRE
        ));
        $this->assertEquals('LINESTRING (110 100, 110 -10, 0 -10)', $writer->write($b));

        /* right, mitre join limited */
        $b = $g->offsetCurve(-10, array(
            'quad_segs' => 2,
            'join' => GEOSBUF_JOIN_MITRE,
            'mitre_limit' => 1.0
        ));
        $this->assertEquals('LINESTRING (110 100, 109 -5, 105 -9, 0 -10)', $writer->write($b));
    }

    public function testGeometry_envelope()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $b = $g->envelope();
        $this->assertEquals('POINT (0 0)', $writer->write($b));

        $g = $reader->read('LINESTRING(0 0, 10 10)');
        $b = $g->envelope();
        $this->assertEquals('POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0))', $writer->write($b));
    }

    public function testGeometry_intersection()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        /* POINT - POINT */
        $g = $reader->read('POINT(0 0)');
        $g2 = $reader->read('POINT(0 0)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'POINT (0 0)', $writer->write($gi));
        $g2 = $reader->read('POINT(1 0)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'GEOMETRYCOLLECTION EMPTY', $writer->write($gi));

        /* POINT - LINE */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('POINT(5 0)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'POINT (5 0)', $writer->write($gi));
        $g2 = $reader->read('POINT(12 0)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'GEOMETRYCOLLECTION EMPTY', $writer->write($gi));

        /* LINE - LINE */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'POINT (5 0)', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(5 0, 20 0)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'LINESTRING (5 0, 10 0)', $writer->write($gi));

        /* LINE - POLY */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'LINESTRING (5 0, 5 10)', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(10 0, 20 0)');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'POINT (10 0)', $writer->write($gi));

        /* POLY - POLY */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POLYGON((5 -5, 5 5, 15 5, 15 -5, 5 -5))');
        $gi = $g->intersection($g2);
        $this->assertTrue($gi->equals($reader->read('POLYGON ((10 5, 10 0, 5 0, 5 5, 10 5))')));
        $g2 = $reader->read('POLYGON((10 0, 20 0, 20 -5, 10 -5, 10 0))');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'POINT (10 0)', $writer->write($gi));
        $g2 = $reader->read('POLYGON((8 0, 20 0, 20 -5, 10 -5, 8 0))');
        $gi = $g->intersection($g2);
        $this->assertEquals( 'LINESTRING (8 0, 10 0)', $writer->write($gi));
    }

    public function testGeometry_clipByRect()
    {
        if (!method_exists(GEOSGeometry::class, 'clipByRect')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();
        $writer->setRoundingPrecision(0);

        /* POINT */
        $g = $reader->read('POINT(0 0)');
        $gi = $g->clipByRect(-1,-1,1,1);
        $this->assertEquals( 'POINT (0 0)', $writer->write($gi));
        $gi = $g->clipByRect(1,1,2,2);
        $this->assertEquals( 'GEOMETRYCOLLECTION EMPTY', $writer->write($gi));

        /* LINE */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $gi = $g->clipByRect(1,-1,2,1);
        $this->assertEquals( 'LINESTRING (1 0, 2 0)', $writer->write($gi));

        /* POLY */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $gi = $g->clipByRect(1,1,5,5);
        $this->assertEquals( 'POLYGON ((1 1, 1 5, 5 5, 5 1, 1 1))', $writer->write($gi));
        $gi = $g->clipByRect(-1,-1,5,5);
        $this->assertEquals( 'POLYGON ((0 0, 0 5, 5 5, 5 0, 0 0))', $writer->write($gi));
    }

    public function testGeometry_convexHull()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $b = $g->convexHull();
        $this->assertEquals('POINT (0 0)', $writer->write($b));

        $g = $reader->read('LINESTRING(0 0, 10 10)');
        $b = $g->convexHull();
        $this->assertEquals('LINESTRING (0 0, 10 10)', $writer->write($b));

        $g = $reader->read('POLYGON((0 0, 0 10, 5 5, 10 10, 10 0, 0 0))');
        $b = $g->convexHull();
        $this->assertEquals('POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0))', $writer->write($b));
    }

    public function testGeometry_difference()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        /* POINT - POINT */
        $g = $reader->read('POINT(0 0)');
        $g2 = $reader->read('POINT(0 0)');
        $gi = $g->difference($g2);
        $this->assertEquals( 'GEOMETRYCOLLECTION EMPTY', $writer->write($gi));
        $g2 = $reader->read('POINT(1 0)');
        $gi = $g->difference($g2);
        $this->assertEquals('POINT (0 0)', $writer->write($gi));

        /* LINE - POINT */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('POINT(5 0)');
        $gi = $g->difference($g2);
        $this->assertEquals('LINESTRING (0 0, 10 0)', $writer->write($gi));

        /* POINT - LINE */
        $g = $reader->read('POINT(5 0)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');
        $gi = $g->difference($g2);
        $this->assertEquals('GEOMETRYCOLLECTION EMPTY', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(0 1, 10 1)');
        $gi = $g->difference($g2);
        $this->assertEquals( 'POINT (5 0)', $writer->write($gi));

        /* LINE - LINE */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->difference($g2);
        $this->assertEquals( 'MULTILINESTRING ((0 0, 5 0), (5 0, 10 0))', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(5 0, 20 0)');
        $gi = $g->difference($g2);
        $this->assertEquals( 'LINESTRING (0 0, 5 0)', $writer->write($gi));

        /* POLY - LINE */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->difference($g2);
        $this->assertEquals('POLYGON ((5 0, 0 0, 0 10, 5 10, 10 10, 10 0, 5 0))', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(10 0, 20 0)');
        $gi = $g->difference($g2);
        $this->assertEquals('POLYGON ((10 0, 0 0, 0 10, 10 10, 10 0))', $writer->write($gi));

        /* POLY - POLY */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POLYGON((5 -5, 5 5, 15 5, 15 -5, 5 -5))');
        $gi = $g->difference($g2);
        $this->assertEquals('POLYGON ((5 0, 0 0, 0 10, 10 10, 10 5, 5 5, 5 0))', $writer->write($gi));
    }

    public function testGeometry_symdifference()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        /* POINT - POINT */
        $g = $reader->read('POINT(0 0)');
        $g2 = $reader->read('POINT(0 0)');
        $gi = $g->symDifference($g2);
        $this->assertEquals( 'GEOMETRYCOLLECTION EMPTY', $writer->write($gi));
        $g2 = $reader->read('POINT(1 0)');
        $gi = $g->symDifference($g2);
        $this->assertEquals( 'MULTIPOINT (0 0, 1 0)', $writer->write($gi));

        /* LINE - POINT */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('POINT(5 0)');
        $gi = $g->symDifference($g2);
        $this->assertEquals('LINESTRING (0 0, 10 0)', $writer->write($gi));

        /* POINT - LINE */
        $g = $reader->read('POINT(5 0)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');
        $gi = $g->symDifference($g2);
        $this->assertEquals( 'LINESTRING (0 0, 10 0)', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(0 1, 10 1)');
        $gi = $g->symDifference($g2);
        $this->assertEquals('GEOMETRYCOLLECTION (POINT (5 0), LINESTRING (0 1, 10 1))', $writer->write($gi));

        /* LINE - LINE */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->symDifference($g2);
        $this->assertEquals('MULTILINESTRING ((0 0, 5 0), (5 0, 10 0), (5 -10, 5 0), (5 0, 5 10))', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(5 0, 20 0)');
        $gi = $g->symDifference($g2);
        $this->assertEquals('MULTILINESTRING ((0 0, 5 0), (10 0, 20 0))', $writer->write($gi));

        /* POLY - LINE */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->symDifference($g2);
        $this->assertEquals('GEOMETRYCOLLECTION (LINESTRING (5 -10, 5 0), POLYGON ((5 0, 0 0, 0 10, 5 10, 10 10, 10 0, 5 0)))', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(10 0, 20 0)');
        $gi = $g->symDifference($g2);
        $this->assertEquals('GEOMETRYCOLLECTION (LINESTRING (10 0, 20 0), POLYGON ((10 0, 0 0, 0 10, 10 10, 10 0)))', $writer->write($gi));

        /* POLY - POLY */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POLYGON((5 -5, 5 5, 15 5, 15 -5, 5 -5))');
        $gi = $g->symDifference($g2);
        $this->assertEquals('MULTIPOLYGON (((5 0, 0 0, 0 10, 10 10, 10 5, 5 5, 5 0)), ((5 0, 10 0, 10 5, 15 5, 15 -5, 5 -5, 5 0)))', $writer->write($gi));
    }

    public function testGeometry_boundary()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $b = $g->boundary();
        $this->assertEquals('GEOMETRYCOLLECTION EMPTY', $writer->write($b));

        $g = $reader->read('LINESTRING(0 0, 10 10)');
        $b = $g->boundary();
        $this->assertEquals('MULTIPOINT (0 0, 10 10)', $writer->write($b));

        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0),( 5 5, 5 6, 6 6, 6 5, 5 5))');
        $b = $g->boundary();
        $this->assertEquals('MULTILINESTRING ((0 0, 10 0, 10 10, 0 10, 0 0), (5 5, 5 6, 6 6, 6 5, 5 5))', $writer->write($b));
    }

    public function testGeometry_union()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        /* POINT - POINT */
        $g = $reader->read('POINT(0 0)');
        $g2 = $reader->read('POINT(0 0)');
        $gi = $g->union($g2);
        $this->assertEquals( 'POINT (0 0)', $writer->write($gi));
        $g2 = $reader->read('POINT(1 0)');
        $gi = $g->union($g2);
        $this->assertEquals( 'MULTIPOINT (0 0, 1 0)', $writer->write($gi));

        /* LINE - POINT */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('POINT(5 0)');
        $gi = $g->union($g2);
        $this->assertEquals('LINESTRING (0 0, 10 0)', $writer->write($gi));

        /* POINT - LINE */
        $g = $reader->read('POINT(5 0)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');
        $gi = $g->union($g2);
        $this->assertEquals( 'LINESTRING (0 0, 10 0)', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(0 1, 10 1)');
        $gi = $g->union($g2);
        $this->assertEquals('GEOMETRYCOLLECTION (POINT (5 0), LINESTRING (0 1, 10 1))', $writer->write($gi));

        /* LINE - LINE */
        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->union($g2);
        $this->assertEquals('MULTILINESTRING ((0 0, 5 0), (5 0, 10 0), (5 -10, 5 0), (5 0, 5 10))', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(5 0, 20 0)');
        $gi = $g->union($g2);
        $this->assertEquals('MULTILINESTRING ((0 0, 5 0), (5 0, 10 0), (10 0, 20 0))', $writer->write($gi));

        /* POLY - LINE */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('LINESTRING(5 -10, 5 10)');
        $gi = $g->union($g2);
        $this->assertEquals('GEOMETRYCOLLECTION (LINESTRING (5 -10, 5 0), POLYGON ((5 0, 0 0, 0 10, 5 10, 10 10, 10 0, 5 0)))', $writer->write($gi));
        $g2 = $reader->read('LINESTRING(10 0, 20 0)');
        $gi = $g->union($g2);
        $this->assertEquals('GEOMETRYCOLLECTION (LINESTRING (10 0, 20 0), POLYGON ((10 0, 0 0, 0 10, 10 10, 10 0)))', $writer->write($gi));

        /* POLY - POLY */
        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POLYGON((5 -5, 5 5, 15 5, 15 -5, 5 -5))');
        $gi = $g->union($g2);
        $this->assertEquals('POLYGON ((5 0, 0 0, 0 10, 10 10, 10 5, 15 5, 15 -5, 5 -5, 5 0))', $writer->write($gi));
    }

    public function testGeometry_unaryunion()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('MULTIPOLYGON(
                 ((0 0, 1 0, 1 1, 0 1, 0 0)),
                 ((10 10, 10 14, 14 14, 14 10, 10 10),
                  (11 11, 11 12, 12 12, 12 11, 11 11)),
                 ((0 0, 11 0, 11 11, 0 11, 0 0))
                ))');

        $gu = $g->union();
        $this->assertEquals('POLYGON ((1 0, 0 0, 0 1, 0 11, 10 11, 10 14, 14 14, 14 10, 11 10, 11 0, 1 0), (11 11, 12 11, 12 12, 11 12, 11 11))', $writer->write($gu));

        $g = $reader->read('MULTILINESTRING(
                 (0 0, 1 0, 1 1, 0 1, 0 0),
                 (10 10, 10 14, 14 14, 14 10, 10 10),
                  (11 11, 11 12, 12 12, 12 11, 11 11),
                 (0 0, 11 0, 11 11, 0 11, 0 0)
                )');

        $gu = $g->union();
        $this->assertEquals('MULTILINESTRING ((0 0, 1 0), (1 0, 1 1, 0 1), (0 1, 0 0), (1 0, 11 0, 11 10), (11 10, 11 11), (11 11, 10 11), (10 11, 0 11, 0 1), (11 11, 11 12, 12 12, 12 11, 11 11), (10 10, 10 11), (10 11, 10 14, 14 14, 14 10, 11 10), (11 10, 10 10))', $writer->write($gu));

        $g = $reader->read('MULTIPOINT(
                 0 0, 1 0, 1 1, 0 1, 0 0,
                 10 10, 10 14, 14 14, 14 10, 10 10,
                  11 11, 11 12, 12 12, 12 11, 11 11,
                 0 0, 11 0, 11 11, 0 11, 0 0
                )');

        $gu = $g->union();
        $this->assertEquals('MULTIPOINT (0 0, 0 1, 0 11, 1 0, 1 1, 10 10, 10 14, 11 0, 11 11, 11 12, 12 11, 12 12, 14 10, 14 14)', $writer->write($gu));

        $g = $reader->read('GEOMETRYCOLLECTION(
                MULTIPOLYGON(
                 ((0 0, 1 0, 1 1, 0 1, 0 0)),
                 ((10 10, 10 14, 14 14, 14 10, 10 10),
                  (11 11, 11 12, 12 12, 12 11, 11 11)),
                 ((0 0, 11 0, 11 11, 0 11, 0 0))
                ),
                MULTILINESTRING(
                 (0 0, 1 0, 1 1, 0 1, 0 0),
                 (10 10, 10 14, 14 14, 14 10, 10 10),
                  (11 11, 11 12, 12 12, 12 11, 11 11),
                 (0 0, 11 0, 11 11, 0 11, 0 0),(-8 8, -8 6)
                ),
                MULTIPOINT(
                 0 0, 1 0, 1 1, 0 1, 0 0,
                 10 10, 10 14, 14 14, 14 10, 10 10,
                  11 11, 11 12, 12 12, 12 11, 11 11,
                 0 0, 11 0, 11 11, 0 11, 0 0, -10 -10
                ))');

        $gu = $g->union();
        $this->assertEquals('GEOMETRYCOLLECTION (POINT (-10 -10), LINESTRING (-8 8, -8 6), POLYGON ((1 0, 0 0, 0 1, 0 11, 10 11, 10 14, 14 14, 14 10, 11 10, 11 0, 1 0), (11 12, 11 11, 12 11, 12 12, 11 12)))', $writer->write($gu));
    }

    public function testGeometry_pointOnSurface()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $b = $g->pointOnSurface();
        $this->assertEquals('POINT (0 0)', $writer->write($b));

        $g = $reader->read('LINESTRING(0 0, 5 5, 10 10)');
        $b = $g->pointOnSurface();
        $this->assertEquals('POINT (5 5)', $writer->write($b));

        $g = $reader->read('POLYGON((0 0, 0 10, 5 5, 10 10, 10 0, 0 0))');
        $b = $g->pointOnSurface();
        $this->assertEquals('POINT (1 8)', $writer->write($b));
    }

    public function testGeometry_centroid()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $b = $g->centroid();
        $this->assertEquals('POINT (0 0)', $writer->write($b));

        $g = $reader->read('LINESTRING(0 0, 10 10)');
        $b = $g->centroid();
        $this->assertEquals('POINT (5 5)', $writer->write($b));

        $g = $reader->read('POLYGON((0 0, 0 10, 5 5, 10 10, 10 0, 0 0))');
        $b = $g->centroid();
        $this->assertEquals('POINT (5 4)', $writer->write($b));
    }

    public function testGeometry_relate()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $g2 = $reader->read('POINT(0 0)');
        $ret = $g->relate($g2);
        $this->assertEquals('0FFFFFFF2', $ret);
        $ret = $g->relate($g2, '0FFFFFFF2');
        $this->assertEquals(TRUE, $ret);
        $ret = $g->relate($g2, '0*******T');
        $this->assertEquals(TRUE, $ret);
        $ret = $g->relate($g2, '0*******1');
        $this->assertEquals(FALSE, $ret);

        $g = $reader->read('POINT(0 0)');
        $g2 = $reader->read('POINT(1 0)');
        $ret = $g->relate($g2);
        $this->assertEquals('FF0FFF0F2', $ret);
        $ret = $g->relate($g2, 'FF0FFF0F2');
        $this->assertEquals(TRUE, $ret);
        $ret = $g->relate($g2, 'F*******2');
        $this->assertEquals(TRUE, $ret);
        $ret = $g->relate($g2, 'T*******2');
        $this->assertEquals(FALSE, $ret);

        $g = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POINT(1 0)');
        $ret = $g->relate($g2);
        $this->assertEquals('FF20F1FF2', $ret);
        $ret = $g->relate($g2, 'FF20F1FF2');
        $this->assertEquals(TRUE, $ret);
        $ret = $g->relate($g2, 'F****T**T');
        $this->assertEquals(TRUE, $ret);
        $ret = $g->relate($g2, 'T*******2');
        $this->assertEquals(FALSE, $ret);

    }

    public function testGeometry_relateBoundaryNodeRule()
    {
        if (!method_exists(GEOSGeometry::class, 'relateBoundaryNodeRule')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('LINESTRING(0 0, 2 4, 5 5, 0 0)');
        $g2 = $reader->read('POINT(0 0)');

        $ret = $g->relateBoundaryNodeRule($g2, GEOSRELATE_BNR_OGC);
        $this->assertEquals('0F1FFFFF2', $ret);

        $ret = $g->relateBoundaryNodeRule($g2, GEOSRELATE_BNR_ENDPOINT);
        $this->assertEquals('FF10FFFF2', $ret);

    }

    public function testGeometry_polygonize()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('GEOMETRYCOLLECTION(
            LINESTRING(0 0, 10 10),
            LINESTRING(185 221, 100 100),
            LINESTRING(185 221, 88 275, 180 316),
            LINESTRING(185 221, 292 281, 180 316),
            LINESTRING(189 98, 83 187, 185 221),
            LINESTRING(189 98, 325 168, 185 221))
            )');

        $g2 = $reader->read('POINT(0 0)');

        $ret = GEOSPolygonize($g);

        $this->assertEquals('array', gettype($ret));
        $this->assertEquals('array', gettype($ret['rings']));
        $this->assertEquals('array', gettype($ret['cut_edges']));
        $this->assertEquals('array', gettype($ret['dangles']));
        $this->assertEquals('array', gettype($ret['invalid_rings']));

        $this->assertEquals(2, count($ret['rings']));
        $this->assertEquals('POLYGON ((185 221, 88 275, 180 316, 292 281, 185 221))', # JTS-confirmed!
            $writer->write($ret['rings'][0]));
        $this->assertEquals('POLYGON ((189 98, 83 187, 185 221, 325 168, 189 98))', # JTS-confirmed !
            $writer->write($ret['rings'][1]));

        $this->assertEquals(0, count($ret['cut_edges']));

        $this->assertEquals(0, count($ret['invalid_rings']));

        /*
         * FIXME: the duplicated dangle (0 0, 10 10) is unexpected
         */

        $this->assertEquals(2, count($ret['dangles']));
        $this->assertEquals('LINESTRING (185 221, 100 100)', # JTS-confirmed !
            $writer->write($ret['dangles'][0]));
        $this->assertEquals('LINESTRING (0 0, 10 10)', # JTS-confirmed !
            $writer->write($ret['dangles'][1]));


        ###########################################################

        $g = $g->union($g2); /* Now make sure linestrings are noded */

        $ret = GEOSPolygonize($g);

        $this->assertEquals('array', gettype($ret));
        $this->assertEquals('array', gettype($ret['rings']));
        $this->assertEquals('array', gettype($ret['cut_edges']));
        $this->assertEquals('array', gettype($ret['dangles']));
        $this->assertEquals('array', gettype($ret['invalid_rings']));

        $this->assertEquals(2, count($ret['dangles']));
        $this->assertEquals('LINESTRING (132 146, 100 100)', $writer->write($ret['dangles'][0]));
        $this->assertEquals('LINESTRING (0 0, 10 10)', $writer->write($ret['dangles'][1]));

        $this->assertEquals(0, count($ret['invalid_rings']));

    // TODO: test a polygonize run with cut lines and invalid_rings

    }

    public function testGeometry_lineMerge()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('MULTILINESTRING(
            (0 0, 10 10),
            (10 10, 10 0),
            (5 0, 10 0),
            (5 -5, 5 0)
            )');

        $ret = GEOSLineMerge($g);

        $this->assertEquals('array', gettype($ret));
        $this->assertEquals('1', count($ret));

        $this->assertEquals('LINESTRING (0 0, 10 10, 10 0, 5 0, 5 -5)', $writer->write($ret[0]));

    }

    public function testGeometry_sharedPaths()
    {
        if (!function_exists('GEOSSharedPaths')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        /* LINE - LINE */
        $g1 = $reader->read('LINESTRING(0 0, 50 0)');
        $g2 = $reader->read('MULTILINESTRING((5 0, 15 0),(40 0, 30 0))');
        $gs = GEOSSharedPaths($g1, $g2);
        $this->assertEquals('GEOMETRYCOLLECTION (MULTILINESTRING ((5 0, 15 0)), MULTILINESTRING ((30 0, 40 0)))', $writer->write($gs));
    }

    public function testGeometry_simplify()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('LINESTRING(0 0, 3 4, 5 10, 10 0, 10 9, 5 11, 0 9)');
        $gs = $g->simplify(2);
        $this->assertEquals( 'LINESTRING (0 0, 5 10, 10 0, 10 9, 0 9)', $writer->write($gs));
        $gs = $g->simplify(2, TRUE);
        $this->assertEquals( 'LINESTRING (0 0, 5 10, 10 0, 10 9, 5 11, 0 9)', $writer->write($gs));
    }

    public function testGeometry_extractUniquePoints()
    {
        if (!method_exists(GEOSGeometry::class, 'extractUniquePoints')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read(
    'GEOMETRYCOLLECTION (
        MULTIPOLYGON (
            ((0 0, 1 0, 1 1, 0 1, 0 0)),
            ((10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11))
        ),
        POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0)),
        MULTILINESTRING ((0 0, 2 3), (10 10, 3 4)),
        LINESTRING (0 0, 2 3),
        MULTIPOINT (0 0, 2 3),
        POINT (9 0),
        POINT(1 0)),
        LINESTRING EMPTY
    ');

        $gs = $g->extractUniquePoints();
        if ( ! $gs ) RETURN_NULL(); /* should get an exception before */

        $this->assertEquals('MULTIPOINT (0 0, 1 0, 1 1, 0 1, 10 10, 10 14, 14 14, 14 10, 11 11, 11 12, 12 12, 12 11, 2 3, 3 4, 9 0)', $writer->write($gs));
    }

    public function testGeometry_relationalOps()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g1 = $reader->read('POINT(0 0)');
        $g2 = $reader->read('POINT(0 0)');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertFalse( $g1->touches($g2) ); /* no bounds, can't touch */
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertTrue( $g1->within($g2) );
        $this->assertTrue( $g1->contains($g2) );
        $this->assertFalse( $g1->overlaps($g2) );
        $this->assertTrue( $g1->equals($g2) );
        $this->assertTrue( $g1->equalsExact($g2) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertTrue( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertTrue( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('POINT(0 0)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertTrue( $g1->touches($g2) );
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertFalse( $g1->within($g2) );
        $this->assertFalse( $g1->contains($g2) );
        $this->assertFalse( $g1->overlaps($g2) );
        $this->assertFalse( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 10) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertFalse( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertTrue( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('POINT(5 0)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertFalse( $g1->touches($g2) );
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertTrue( $g1->within($g2) );
        $this->assertFalse( $g1->contains($g2) );
        $this->assertFalse( $g1->overlaps($g2) );
        $this->assertFalse( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 10) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertFalse( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertTrue( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('LINESTRING(5 -5, 5 5)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertFalse( $g1->touches($g2) );
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertTrue( $g1->crosses($g2) );
        $this->assertFalse( $g1->within($g2) );
        $this->assertFalse( $g1->contains($g2) );
        $this->assertFalse( $g1->overlaps($g2) );
        $this->assertFalse( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 1) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertFalse( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertFalse( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('LINESTRING(5 0, 15 0)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertFalse( $g1->touches($g2) );
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertFalse( $g1->within($g2) );
        $this->assertFalse( $g1->contains($g2) );
        $this->assertTrue( $g1->overlaps($g2) );
        $this->assertFalse( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 1) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertFalse( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertFalse( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('LINESTRING(0 0, 5 0, 10 0)');
        $g2 = $reader->read('LINESTRING(0 0, 10 0)');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertFalse( $g1->touches($g2) );
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertTrue( $g1->within($g2) );
        $this->assertTrue( $g1->contains($g2) );
        $this->assertFalse( $g1->overlaps($g2) );
        $this->assertTrue( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 1) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertTrue( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertTrue( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POLYGON((5 -5, 5 5, 15 5, 15 -5, 5 -5))');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertFalse( $g1->touches($g2) );
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertFalse( $g1->within($g2) );
        $this->assertFalse( $g1->contains($g2) );
        $this->assertTrue( $g1->overlaps($g2) );
        $this->assertFalse( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 1) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertFalse( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertFalse( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POINT(15 15)');

        $this->assertTrue( $g1->disjoint($g2) );
        $this->assertFalse( $g1->touches($g2) );
        $this->assertFalse( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertFalse( $g1->within($g2) );
        $this->assertFalse( $g1->contains($g2) );
        $this->assertFalse( $g1->overlaps($g2) );
        $this->assertFalse( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 1) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertFalse( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertFalse( $g1->coveredBy($g2) );
        }

        $g1 = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
        $g2 = $reader->read('POINT(5 0)');

        $this->assertFalse( $g1->disjoint($g2) );
        $this->assertTrue( $g1->touches($g2) );
        $this->assertTrue( $g1->intersects($g2) );
        $this->assertFalse( $g1->crosses($g2) );
        $this->assertFalse( $g1->within($g2) );
        $this->assertFalse( $g1->contains($g2) );
        $this->assertFalse( $g1->overlaps($g2) );
        $this->assertFalse( $g1->equals($g2) );
        $this->assertFalse( $g1->equalsExact($g2, 1) );

        if (method_exists(GEOSGeometry::class, 'covers')) {
            $this->assertTrue( $g1->covers($g2) );
        }

        if (method_exists(GEOSGeometry::class, 'coveredBy')) {
            $this->assertFalse( $g1->coveredBy($g2) );
        }

    }

    public function testGeometry_isEmpty()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g1 = $reader->read('POINT(0 0)');
        $this->assertFalse( $g1->isEmpty() );

        $g1 = $reader->read('POINT EMPTY');
        $this->assertTrue( $g1->isEmpty() );

        $g1 = $reader->read('LINESTRING(0 0, 10 0)');
        $this->assertFalse( $g1->isEmpty() );

        $g1 = $reader->read('LINESTRING EMPTY');
        $this->assertTrue( $g1->isEmpty() );

        $g1 = $reader->read('POLYGON((0 0, 10 0, 10 10, 0 0))');
        $this->assertFalse( $g1->isEmpty() );

        $g1 = $reader->read('POLYGON EMPTY');
        $this->assertTrue( $g1->isEmpty() );

        $g1 = $reader->read('GEOMETRYCOLLECTION(POINT(0 0))');
        $this->assertFalse( $g1->isEmpty() );

        $g1 = $reader->read('GEOMETRYCOLLECTION EMPTY');
        $this->assertTrue( $g1->isEmpty() );
    }

    public function testGeometry_checkValidity()
    {
        if (!method_exists(GEOSGeometry::class, 'checkValidity')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POINT(0 0)');
        $val = $g->checkValidity();
        $this->assertType( 'array', $val );
        $this->assertTrue( $val['valid'] );
        $this->assertFalse( isset($val['reason']) );
        $this->assertFalse( isset($val['location']) );

        $g = $reader->read('POINT(0 NaN)');
        $val = $g->checkValidity();
        $this->assertType( 'array', $val );
        $this->assertFalse( $val['valid'] );
        $this->assertEquals( 'Invalid Coordinate', $val['reason'] );
        $this->assertEquals( 'POINT (0 nan)',
            $writer->write($val['location']) );

        $g = $reader->read(
           'POLYGON((0 0, -10 10, 10 10, 0 0, 4 5, -4 5, 0 0)))'
        );
        $val = $g->checkValidity();
        $this->assertType( 'array', $val );
        $this->assertFalse( $val['valid'] );
        $this->assertEquals( 'Ring Self-intersection', $val['reason'] );
        $this->assertEquals( 'POINT (0 0)',
            $writer->write($val['location']) );

        $g = $reader->read(
           'POLYGON((0 0, -10 10, 10 10, 0 0, 4 5, -4 5, 0 0)))'
        );
        $flags = GEOSVALID_ALLOW_SELFTOUCHING_RING_FORMING_HOLE;
        $val = $g->checkValidity($flags);
        $this->assertType( 'array', $val );
        $this->assertTrue( $val['valid'] );
        $this->assertFalse( isset($val['reason']) );
        $this->assertFalse( isset($val['location']) );
    }

    public function testGeometry_isSimple()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertTrue( $g->isSimple() );

        $g = $reader->read('LINESTRING(0 0, 10 0)');
        $this->assertTrue( $g->isSimple() );

        $g = $reader->read('LINESTRING(0 0, 10 0, 5 5, 5 -5)');
        $this->assertFalse( $g->isSimple() );
    }

    public function testGeometry_isRing()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertFalse( $g->isRing() );

        $g = $reader->read('LINESTRING(0 0, 10 0, 5 5, 5 -5)');
        $this->assertFalse( $g->isRing() );

        $g = $reader->read('LINESTRING(0 0, 10 0, 5 5, 0 0)');
        $this->assertTrue( $g->isRing() );
    }

    public function testGeometry_hasZ()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertFalse( $g->hasZ() );

        $g = $reader->read('POINT(0 0 0)');
        $this->assertTrue( $g->hasZ() );

    }

    public function testGeometry_isClosed()
    {
        if (!method_exists(GEOSGeometry::class, 'isClosed')) {
            return;
        }

        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        try  {
            $this->assertFalse( $g->isClosed() );
            $this->assertTrue(FALSE);
        } catch (Exception $e) {
            $this->assertContains('LineString', $e->getMessage());
        }

        $g = $reader->read('LINESTRING(0 0, 10 0, 5 5, 5 -5)');
        $this->assertFalse( $g->isClosed() );

        $g = $reader->read('LINESTRING(0 0, 10 0, 5 5, 0 0)');
        $this->assertTrue( $g->isClosed() );
    }

    public function testGeometry_type()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertEquals('Point', $g->typeName());
        $this->assertEquals(GEOS_POINT, $g->typeId());

        $g = $reader->read('MULTIPOINT (0 1, 2 3)');
        $this->assertEquals('MultiPoint', $g->typeName());
        $this->assertEquals(GEOS_MULTIPOINT, $g->typeId());

        $g = $reader->read('LINESTRING (0 0, 2 3)');
        $this->assertEquals('LineString', $g->typeName());
        $this->assertEquals(GEOS_LINESTRING, $g->typeId());

        $g = $reader->read('MULTILINESTRING ((0 1, 2 3), (10 10, 3 4))');
        $this->assertEquals('MultiLineString', $g->typeName());
        $this->assertEquals(GEOS_MULTILINESTRING, $g->typeId());

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertEquals('Polygon', $g->typeName());
        $this->assertEquals(GEOS_POLYGON, $g->typeId());

        $g = $reader->read('MULTIPOLYGON (((0 0, 1 0, 1 1, 0 1, 0 0)), ((10 10, 10 14, 14 14, 14 10, 10 10), (11 11, 11 12, 12 12, 12 11, 11 11)))');
        $this->assertEquals('MultiPolygon', $g->typeName());
        $this->assertEquals(GEOS_MULTIPOLYGON, $g->typeId());

        $g = $reader->read('GEOMETRYCOLLECTION (MULTIPOLYGON (((0 0, 1 0, 1 1, 0 1, 0 0)), ((10 10, 10 14, 14 14, 14 10, 10 10), (11 11, 11 12, 12 12, 12 11, 11 11))), POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0)), MULTILINESTRING ((0 0, 2 3), (10 10, 3 4)), LINESTRING (0 0, 2 3), MULTIPOINT (0 0, 2 3), POINT (9 0))');
        $this->assertEquals('GeometryCollection', $g->typeName());
        $this->assertEquals(GEOS_GEOMETRYCOLLECTION, $g->typeId());
    }

    public function testGeometry_srid()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertEquals(0, $g->getSRID());
        $g->setSRID(2);
        $this->assertEquals(2, $g->getSRID());
    }

    public function testGeometry_numGeometries()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertEquals(1, $g->numGeometries());

        $g = $reader->read('MULTIPOINT (0 1, 2 3)');
        $this->assertEquals(2, $g->numGeometries());

        $g = $reader->read('LINESTRING (0 0, 2 3)');
        $this->assertEquals(1, $g->numGeometries());

        $g = $reader->read('MULTILINESTRING ((0 1, 2 3), (10 10, 3 4))');
        $this->assertEquals(2, $g->numGeometries());

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertEquals(1, $g->numGeometries());

        $g = $reader->read('MULTIPOLYGON (
            ((0 0, 1 0, 1 1, 0 1, 0 0)),
            ((10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11)))');
        $this->assertEquals(2, $g->numGeometries());

        $g = $reader->read('GEOMETRYCOLLECTION (
            MULTIPOLYGON (
                ((0 0, 1 0, 1 1, 0 1, 0 0)),
                ((10 10, 10 14, 14 14, 14 10, 10 10),
                    (11 11, 11 12, 12 12, 12 11, 11 11))
            ),
            POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0)),
            MULTILINESTRING ((0 0, 2 3), (10 10, 3 4)),
            LINESTRING (0 0, 2 3),
            MULTIPOINT (0 0, 2 3),
            POINT (9 0))');
        $this->assertEquals(6, $g->numGeometries());
    }

    public function testGeometry_geometryN()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $c = $g->geometryN(0);
        $this->assertTrue( $g->equalsExact($c) );

        $g = $reader->read('MULTIPOINT (0 1, 2 3)');
        $this->assertEquals($reader->read('POINT(0 1)'), $g->geometryN(0));
        $this->assertEquals($reader->read('POINT(2 3)'), $g->geometryN(1));

        $c = $g->geometryN(2);
        $this->assertNull( $c );

    }

    public function testGeometry_numInteriorRings()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertEquals(0, $g->numInteriorRings());

        $g = $reader->read('POLYGON (
            (10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11))');
        $this->assertEquals(1, $g->numInteriorRings());

        $g = $reader->read('POLYGON (
            (10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11),
                (13 11, 13 12, 13.5 12, 13.5 11, 13 11))');
        $this->assertEquals(2, $g->numInteriorRings());

        $g = $reader->read('POINT (0 0)');
        try {
            $g->numInteriorRings();
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'Polygon', $e->getMessage() );
        }

    }

    public function testGeometry_numPoints()
    {
        if (!method_exists(GEOSGeometry::class, 'numPoints')) {
            return;
        }

        $reader = new GEOSWKTReader();

        $g = $reader->read('LINESTRING (0 0, 1 0, 1 1, 0 1)');
        $this->assertEquals(4, $g->numPoints());

        $g = $reader->read('POINT (0 0)');
        try {
            $g->numPoints();
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'LineString', $e->getMessage() );
        }

    }

    public function testGeometry_getXY()
    {
        if (!method_exists(GEOSGeometry::class, 'getX')) {
            return;
        }

        if (!method_exists(GEOSGeometry::class, 'getY')) {
            return;
        }

        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT (1 2)');
        $this->assertEquals(1, $g->getX());
        $this->assertEquals(2, $g->getY());

        $g = $reader->read('LINESTRING (0 0, 1 1)');
        try {
            $g->getX();
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'Point', $e->getMessage() );
        }

        try {
            $g->getY();
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'Point', $e->getMessage() );
        }

    }

    public function testGeometry_interiorRingN()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POLYGON (
            (10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11))');
        $r = $g->interiorRingN(0);
        $this->assertEquals('LINEARRING (11 11, 11 12, 12 12, 12 11, 11 11)',
            $writer->write($r) );

        $g = $reader->read('POLYGON (
            (10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11),
                (13 11, 13 12, 13.5 12, 13.5 11, 13 11))');
        $r = $g->interiorRingN(0);
        $this->assertEquals('LINEARRING (11 11, 11 12, 12 12, 12 11, 11 11)',
            $writer->write($r) );
        $r = $g->interiorRingN(1);
        $this->assertEquals('LINEARRING (13 11, 13 12, 14 12, 14 11, 13 11)',
            $writer->write($r) );

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertNull($g->interiorRingN(0));

        $g = $reader->read('POINT (0 0)');
        try {
            $g->interiorRingN(0);
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'Polygon', $e->getMessage() );
        }

    }

    public function testGeometry_exteriorRing()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POLYGON (
            (10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11))');
        $r = $g->exteriorRing();
        $this->assertEquals('LINEARRING (10 10, 10 14, 14 14, 14 10, 10 10)',
            $writer->write($r) );

        $g = $reader->read('POINT (0 0)');
        try {
            $g->exteriorRing(0);
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'Polygon', $e->getMessage() );
        }

    }

    public function testGeometry_numCoordinates()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertEquals(1, $g->numCoordinates());

        $g = $reader->read('MULTIPOINT (0 1, 2 3)');
        $this->assertEquals(2, $g->numCoordinates());

        $g = $reader->read('LINESTRING (0 0, 2 3)');
        $this->assertEquals(2, $g->numCoordinates());

        $g = $reader->read('MULTILINESTRING ((0 1, 2 3), (10 10, 3 4))');
        $this->assertEquals(4, $g->numCoordinates());

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertEquals(5, $g->numCoordinates());

        $g = $reader->read('MULTIPOLYGON (
            ((0 0, 1 0, 1 1, 0 1, 0 0)),
            ((10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11)))');
        $this->assertEquals(15, $g->numCoordinates());

        $g = $reader->read('GEOMETRYCOLLECTION (
            MULTIPOLYGON (
                ((0 0, 1 0, 1 1, 0 1, 0 0)),
                ((10 10, 10 14, 14 14, 14 10, 10 10),
                    (11 11, 11 12, 12 12, 12 11, 11 11))
            ),
            POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0)),
            MULTILINESTRING ((0 0, 2 3), (10 10, 3 4)),
            LINESTRING (0 0, 2 3),
            MULTIPOINT (0 0, 2 3),
            POINT (9 0))');
        $this->assertEquals(29, $g->numCoordinates());
    }

    public function testGeometry_dimension()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertEquals(0, $g->dimension());

        $g = $reader->read('MULTIPOINT (0 1, 2 3)');
        $this->assertEquals(0, $g->dimension());

        $g = $reader->read('LINESTRING (0 0, 2 3)');
        $this->assertEquals(1, $g->dimension());

        $g = $reader->read('MULTILINESTRING ((0 1, 2 3), (10 10, 3 4))');
        $this->assertEquals(1, $g->dimension());

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertEquals(2, $g->dimension());

        $g = $reader->read('MULTIPOLYGON (
            ((0 0, 1 0, 1 1, 0 1, 0 0)),
            ((10 10, 10 14, 14 14, 14 10, 10 10),
                (11 11, 11 12, 12 12, 12 11, 11 11)))');
        $this->assertEquals(2, $g->dimension());

        $g = $reader->read('GEOMETRYCOLLECTION (
            MULTIPOLYGON (
                ((0 0, 1 0, 1 1, 0 1, 0 0)),
                ((10 10, 10 14, 14 14, 14 10, 10 10),
                    (11 11, 11 12, 12 12, 12 11, 11 11))
            ),
            POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0)),
            MULTILINESTRING ((0 0, 2 3), (10 10, 3 4)),
            LINESTRING (0 0, 2 3),
            MULTIPOINT (0 0, 2 3),
            POINT (9 0))');
        $this->assertEquals(2, $g->dimension());
    }

    public function testGeometry_coordinateDimension()
    {
        if (!method_exists(GEOSGeometry::class, 'coordinateDimension')) {
            return;
        }

        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(0 0)');
        $this->assertEquals(2, $g->coordinateDimension());

        $g = $reader->read('POINT(0 0 0)');
        $this->assertEquals(3, $g->coordinateDimension());

    }

    public function testGeometry_pointN()
    {
        if (!method_exists(GEOSGeometry::class, 'pointN')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();
        $writer->setRoundingPrecision(0);

        $g = $reader->read('LINESTRING (10 10, 10 14, 14 14, 14 10)');
        $this->assertEquals('POINT (10 10)', $writer->write($g->pointN(0)) );
        $this->assertEquals('POINT (10 14)', $writer->write($g->pointN(1)) );
        $this->assertEquals('POINT (14 14)', $writer->write($g->pointN(2)) );
        $this->assertEquals('POINT (14 10)', $writer->write($g->pointN(3)) );
        $this->assertNull( $g->pointN(4) );

        $g = $reader->read('LINEARRING (11 11, 11 12, 12 11, 11 11)');
        $this->assertEquals('POINT (11 11)', $writer->write($g->pointN(0)) );
        $this->assertEquals('POINT (11 12)', $writer->write($g->pointN(1)) );
        $this->assertEquals('POINT (12 11)', $writer->write($g->pointN(2)) );
        $this->assertEquals('POINT (11 11)', $writer->write($g->pointN(3)) );

        $g = $reader->read('POINT (0 0)');
        try {
            $g->pointN(0);
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'LineString', $e->getMessage() );
        }

    }

    public function testGeometry_startendPoint()
    {
        if (!method_exists(GEOSGeometry::class, 'startPoint')) {
            return;
        }

        if (!method_exists(GEOSGeometry::class, 'endPoint')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('LINESTRING (10 10, 10 14, 14 14, 14 10)');
        $this->assertEquals('POINT (10 10)', $writer->write($g->startPoint()) );
        $this->assertEquals('POINT (14 10)', $writer->write($g->endPoint()) );
        $this->assertNull( $g->pointN(4) );

        $g = $reader->read('LINEARRING (11 11, 11 12, 12 11, 11 11)');
        $this->assertEquals('POINT (11 11)', $writer->write($g->startPoint()) );
        $this->assertEquals('POINT (11 11)', $writer->write($g->endPoint()) );

        $g = $reader->read('POINT (0 0)');
        try {
            $g->pointN(0);
            $this->assertTrue( FALSE );
        } catch (Exception $e) {
            $this->assertContains( 'LineString', $e->getMessage() );
        }

    }

    public function testGeometry_area()
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();

        if (method_exists(GEOSWKTWriter::class, 'setRoundingPrecision')) {
            $writer->setRoundingPrecision(0);
        }

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertEquals( 1.0, $g->area() );

        $g = $reader->read('POINT (0 0)');
        $this->assertEquals( 0.0, $g->area() );

        $g = $reader->read('LINESTRING (0 0 , 10 0)');
        $this->assertEquals( 0.0, $g->area() );

    }

    public function testGeometry_length()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertEquals( 4.0, $g->length() );

        $g = $reader->read('POINT (0 0)');
        $this->assertEquals( 0.0, $g->length() );

        $g = $reader->read('LINESTRING (0 0 , 10 0)');
        $this->assertEquals( 10.0, $g->length() );

    }

    public function testGeometry_distance()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');

        $g2 = $reader->read('POINT(0.5 0.5)');
        $this->assertEquals( 0.0, $g->distance($g2) );

        $g2 = $reader->read('POINT (-1 0)');
        $this->assertEquals( 1.0, $g->distance($g2) );

        $g2 = $reader->read('LINESTRING (3 0 , 10 0)');
        $this->assertEquals( 2.0, $g->distance($g2) );

    }

    public function testGeometry_hausdorffDistance()
    {
        $reader = new GEOSWKTReader();

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');

        $g2 = $reader->read('POINT(0 10)');
        $this->assertEquals( 10.0, round($g->hausdorffDistance($g2)) );

        $g2 = $reader->read('POINT (-1 0)');
        $this->assertEquals( 2.0, round($g->hausdorffDistance($g2)) );

        $g2 = $reader->read('LINESTRING (3 0 , 10 0)');
        $this->assertEquals( 9.0, round($g->hausdorffDistance($g2)) );

    }

    public function testGeometry_delaunayTriangulation()
    {
        if (!method_exists(GEOSGeometry::class, 'delaunayTriangulation')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();
        $writer->setRoundingPrecision(0);

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');

        $b = $g->delaunayTriangulation();
        $this->assertEquals('GEOMETRYCOLLECTION (POLYGON ((0 1, 0 0, 1 0, 0 1)), POLYGON ((0 1, 1 0, 1 1, 0 1)))', $writer->write($b));

        $b = $g->delaunayTriangulation(0,true);
        $this->assertEquals('MULTILINESTRING ((0 1, 1 1), (0 0, 0 1), (0 0, 1 0), (1 0, 1 1), (0 1, 1 0))', $writer->write($b));

    }

    public function testGeometry_voronoiDiagram()
    {
        if (!method_exists(GEOSGeometry::class, 'voronoiDiagram')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();
        $writer->setRoundingPrecision(0);

        $g = $reader->read('MULTIPOINT(0 0, 100 0, 100 100, 0 100)');

        $b = $g->voronoiDiagram();
        $this->assertEquals('GEOMETRYCOLLECTION (POLYGON ((50 50, 50 200, 200 200, 200 50, 50 50)), POLYGON ((50 -100, 50 50, 200 50, 200 -100, 50 -100)), POLYGON ((-100 50, -100 200, 50 200, 50 50, -100 50)), POLYGON ((-100 -100, -100 50, 50 50, 50 -100, -100 -100)))', $writer->write($b->normalize()));

        $b = $g->voronoiDiagram(0, 1);
        $this->assertEquals('MULTILINESTRING ((50 50, 200 50), (50 50, 50 200), (50 -100, 50 50), (-100 50, 50 50))', $writer->write($b->normalize()));

        $b = $g->voronoiDiagram(0, 1, $g->buffer(1000));
        $this->assertEquals('MULTILINESTRING ((50 50, 1100 50), (50 50, 50 1100), (50 -1000, 50 50), (-1000 50, 50 50))', $writer->write($b->normalize()));

    }

    public function testGeometry_snapTo()
    {
        if (!method_exists(GEOSGeometry::class, 'snapTo')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();
        $writer->setTrim(true);

        $g = $reader->read('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))');

        $g2 = $reader->read('POINT(0.1 0)');

        $snapped = $g->snapTo($g2, 0);
        $this->assertEquals('POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))', $writer->write($snapped));

        $snapped = $g->snapTo($g2, 0.5);
        $this->assertEquals('POLYGON ((0.1 0, 1 0, 1 1, 0 1, 0.1 0))', $writer->write($snapped));
    }

    public function testGeometry_node()
    {
        if (!method_exists(GEOSGeometry::class, 'node')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $writer = new GEOSWKTWriter();
        $writer->setTrim(true);

        $g = $reader->read('LINESTRING(0 0, 10 0, 5 -5, 5 5)');

        $noded = $g->node();
        $this->assertEquals('MULTILINESTRING ((0 0, 5 0), (5 0, 10 0, 5 -5, 5 0), (5 0, 5 5))', $writer->write($noded));

    }
}

GeometryTest::run();

?>
--EXPECT--
