--TEST--
tags
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq(new Cbor\Tag(38, ['en', 'Hello']), cdec('d8268262656E6548656C6C6F', CBOR_TEXT));

    cdecThrows(CBOR_ERROR_SYNTAX, 'd801ff', 0);
    cencThrows(CBOR_ERROR_SYNTAX, new Cbor\Tag(-1, true));
    eq('0xd819f5', cenc(new Cbor\Tag(Cbor\Tag::STRING_REF, true)));
    eq('0xd81901', cenc(new Cbor\Tag(Cbor\Tag::STRING_REF, 1)));
    cencThrows(CBOR_ERROR_TAG_TYPE, new Cbor\Tag(Cbor\Tag::STRING_REF, true), options: ['string_ref' => true]);
    cencThrows(CBOR_ERROR_TAG_SYNTAX, new Cbor\Tag(Cbor\Tag::STRING_REF, 1), options: ['string_ref' => 'explicit']);
    cencThrows(CBOR_ERROR_TAG_VALUE, new Cbor\Tag(Cbor\Tag::STRING_REF, 1), options: ['string_ref' => true]);
});

?>
--EXPECT--
Done.
