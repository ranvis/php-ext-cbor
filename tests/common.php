<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

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

interface TesterInterface
{
    public function test(): bool|string;
}

class BoolTester implements TesterInterface
{
    public function __construct(
        public mixed $value,
    ) {
    }

    public function test(): bool|string
    {
        if ($this->value) {
            return true;
        }
        $value = var_export($value, true);
        return "Value   : " . $value . "\n";
    }
}

class EqTester implements TesterInterface
{
    public function __construct(
        private mixed $exp,
        private mixed $act,
    ) {
    }

    public function test(): bool|string
    {
        $exp = $this->exp;
        $act = $this->act;
        $exported = !is_string($exp) || !is_string($act);
        if ($exported) {
            $exp = var_export($exp, true);
            $act = var_export($act, true);
        }
        if ($exp === $act) {
            return true;
        }
        if (!$exported) {
            $exp = var_export($exp, true);
            $act = var_export($act, true);
        }
        return "Expected: " . $exp . "\n"
            . "Actual  : " . $act . "\n";
    }
}

class RegExTester implements TesterInterface
{
    public function __construct(
        private string $exp,
        private string $act,
    ) {
    }

    public function test(): bool|string
    {
        if (preg_match($this->exp, $this->act)) {
            return true;
        }
        return sprintf("Expected: regex %s\nActual  : %s\n", $this->exp, $this->act);
    }
}

function run($func): void
{
    TestStats::reset();
    try {
        $func();
    } catch (Cbor\Exception $e) {
        printf("Exception thrown: %s(%d):%s %s\n",
            get_class($e),
            $e->getCode(),
            getErrorName($e->getCode()),
            $e->getMessage(),
        );
        throw $e;
    }
    //TestStats::show();
    echo "Done.\n";
}

function ok(mixed $value, int $depth = 0): void
{
    xTest(new BoolTester($value), $depth + 1);
}

function eq(mixed $exp, mixed $act, int $depth = 0): void
{
    xTest(new EqTester($exp, $act), $depth + 1);
}

function eqRegEx(string $exp, string $act, int $depth = 0): void
{
    xTest(new RegExTester($exp, $act), $depth + 1);
}

function xTest(TesterInterface $tester, int $depth = 0): void
{
    TestStats::inc('assertCompare');
    $result = $tester->test();
    if ($result === true) {
        return;
    }
    if ($result === false) {
        throw new LogicException($tester::class . ' should return an error message instead of false when test is failed.');  // true return type: PHP>=8.2
    }
    echo $result;
    trace($depth + 1);
}

function trace(int $depth = 0)
{
    $bt = debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS, $depth + 1)[$depth];
    static $phptMap = [];
    $phpt = $bt['file'] . 't';
    if (isset($phptMap[$phpt])) {
        if (is_int($phptMap[$phpt])) {
            $bt['line'] += $phptMap[$phpt];
            $bt['file'] = $phpt;
        }
    } else if (file_exists($phpt)) {
        $script = file_get_contents($phpt) ?: '';
        $pos = strpos($script, "\n--FILE--\n");  // LF only
        if ($pos !== false) {
            $phptMap[$phpt] = substr_count($script, "\n", 0, $pos) + 2;
            $bt['line'] += $phptMap[$phpt];
            $bt['file'] = $phpt;
        } else {
            $phptMap[$phpt] = false;
        }
    } else {
        $phptMap[$phpt] = false;
    }
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
    return cborDecode(decodeHex($value), ...$args);
}

function cdecHex($value, ...$args): mixed
{
    $decoded = cborDecode(decodeHex($value), ...$args);
    return toDump($decoded);
}

function decodeHex(string $value): string
{
    return hex2bin($value);
}

function toDump($value): string
{
    if (!is_string($value)) {
        return var_export($value, true);
    }
    return '0x' . bin2hex($value);
}

function xThrows(int|string $code, callable $fn): void
{
    try {
        $actual = toDump($fn());
    } catch (Cbor\Exception $e) {
        $actual = getErrorName($e->getCode());
    } catch (Throwable $e) {
        $actual = get_class($e);
        if (is_int($code) && $e instanceof Error) {
            $actual .= ': ' . $e->getMessage();
        } elseif (!is_string($code) || str_contains($code, '#')) {
            $actual .= '#' . $e->getCode();
        }
    }
    eq(is_int($code) ? getErrorName($code) : $code, $actual, 2);
}

function cencThrows(int|string $code, mixed $value, ...$args): void
{
    TestStats::inc('assertThrows');
    xThrows($code, fn () => cborEncode($value, ...$args));
}

function cdecThrows(int|string $code, string $value, ...$args): void
{
    TestStats::inc('assertThrows');
    xThrows($code, fn () => cborDecode(decodeHex($value), ...$args));
}

function throws(string $exception, callable $fn): void
{
    TestStats::inc('assertThrows');
    xThrows($exception, $fn);
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
               'CBOR_ERROR_DUPLICATE_KEY',
         33 => 'CBOR_ERROR_TRUNCATED_DATA',
               'CBOR_ERROR_MALFORMED_DATA',
               'CBOR_ERROR_EXTRANEOUS_DATA',
         41 => 'CBOR_ERROR_TAG_SYNTAX',
               'CBOR_ERROR_TAG_TYPE',
               'CBOR_ERROR_TAG_VALUE',
        241 => 'CBOR_ERROR_INTERNAL',
        /* error names end */
    ];
    return $errors[$code] ?? "#$code";
}
