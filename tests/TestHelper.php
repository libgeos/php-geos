<?php

# hex2bin for PHP < 5.4
# https://gist.github.com/mcrumley/5672621
if (!function_exists('hex2bin')) {
    function hex2bin($str) {
        $map = array(
            '00'=>"\x00", '10'=>"\x10", '20'=>"\x20", '30'=>"\x30", '40'=>"\x40", '50'=>"\x50", '60'=>"\x60", '70'=>"\x70",
            '01'=>"\x01", '11'=>"\x11", '21'=>"\x21", '31'=>"\x31", '41'=>"\x41", '51'=>"\x51", '61'=>"\x61", '71'=>"\x71",
            '02'=>"\x02", '12'=>"\x12", '22'=>"\x22", '32'=>"\x32", '42'=>"\x42", '52'=>"\x52", '62'=>"\x62", '72'=>"\x72",
            '03'=>"\x03", '13'=>"\x13", '23'=>"\x23", '33'=>"\x33", '43'=>"\x43", '53'=>"\x53", '63'=>"\x63", '73'=>"\x73",
            '04'=>"\x04", '14'=>"\x14", '24'=>"\x24", '34'=>"\x34", '44'=>"\x44", '54'=>"\x54", '64'=>"\x64", '74'=>"\x74",
            '05'=>"\x05", '15'=>"\x15", '25'=>"\x25", '35'=>"\x35", '45'=>"\x45", '55'=>"\x55", '65'=>"\x65", '75'=>"\x75",
            '06'=>"\x06", '16'=>"\x16", '26'=>"\x26", '36'=>"\x36", '46'=>"\x46", '56'=>"\x56", '66'=>"\x66", '76'=>"\x76",
            '07'=>"\x07", '17'=>"\x17", '27'=>"\x27", '37'=>"\x37", '47'=>"\x47", '57'=>"\x57", '67'=>"\x67", '77'=>"\x77",
            '08'=>"\x08", '18'=>"\x18", '28'=>"\x28", '38'=>"\x38", '48'=>"\x48", '58'=>"\x58", '68'=>"\x68", '78'=>"\x78",
            '09'=>"\x09", '19'=>"\x19", '29'=>"\x29", '39'=>"\x39", '49'=>"\x49", '59'=>"\x59", '69'=>"\x69", '79'=>"\x79",
            '0a'=>"\x0a", '1a'=>"\x1a", '2a'=>"\x2a", '3a'=>"\x3a", '4a'=>"\x4a", '5a'=>"\x5a", '6a'=>"\x6a", '7a'=>"\x7a",
            '0b'=>"\x0b", '1b'=>"\x1b", '2b'=>"\x2b", '3b'=>"\x3b", '4b'=>"\x4b", '5b'=>"\x5b", '6b'=>"\x6b", '7b'=>"\x7b",
            '0c'=>"\x0c", '1c'=>"\x1c", '2c'=>"\x2c", '3c'=>"\x3c", '4c'=>"\x4c", '5c'=>"\x5c", '6c'=>"\x6c", '7c'=>"\x7c",
            '0d'=>"\x0d", '1d'=>"\x1d", '2d'=>"\x2d", '3d'=>"\x3d", '4d'=>"\x4d", '5d'=>"\x5d", '6d'=>"\x6d", '7d'=>"\x7d",
            '0e'=>"\x0e", '1e'=>"\x1e", '2e'=>"\x2e", '3e'=>"\x3e", '4e'=>"\x4e", '5e'=>"\x5e", '6e'=>"\x6e", '7e'=>"\x7e",
            '0f'=>"\x0f", '1f'=>"\x1f", '2f'=>"\x2f", '3f'=>"\x3f", '4f'=>"\x4f", '5f'=>"\x5f", '6f'=>"\x6f", '7f'=>"\x7f",

            '80'=>"\x80", '90'=>"\x90", 'a0'=>"\xa0", 'b0'=>"\xb0", 'c0'=>"\xc0", 'd0'=>"\xd0", 'e0'=>"\xe0", 'f0'=>"\xf0",
            '81'=>"\x81", '91'=>"\x91", 'a1'=>"\xa1", 'b1'=>"\xb1", 'c1'=>"\xc1", 'd1'=>"\xd1", 'e1'=>"\xe1", 'f1'=>"\xf1",
            '82'=>"\x82", '92'=>"\x92", 'a2'=>"\xa2", 'b2'=>"\xb2", 'c2'=>"\xc2", 'd2'=>"\xd2", 'e2'=>"\xe2", 'f2'=>"\xf2",
            '83'=>"\x83", '93'=>"\x93", 'a3'=>"\xa3", 'b3'=>"\xb3", 'c3'=>"\xc3", 'd3'=>"\xd3", 'e3'=>"\xe3", 'f3'=>"\xf3",
            '84'=>"\x84", '94'=>"\x94", 'a4'=>"\xa4", 'b4'=>"\xb4", 'c4'=>"\xc4", 'd4'=>"\xd4", 'e4'=>"\xe4", 'f4'=>"\xf4",
            '85'=>"\x85", '95'=>"\x95", 'a5'=>"\xa5", 'b5'=>"\xb5", 'c5'=>"\xc5", 'd5'=>"\xd5", 'e5'=>"\xe5", 'f5'=>"\xf5",
            '86'=>"\x86", '96'=>"\x96", 'a6'=>"\xa6", 'b6'=>"\xb6", 'c6'=>"\xc6", 'd6'=>"\xd6", 'e6'=>"\xe6", 'f6'=>"\xf6",
            '87'=>"\x87", '97'=>"\x97", 'a7'=>"\xa7", 'b7'=>"\xb7", 'c7'=>"\xc7", 'd7'=>"\xd7", 'e7'=>"\xe7", 'f7'=>"\xf7",
            '88'=>"\x88", '98'=>"\x98", 'a8'=>"\xa8", 'b8'=>"\xb8", 'c8'=>"\xc8", 'd8'=>"\xd8", 'e8'=>"\xe8", 'f8'=>"\xf8",
            '89'=>"\x89", '99'=>"\x99", 'a9'=>"\xa9", 'b9'=>"\xb9", 'c9'=>"\xc9", 'd9'=>"\xd9", 'e9'=>"\xe9", 'f9'=>"\xf9",
            '8a'=>"\x8a", '9a'=>"\x9a", 'aa'=>"\xaa", 'ba'=>"\xba", 'ca'=>"\xca", 'da'=>"\xda", 'ea'=>"\xea", 'fa'=>"\xfa",
            '8b'=>"\x8b", '9b'=>"\x9b", 'ab'=>"\xab", 'bb'=>"\xbb", 'cb'=>"\xcb", 'db'=>"\xdb", 'eb'=>"\xeb", 'fb'=>"\xfb",
            '8c'=>"\x8c", '9c'=>"\x9c", 'ac'=>"\xac", 'bc'=>"\xbc", 'cc'=>"\xcc", 'dc'=>"\xdc", 'ec'=>"\xec", 'fc'=>"\xfc",
            '8d'=>"\x8d", '9d'=>"\x9d", 'ad'=>"\xad", 'bd'=>"\xbd", 'cd'=>"\xcd", 'dd'=>"\xdd", 'ed'=>"\xed", 'fd'=>"\xfd",
            '8e'=>"\x8e", '9e'=>"\x9e", 'ae'=>"\xae", 'be'=>"\xbe", 'ce'=>"\xce", 'de'=>"\xde", 'ee'=>"\xee", 'fe'=>"\xfe",
            '8f'=>"\x8f", '9f'=>"\x9f", 'af'=>"\xaf", 'bf'=>"\xbf", 'cf'=>"\xcf", 'df'=>"\xdf", 'ef'=>"\xef", 'ff'=>"\xff",
        );

        $strlen = strlen($str);

        if ($strlen % 2 !== 0) {
            user_error('Hexadecimal input string must have an even length', E_USER_WARNING);
            return false;
        }

        if (strspn($str, '0123456789ABCDEFabcdef') !== $strlen) {
            return false;
        }

        return strtr(strtolower($str), $map);
    }
}

