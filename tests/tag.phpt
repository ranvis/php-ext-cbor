--TEST--
tags
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
	eq(new Cbor\Tag(38, ['en', 'Hello']), cdec('d8268262656E6548656C6C6F', CBOR_TEXT));
});

?>
--EXPECT--
Done.
