--TEST--
WKBWriter tests
--SKIPIF--
<?php if (!extension_loaded('geos')) { print "geos extension not loaded\n"; exit(1); } ?>
--FILE--
<?php

require './tests/TestHelper.php';

class WKBWriterTest extends GEOSTest
{
    public function testWKBWriter__construct()
    {
        $writer = new GEOSWKBWriter();
        $this->assertNotNull($writer);
    }

    public function testWKBWriter_getOutputDimension()
    {
        $writer = new GEOSWKBWriter();
        $this->assertEquals(2, $writer->getOutputDimension());
    }

    public function testWKBWriter_setOutputDimension()
    {
        $writer = new GEOSWKBWriter();
        $writer->setOutputDimension(3);
        $this->assertEquals(3, $writer->getOutputDimension());
        $writer->setOutputDimension(2);
        $this->assertEquals(2, $writer->getOutputDimension());

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

    public function testWKBWriter_getsetByteOrder()
    {
        $writer = new GEOSWKBWriter();

        /* Machine-dependent */
        $bo = $writer->getByteOrder();

        $obo = $bo ? 0 : 1;
        $writer->setByteOrder($obo);
        $this->assertEquals($obo, $writer->getByteOrder());

        # Anything different from 0 (BIG_ENDIAN) or 1 (LITTLE_ENDIAN)
        # is invalid
        try {
            $writer->setByteOrder(5);
            $this->assertTrue(FALSE);
        } catch (Exception $e) {
            $this->assertContains('LITTLE (1) or BIG (0)', $e->getMessage());
        }
    }

    public function testWKBWriter_getsetIncludeSRID()
    {
        $writer = new GEOSWKBWriter();

        $this->assertEquals(FALSE, $writer->getIncludeSRID());
        $writer->setIncludeSRID(TRUE);
        $this->assertEquals(TRUE, $writer->getIncludeSRID());
        $writer->setIncludeSRID(FALSE);
        $this->assertEquals(FALSE, $writer->getIncludeSRID());
    }

    /**
     * @dataProvider providerWKBWriter_write
     *
     * @param integer $byteOrder       The byte order: 0 for BIG endian, 1 for LITTLE endian.
     * @param integer $inputDimension  The input dimension: 2 or 3.
     * @param integer $outputDimension The output dimension: 2 or 3.
     * @param boolean $includeSrid     Whether to include the SRID in the output.
     * @param string  $wkb             The expected HEX WKB output.
     */
    public function runWKBWriter_write($byteOrder, $inputDimension, $outputDimension, $includeSrid, $wkb)
    {
        $reader = new GEOSWKTReader();
        $writer = new GEOSWKBWriter();

        $writer->setByteOrder($byteOrder);
        $writer->setOutputDimension($outputDimension);
        $writer->setIncludeSRID($includeSrid);

        if ($inputDimension === 3) {
            $g = $reader->read('POINT(6 7 8)');
            $g->setSRID(53);
        } else {
            $g = $reader->read('POINT(6 7)');
            $g->setSRID(43);
        }

        $this->assertEquals(hex2bin($wkb), $writer->write($g));
        $this->assertEquals($wkb, $writer->writeHEX($g));
    }

    public function testWKBWriter_write()
    {
        // 2D input
        $this->runWKBWriter_write(1, 2, 2, false, '010100000000000000000018400000000000001C40');        // 2D LITTLE endian
        $this->runWKBWriter_write(1, 2, 2, true, '01010000202B00000000000000000018400000000000001C40'); // 2D LITTLE endian + SRID
        $this->runWKBWriter_write(0, 2, 2, false, '00000000014018000000000000401C000000000000');        // 2D BIG endian
        $this->runWKBWriter_write(0, 2, 2, true, '00200000010000002B4018000000000000401C000000000000'); // 2D BIG endian + SRID
        $this->runWKBWriter_write(1, 2, 3, false, '010100000000000000000018400000000000001C40');        // 3D LITTLE endian
        $this->runWKBWriter_write(1, 2, 3, true, '01010000202B00000000000000000018400000000000001C40'); // 3D LITTLE endian + SRID
        $this->runWKBWriter_write(0, 2, 3, false, '00000000014018000000000000401C000000000000');        // 3D BIG endian
        $this->runWKBWriter_write(0, 2, 3, true, '00200000010000002B4018000000000000401C000000000000'); // 3D BIG endian + SRID

        // 3D input
        $this->runWKBWriter_write(1, 3, 2, false, '010100000000000000000018400000000000001C40');                        // 2D LITTLE endian
        $this->runWKBWriter_write(1, 3, 2, true, '01010000203500000000000000000018400000000000001C40');                 // 2D LITTLE endian + SRID
        $this->runWKBWriter_write(0, 3, 2, false, '00000000014018000000000000401C000000000000');                        // 2D BIG endian
        $this->runWKBWriter_write(0, 3, 2, true, '0020000001000000354018000000000000401C000000000000');                 // 2D BIG endian + SRID
        $this->runWKBWriter_write(1, 3, 3, false, '010100008000000000000018400000000000001C400000000000002040');        // 3D LITTLE endian
        $this->runWKBWriter_write(1, 3, 3, true, '01010000A03500000000000000000018400000000000001C400000000000002040'); // 3D LITTLE endian + SRID
        $this->runWKBWriter_write(0, 3, 3, false, '00800000014018000000000000401C0000000000004020000000000000');        // 3D BIG endian
        $this->runWKBWriter_write(0, 3, 3, true, '00A0000001000000354018000000000000401C0000000000004020000000000000'); // 3D BIG endian + SRID
    }

    public function testInvalidWriteThrowsException()
    {
        $writer = new GEOSWKBWriter();

        try {
            $writer->write(1);
            $this->assertTrue(false);
        } catch (ErrorException $e) {
            $this->assertContains('expects parameter 1 to be object, int', $e->getMessage());
        } catch (Error $e) {
            $this->assertContains('Argument #1', $e->getMessage());
        }
    }

    public function testInvalidWriteHEXThrowsException()
    {
        $writer = new GEOSWKBWriter();

        try {
            $writer->writeHEX(1);
            $this->assertTrue(false);
        } catch (ErrorException $e) {
            $this->assertContains('expects parameter 1 to be object, int', $e->getMessage());
        } catch (Error $e) {
            $this->assertContains('Argument #1', $e->getMessage());
	    }
    }
}

WKBWriterTest::run();

?>
--EXPECT--
WKBWriterTest->testWKBWriter__construct	OK
WKBWriterTest->testWKBWriter_getOutputDimension	OK
WKBWriterTest->testWKBWriter_setOutputDimension	OK
WKBWriterTest->testWKBWriter_getsetByteOrder	OK
WKBWriterTest->testWKBWriter_getsetIncludeSRID	OK
WKBWriterTest->testWKBWriter_write	OK
WKBWriterTest->testInvalidWriteThrowsException	OK
WKBWriterTest->testInvalidWriteHEXThrowsException	OK
