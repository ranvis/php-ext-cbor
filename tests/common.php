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

function eq($exp, $act, int $depth = 0): void
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
    $bt = debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS, $depth + 1)[$depth];
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
    return toDump($decoded);
}

function toDump($value): string
{
    if (!is_string($value)) {
        return var_export($value, true);
    }
    return '0x' . bin2hex($value);
}

function xThrows(int $code, callable $fn): void
{
    try {
        $actual = toDump($fn());
    } catch (Cbor\Exception $e) {
        $actual = getErrorName($e->getCode());
    }
    eq(getErrorName($code), $actual, 2);
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

function getErrorName(int $code): string
{
    $errors = [
        /* error names start */
          1 => 'CBOR_ERROR_INVALID_FLAGS',
               'CBOR_ERROR_INVALID_OPTIONS',
               'CBOR_ERROR_DEPTH',
               'CBOR_ERROR_RECURSION',
               'CBOR_ERROR_SYNTAX',
               'CBOR_ERROR_UTF8',
         17 => 'CBOR_ERROR_UNSUPPORTED_TYPE',
               'CBOR_ERROR_UNSUPPORTED_VALUE',
               'CBOR_ERROR_UNSUPPORTED_SIZE',
         25 => 'CBOR_ERROR_UNSUPPORTED_KEY_TYPE',
               'CBOR_ERROR_UNSUPPORTED_KEY_VALUE',
               'CBOR_ERROR_UNSUPPORTED_KEY_SIZE',
         33 => 'CBOR_ERROR_TRUNCATED_DATA',
               'CBOR_ERROR_MALFORMED_DATA',
               'CBOR_ERROR_EXTRANEOUS_DATA',
         41 => 'CBOR_ERROR_TAG_SYNTAX',
               'CBOR_ERROR_TAG_TYPE',
               'CBOR_ERROR_TAG_VALUE',
        255 => 'CBOR_ERROR_INTERNAL',
        /* error names end */
    ];
    return $errors[$code] ?? "#$code";
}
