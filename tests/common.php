<?php

declare(strict_types=1);

final class TestStats
{
    public static $v;

    public static function reset(): void
    {
        self::$v = array_fill_keys(explode(' ',
            'encode decode assertCompare assertThrows'
        ), 0);
    }

    public static function inc($name): void
    {
        TestStats::$v[$name]++;
    }

    public static function show(): void
    {
        echo "Stats:\n";
        foreach (TestStats::$v as $name => $count) {
            echo "  $name: $count\n";
        }
    }
}

function run($func): void
{
    TestStats::reset();
    $func();
    //TestStats::show();
    echo "Done.\n";
}

function eq($exp, $act): void
{
    TestStats::inc('assertCompare');
    $exported = !is_scalar($exp) || !is_scalar($act);
    if ($exported) {
        $exp = var_export($exp, true);
        $act = var_export($act, true);
    }
    if ($exp === $act) {
        return;
    }
    if (!$exported) {
        $exp = var_export($exp, true);
        $act = var_export($act, true);
    }
    $bt = debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS, 1)[0];
    echo "Expected: " . $exp . "\n";
    echo "Actual  : " . $act . "\n";
    echo "File    : " . $bt['file'] . "\n";
    echo "Line    : " . $bt['line'] . "\n";
    echo "\n";
}

function cborEncode(...$args)
{
    TestStats::inc('encode');
    return cbor_encode(...$args);
}

function cborDecode(...$args)
{
    TestStats::inc('decode');
    return cbor_decode(...$args);
}

function cenc(...$args): string
{
    return '0x' . bin2hex(cborEncode(...$args));
}

function cdec($value, ...$args): mixed
{
    return cborDecode(hex2bin($value), ...$args);
}

function cdecHex($value, ...$args): mixed
{
    $decoded = cborDecode(hex2bin($value), ...$args);
    if (!is_string($decoded)) {
        return var_export($decoded, true);
    }
    return '0x' . bin2hex($decoded);
}

function cdecExport($value, ...$args): string
{
    $decoded = cborDecode(hex2bin($value), ...$args);
    return var_export($decoded, true);
}

function xThrows(int $code, callable $fn): void
{
    $actual = false;
    try {
        $fn();
    } catch (Cbor\Exception $e) {
        $actual = $e->getCode();
    }
    eq($code, $actual);
}

function cencThrows(int $code, mixed $value, ...$args): void
{
    TestStats::inc('assertThrows');
    xThrows($code, fn () => cborEncode($value, ...$args));
}

function cdecThrows(int $code, string $value, ...$args): void
{
    TestStats::inc('assertThrows');
    xThrows($code, fn () => cborDecode(hex2bin($value), ...$args));
}
