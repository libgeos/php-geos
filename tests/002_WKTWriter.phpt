--TEST--
WKTWriter tests
--SKIPIF--
<?php if (!extension_loaded('geos')) { print "geos extension not loaded\n"; exit(1); } ?>
--FILE--
<?php

require './tests/TestHelper.php';

class WKTWriterTest extends GEOSTest
{
    public function testWKTWriter__construct()
    {
        $writer = new GEOSWKTWriter();
        $this->assertNotNull($writer);
    }

    public function testWKTWriter_write()
    {
        $writer = new GEOSWKTWriter();
        $reader = new GEOSWKTReader();

        try {
            $writer->write(1);
            $this->assertTrue(FALSE); # this is just to fail if we get here
        } catch (Exception $e) {
            $this->assertContains('expects parameter 1', $e->getMessage());
        } catch (Error $e) {
            $this->assertContains('Argument #1', $e->getMessage());
        }

        $g = $reader->read('POINT(6 7)');

        $this->assertEquals('POINT (6.0000000000000000 7.0000000000000000)',
            $writer->write($g));
    }

    public function testWKTWriter_setTrim()
    {
        if (!method_exists("GEOSWKTWriter", 'setTrim')) {
            return;
        }

        $writer = new GEOSWKTWriter();
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(6 7)');
        $this->assertNotNull($g);

        $writer->setTrim(TRUE);
        $this->assertEquals('POINT (6 7)',
            $writer->write($g));

        $writer->setTrim(FALSE);
        $this->assertEquals('POINT (6.0000000000000000 7.0000000000000000)',
            $writer->write($g));

    }

    public function testWKT_roundTrip()
    {
        $r = new GEOSWKTReader();
        $w = new GEOSWKTWriter();

        if (method_exists("GEOSWKTWriter", 'setTrim')) {
            $w->setTrim(TRUE);
        }

        $in[] = 'POINT (0 0)';
        $in[] = 'POINT EMPTY';
        $in[] = 'MULTIPOINT (0 1, 2 3)';
        $in[] = 'MULTIPOINT EMPTY';
        $in[] = 'LINESTRING (0 0, 2 3)';
        $in[] = 'LINESTRING EMPTY';
        $in[] = 'MULTILINESTRING ((0 1, 2 3), (10 10, 3 4))';
        $in[] = 'MULTILINESTRING EMPTY';
        $in[] = 'POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))';
        $in[] = 'POLYGON EMPTY';
        $in[] = 'MULTIPOLYGON (((0 0, 1 0, 1 1, 0 1, 0 0)), ((10 10, 10 14, 14 14, 14 10, 10 10), (11 11, 11 12, 12 12, 12 11, 11 11)))';
        $in[] = 'MULTIPOLYGON EMPTY';
        $in[] = 'GEOMETRYCOLLECTION (MULTIPOLYGON (((0 0, 1 0, 1 1, 0 1, 0 0)), ((10 10, 10 14, 14 14, 14 10, 10 10), (11 11, 11 12, 12 12, 12 11, 11 11))), POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0)), MULTILINESTRING ((0 0, 2 3), (10 10, 3 4)), LINESTRING (0 0, 2 3), MULTIPOINT (0 0, 2 3), POINT (9 0))';
        $in[] = 'GEOMETRYCOLLECTION EMPTY';

        foreach ($in as $i) {
            $this->assertEquals($i, $w->write($r->read($i)));
        }

    }

    public function testWKTWriter_setRoundingPrecision()
    {
        if (!method_exists("GEOSWKTWriter", 'setRoundingPrecision')) {
            return;
        }

        $writer = new GEOSWKTWriter();
        $reader = new GEOSWKTReader();

        $g = $reader->read('POINT(6.123456 7.123456)');

        $this->assertEquals('POINT (6.1234560000000000 7.1234560000000000)',
            $writer->write($g));

        $writer->setRoundingPrecision(2);
        $this->assertEquals('POINT (6.12 7.12)', $writer->write($g));

        $writer->setRoundingPrecision(5); /* rounds */
        $this->assertEquals('POINT (6.12346 7.12346)', $writer->write($g));

        $writer->setRoundingPrecision(1);
        $this->assertEquals('POINT (6.1 7.1)', $writer->write($g));

        $writer->setRoundingPrecision(0);
        $this->assertEquals('POINT (6 7)', $writer->write($g));

    }

    public function testWKTWriter_getOutputDimension()
    {
        if (!method_exists("GEOSWKTWriter", 'getOutputDimension')) {
            return;
        }

        $writer = new GEOSWKTWriter();
        $this->assertEquals(2, $writer->getOutputDimension());
    }

    public function testWKTWriter_setOutputDimension()
    {
        if (!method_exists("GEOSWKTWriter", 'setOutputDimension')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $g3d = $reader->read('POINT(1 2 3)');
        $g2d = $reader->read('POINT(3 2)');

        $writer = new GEOSWKTWriter();
        $writer->setTrim(TRUE);

        # Only 2d by default
        $this->assertEquals('POINT (1 2)', $writer->write($g3d));

        # 3d if requested _and_ available
        $writer->setOutputDimension(3);
        $this->assertEquals('POINT Z (1 2 3)', $writer->write($g3d));
        $this->assertEquals('POINT (3 2)', $writer->write($g2d));

        # 1 is invalid
        try {
            $writer->setOutputDimension(1);
            $this->assertTrue(FALSE);
        } catch (Exception $e) {
            $this->assertContains('must be 2 or 3', $e->getMessage());
        }

        # 4 is invalid
        try {
            $writer->setOutputDimension(4);
            $this->assertTrue(FALSE);
        } catch (Exception $e) {
            $this->assertContains('must be 2 or 3', $e->getMessage());
        }

    }

    public function testWKTWriter_setOld3D()
    {
        if (!method_exists("GEOSWKTWriter", 'setOld3D')) {
            return;
        }

        $reader = new GEOSWKTReader();
        $g3d = $reader->read('POINT(1 2 3)');

        $writer = new GEOSWKTWriter();
        $writer->setTrim(TRUE);

        # New 3d WKT by default
        $writer->setOutputDimension(3);
        $this->assertEquals('POINT Z (1 2 3)', $writer->write($g3d));

        # Switch to old
        $writer->setOld3D(TRUE);
        $this->assertEquals('POINT (1 2 3)', $writer->write($g3d));

        # Old3d flag is not reset when changing dimensions
        $writer->setOutputDimension(2);
        $this->assertEquals('POINT (1 2)', $writer->write($g3d));
        $writer->setOutputDimension(3);
        $this->assertEquals('POINT (1 2 3)', $writer->write($g3d));

        # Likewise, dimensions spec is not reset when changing old3d flag
        $writer->setOld3D(FALSE);
        $this->assertEquals('POINT Z (1 2 3)', $writer->write($g3d));

    }
}

WKTWriterTest::run();

?>
--EXPECT--
WKTWriterTest->testWKTWriter__construct	OK
WKTWriterTest->testWKTWriter_write	OK
WKTWriterTest->testWKTWriter_setTrim	OK
WKTWriterTest->testWKT_roundTrip	OK
WKTWriterTest->testWKTWriter_setRoundingPrecision	OK
WKTWriterTest->testWKTWriter_getOutputDimension	OK
WKTWriterTest->testWKTWriter_setOutputDimension	OK
WKTWriterTest->testWKTWriter_setOld3D	OK
