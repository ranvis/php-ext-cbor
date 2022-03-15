<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 * @generate-class-entries
 */

namespace Cbor;

// as of writing, code generator doesn't support partially namespaced script; i.e. mix global functions and namespace X { ... }

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
    //public function __toFloat(): float {}
}

final class Float32 extends FloatX
{
    //public function __toFloat(): float {}
}

final class Tag
{
    /* tag constants start /
    public const CBOR_TAG_SELF_DESCRIBE = 55799;
    public const CBOR_TAG_STRING_REF_NS = 256;
    public const CBOR_TAG_STRING_REF = 25;
    public const CBOR_TAG_SHAREABLE = 28;
    public const CBOR_TAG_SHARED_REF = 29;
    / tag constants end */

    public int $tag;
    public mixed $content;

    public function __construct(int $tag, mixed $content) {}
}
