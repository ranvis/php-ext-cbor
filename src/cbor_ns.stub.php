<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 * @generate-class-entries
 */

namespace Cbor;

// as of writing, generator doesn't support partially namespaced script; i.e. mix global functions and namespace X { ... }

class Exception extends \Exception
{
}

// EXPERIMENTAL: TODO:
interface Encodable
{
    public function cborEncode(): mixed;
}

final class Undefined
{
    private function __construct() {}
    public static function __set_state(array $properties): object {}

    public static function get(): Undefined {}
    //public function __clone(): self {}
    //public function __toBool(): false {}
}

/** @internal */
abstract class XString
{
    public string $value;

    public function __construct(string $value) {}
}

final class Byte extends XString
{
    //public function __toString(): string {}
}

final class Text extends XString
{
    //public function __toString(): string {}
}

/** @internal */
abstract class FloatX
{
    public float $value;

    public function __construct(float $value) {}
}

final class Float16 extends FloatX
{
}

final class Float32 extends FloatX
{
}

final class Tag
{
    public const SELF_DESCRIBE = 55799;

    public int $tag;
    public mixed $data;

    public function __construct(int $tag, mixed $data) {}
}
