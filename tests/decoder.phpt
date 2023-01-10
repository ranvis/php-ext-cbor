--TEST--
decoder
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $decoder = new Cbor\Decoder();
    // decode()
    eq(0, $decoder->decode(hex2bin('00')));
    xThrows(CBOR_ERROR_EXTRANEOUS_DATA, fn () => $decoder->decode(hex2bin('0001')));
    // initial state
    eq(false, $decoder->isPartial());
    eq('', $decoder->getBuffer());
    eq(false, $decoder->hasValue());
    throws(Exception::class, fn () => $decoder->getValue());

    // empty data
    $decoder->add('');
    eq(false, $decoder->process());
    eq(false, $decoder->isPartial());

    // sequence
    $decoder->add(hex2bin('0001'));
    eq(true, $decoder->process());
    eq(true, $decoder->hasValue());
    eq(false, $decoder->isPartial());
    eq(hex2bin('01'), $decoder->getBuffer());
    eq(true, $decoder->hasValue());
    eq(0, $decoder->getValue());
    eq(false, $decoder->hasValue());
    eq(true, $decoder->process());
    eq('', $decoder->getBuffer());
    eq(1, $decoder->getValue(false));
    eq(true, $decoder->hasValue());
    eq(1, $decoder->getValue());
    eq(false, $decoder->hasValue());
    throws(Exception::class, fn () => $decoder->getValue());

    // splitted
    $input = str_split(hex2bin('8301820203820405'), 3);
    $decoder->add($input[0]);
    eq(bin2hex($input[0]), bin2hex($decoder->getBuffer()));
    eq(false, $decoder->process());
    eq('', bin2hex($decoder->getBuffer()));
    eq(false, $decoder->hasValue());
    eq(true, $decoder->isPartial());
    eq(false, $decoder->isProcessing());
    $decoder->add($input[1]);
    eq(true, $decoder->isPartial());
    eq(false, $decoder->process());
    $decoder->add($input[2]);
    eq(true, $decoder->process());
    eq(true, $decoder->hasValue());
    eq(false, $decoder->isPartial());
    eq('', bin2hex($decoder->getBuffer()));
    eq([1, [2, 3], [4, 5]], $decoder->getValue());
    eq(false, $decoder->hasValue());

    // reset
    $decoder->add(hex2bin('0001'));
    eq(true, $decoder->process());
    $decoder->reset();
    eq(false, $decoder->hasValue());
    eq(false, $decoder->isPartial());
    eq('', $decoder->getBuffer());
    $decoder->add(hex2bin('44303132'));
    eq(false, $decoder->process());
    eq('44303132', bin2hex($decoder->getBuffer()));  // string must be processed as a whole at present
    eq(true, $decoder->isPartial());
    $decoder->reset();
    eq('', $decoder->getBuffer());
    eq(false, $decoder->isPartial());

    // prohibit reentrance
    // ...for now no PHP function may be called during decoding
    // eq(true, $decoder->isProcessing());

    throws(Exception::class, fn () => serialize($decoder));
    throws(Error::class, fn () => clone $decoder);

    // large data
    $decoder->reset();
    eq(false, $decoder->hasValue());
    $s128k = str_repeat('0123456789abcdef', (1024 / 16) * 128);
    $cborByteStr = pack('CN', 0x5a, strlen($s128k)) . $s128k;
    $decoder->add(chr(0x9f)); // indef-length array
    $decoder->add($cborByteStr);
    $decoder->process();
    $decoder->add($cborByteStr);
    $decoder->process();
    for ($i = 0; $i < 8; $i++) {
        $decoder->add($cborByteStr);
    }
    $decoder->process();
    $decoder->add($cborByteStr);
    $decoder->add(chr(0xff)); // break
    eq(false, $decoder->hasValue());
    $decoder->process();
    eq(true, $decoder->hasValue());
    eq(false, $decoder->isPartial());
    $result = $decoder->getValue();
    ok(is_array($result));
    foreach ($result as $value) {
        ok($s128k === $value);
    }
});

?>
--EXPECT--
Done.
