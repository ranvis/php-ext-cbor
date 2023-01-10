--TEST--
miscellaneous test
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '');
});

?>
--EXPECT--
Done.
