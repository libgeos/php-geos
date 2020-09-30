--TEST--
WKTReader tests
--SKIPIF--
<?php if (!extension_loaded('geos')) { print "geos extension not loaded\n"; exit(1); } ?>
--FILE--
<?php

require './tests/TestHelper.php';

class WKTReaderTest extends GEOSTest
{
    public function testWKTReader__construct()
    {
        $reader = new GEOSWKTReader();
        $this->assertNotNull($reader);
    }

    public function testWKTReader_read()
    {
        $reader = new GEOSWKTReader();

        /* Good WKT */
        $geom = $reader->read('POINT(0 0)');
        $this->assertNotNull($geom);
        $geom = $reader->read('POINT(0 0 0)');
        $this->assertNotNull($geom);
        $geom = $reader->read('POINT Z (0 0 0)');
        $this->assertNotNull($geom);
        $geom = $reader->read('POINT EMPTY');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTIPOINT(0 0 1, 2 3 4)');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTIPOINT Z (0 0 1, 2 3 4)');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTIPOINT((0 0), (2 3))');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTIPOINT EMPTY');
        $this->assertNotNull($geom);
        $geom = $reader->read('LINESTRING(0 0 1, 2 3 4)');
        $this->assertNotNull($geom);
        $geom = $reader->read('LINESTRING EMPTY');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTILINESTRING((0 0 1, 2 3 4),
                                               (10 10 2, 3 4 5))');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTILINESTRING Z ((0 0 1, 2 3 4),
                                               (10 10 2, 3 4 5))');
        $this->assertNotNull($geom);
        $geom = $reader->read('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))');
        $this->assertNotNull($geom);
        $geom = $reader->read('POLYGON EMPTY');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTIPOLYGON(
                                ((0 0, 1 0, 1 1, 0 1, 0 0)),
                                ((10 10, 10 14, 14 14, 14 10, 10 10),
                                    (11 11, 11 12, 12 12, 12 11, 11 11))
                               )');
        $this->assertNotNull($geom);
        $geom = $reader->read('MULTIPOLYGON EMPTY');
        $this->assertNotNull($geom);
        $geom = $reader->read('GEOMETRYCOLLECTION(
                MULTIPOLYGON(
                 ((0 0, 1 0, 1 1, 0 1, 0 0)),
                 ((10 10, 10 14, 14 14, 14 10, 10 10),
                  (11 11, 11 12, 12 12, 12 11, 11 11))
                ),
                POLYGON((0 0, 1 0, 1 1, 0 1, 0 0)),
                MULTILINESTRING((0 0, 2 3), (10 10, 3 4)),
                LINESTRING(0 0, 2 3),
                MULTIPOINT(0 0, 2 3),
                POINT(9 0)
        )');
        $this->assertNotNull($geom);
        $geom = $reader->read('GEOMETRYCOLLECTION EMPTY');
        $this->assertNotNull($geom);
    }

    public function testBogusWKT()
    {
        $reader = new GEOSWKTReader();

        /* BOGUS WKT */
        try {
            $reader->read("MULTIDOT(0 1 2 3)");
            $this->assertTrue(FALSE); # this is just to fail if we get here
        } catch (Exception $e) {
            $this->assertContains('ParseException', $e->getMessage());
        }
    }

    public function testNoArgumentsToRead()
    {
        $reader = new GEOSWKTReader();

        /* BOGUS call (#448) */
        try {
            $reader->read();
            $this->assertTrue(FALSE); # this is just to fail if we get here
        } catch (Exception $e) {
            $this->assertContains('expects exactly 1 parameter',
                                  $e->getMessage());
        } catch (Error $e) {
            $this->assertContains('expects exactly 1', $e->getMessage());
        }
    }
}

WKTReaderTest::run();

?>
--EXPECT--
WKTReaderTest->testWKTReader__construct	OK
WKTReaderTest->testWKTReader_read	OK
WKTReaderTest->testBogusWKT	OK
WKTReaderTest->testNoArgumentsToRead	OK