function exception_error_handler($errno, $errstr, $errfile, $errline ) {
    throw new ErrorException($errstr, $errno, 0, $errfile, $errline);
}
set_error_handler("exception_error_handler");

class GEOSTest
{
    static public function run()
    {
        $instance = new static();

        foreach (get_class_methods($instance) as $method) {
            if (strpos($method, 'test') === 0) {
                $class = get_class($instance);

                try {
                    $instance->$method();
                    print "{$class}->{$method}\tOK" . PHP_EOL;
                } catch (Exception $e) {
                    print "{$class}->{$method}\tERROR:" . PHP_EOL;
                    print $e->getMessage() . "\n";
                    debug_print_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);
                    throw $e;
                }
            }
        }
    }

    public function assertContains($expected, $actual)
    {
        if (strpos($actual, $expected) === false) {
            throw new Exception("Expected '{$expected}' to contain '{$actual}'.");
        }
    }

    public function assertEquals($expected, $actual)
    {
        if ($actual != $expected) {
            throw new Exception("Expected {$expected} to equal {$actual}.");
        }
    }

    public function assertNull($actual)
    {
        if (!is_null($actual)) {
            throw new Exception("Expected null.");
        }
    }

    public function assertNotNull($actual)
    {
        if (is_null($actual)) {
            throw new Exception("Expected not null.");
        }
    }

    public function assertTrue($expected)
    {
        if ($expected !== true) {
            throw new Exception("Expected true");
        }
    }

    public function assertFalse($expected)
    {
        if ($expected !== false) {
            throw new Exception("Expected false");
        }
    }

    public function assertType($expectedType, $value)
    {
        $validType = false;

        switch ($expectedType) {
            case 'array':
                $validType = is_array($value);
        }

        if (!$validType) {
            throw new Exception("Expected type '{$expectedType}.'");
        }
    }
}
