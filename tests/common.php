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
        $value = toDump($this->value);
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
            $exp = toDump($exp);
            $act = toDump($act);
        }
        if ($exp === $act) {
            return true;
        }
        if (!$exported) {
            $exp = toDump($exp);
            $act = toDump($act);
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
    } catch (Throwable $e) {
        printf("Exception thrown: %s(%d):%s %s\n",
            get_class($e),
            $e->getCode(),
            getErrorName($e),
            $e->getMessage(),
        );
        echo "  " . str_replace("\n", "\n  ", $e->getTraceAsString()), "\n";
        echo "Stopped.\n";
        return;
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
    return hex2bin(preg_replace('/[][,{}: ()h"]/', '', $value));
}

function toDump(mixed $value): string
{
    if (!is_string($value) || preg_match('/\A[\n\x20-\x7e]*\z/', $value)) {
        $str = var_export($value, true);
        // var_dump() is not used because of having object ID etc.
        $str = preg_replace_callback('/[^\n\x20-\x7e]/', fn ($m) => '\' . "\\x' . bin2hex($m[0]) . '" . \'', $str);
        $str = str_replace('" . \'\' . "', '', $str);
        return $str;
    }
    return '0x' . bin2hex($value);
}

function xThrows(int|string $code, callable $fn): void
{
    try {
        $actual = toDump($fn());
    } catch (Cbor\Exception $e) {
        $actual = getErrorName($e);
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

function getErrorName(int|Throwable $code): string
{
    if (is_object($code)) {
        $e = $code;
        $code = $code->getCode();
        if (!$e instanceof Cbor\Exception) {
            return "#$code";
        }
    }
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
