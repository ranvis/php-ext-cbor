--TEST--
Decode to diagnostic notation
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $data = '9f404201026443424f52438090a001827f657374726561646d696e67fffa47c35000a1f97e00f4bf82f6d5826161bf61626163fff5fff8f0ff';
    $diag = '[_ h\'\', h\'0102\', "CBOR", h\'8090a0\', 1, [(_ "strea", "ming"), 100000.0_2], {NaN_1: false}, {_ [null, 21(["a", {_ "b": "c"}])]: true}, simple(240)]';
    eq($diag, cdec($data, CBOR_EDN));
    eq(str_replace(' ', '', $diag), cdec($data, CBOR_EDN, ['space' => false]));
    $diag = <<<'END'
        [_
          h'',
          h'0102',
          "CBOR",
          h'8090a0',
          1,
          [
            (_
              "strea",
              "ming"
            ),
            100000.0_2
          ],
          {
            NaN_1: false
          },
          {_
            [
              null,
              21([
                "a",
                {_
                  "b": "c"
                }
              ])
            ]: true
          },
          simple(240)
        ]
        END;
    eq($diag, cdec($data, CBOR_EDN, ['indent' => 2]));
    eq(str_replace(' ', '', str_replace('  ', "\t", $diag)), cdec($data, CBOR_EDN, ['indent' => "\t", 'space' => false]));
    eq(str_replace('  ', '', $diag), cdec($data, CBOR_EDN, ['indent' => 0]));
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['indent' => 30]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['space' => 4]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['indent' => ">"]);

    // byte_space, byte_wrap
    $data = '581c00000000000000001111111111111111222222222222222233333333';
    eq('h\'00000000000000001111111111111111222222222222222233333333\'', cdec($data, CBOR_EDN));
    eq('h\'00 00 00 00 00 00 00 00 11 11 11 11 11 11 11 11 22 22 22 22 22 22 22 22 33 33 33 33\'', cdec($data, CBOR_EDN, ['byte_space' => 1]));
    eq('h\'00000000 00000000  11111111 11111111  22222222 22222222  33333333\'', cdec($data, CBOR_EDN, ['byte_space' => 4 | 8]));
    eq('h\'0000000000000000 1111111111111111\'
h\'2222222222222222 33333333\'', cdec($data, CBOR_EDN, ['byte_space' => 8, 'byte_wrap' => 16, 'indent' => 2]));
    eq('h\'00000000000000\' h\'00111111111111\' h\'11112222222222\' h\'22222233333333\'', cdec($data, CBOR_EDN, ['byte_space' => 8, 'byte_wrap' => 7]));
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['byte_space' => false]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['byte_wrap' => 0]);

    // self-describe flag
    $data = 'd9d9f7' . bin2hex(chr(0xf6));
    eq('null', cdec($data, CBOR_EDN));
    eq('55799(null)', cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE));
    $data = 'd9d9f7480001020304050607';
    eq('55799(h\'0001020304050607\')', cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE, ['indent' => 2]));
    eq('55799(h\'0001020304050607\')', cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE, ['indent' => 2, 'byte_wrap' => 8]));
    eq(<<<'END'
        55799(
          h'00010203'
          h'04050607'
        )
        END, cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE, ['indent' => 2, 'byte_wrap' => 4]));

    // text escaping
    $data = "\u{202e}efas\u{202c}";
    $data = bin2hex(chr(0x60 + strlen($data)) . $data);
    eq('"\u202eefas\u202c"', cdec($data, CBOR_EDN));
});

?>
--EXPECT--
Done.
