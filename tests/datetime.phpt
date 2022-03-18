--TEST--
datetime tag
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // RFC 8949 example
    $dt = new DateTimeImmutable('2013-03-21T20:04:00Z');
    eq('0xc074323031332d30332d32315432303a30343a30305a', cenc($dt));

    $dt = new DateTimeImmutable('@981173106');
    eq('0xc074' . bin2hex('2001-02-03T04:05:06Z'), cenc($dt));
    $dt = DateTime::createFromImmutable($dt);
    $dt->setTime(4, 5, 6, 7);
    eq('0xc0781b' . bin2hex('2001-02-03T04:05:06.000007Z'), cenc($dt));
    $dt->setTime(4, 5, 6, 700_000);
    eq('0xc076' . bin2hex('2001-02-03T04:05:06.7Z'), cenc($dt));
    $dt->setTimeZone(new DateTimeZone('+0130'));
    eq('0xc0781b' . bin2hex('2001-02-03T05:35:06.7+01:30'), cenc($dt));

    cencThrows(CBOR_ERROR_UNSUPPORTED_TYPE, $dt, options: ['datetime' => false]);
});

?>
--EXPECT--
Done.
