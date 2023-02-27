--TEST--
module info
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    ob_start();
    ok(phpinfo());
    $info = ob_get_clean();
    ok(str_contains($info, 'CBOR support'));
});

?>
--EXPECT--
Done.
