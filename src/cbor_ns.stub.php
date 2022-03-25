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

interface Serializable
{
    public function cborSerialize(): mixed;
}

final class Undefined implements \JsonSerializable
{
    private function __construct() {}
    public static function __set_state(array $properties): object {}

    public static function get(): Undefined {}
    //public function __clone(): self {}
    //public function __toBool(): false {}
    public function jsonSerialize(): mixed {}
}

/** @internal */
abstract class XString implements \JsonSerializable
{
    public string $value;

    public function __construct(string $value) {}
    public function jsonSerialize(): mixed {}
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
abstract class FloatX implements \JsonSerializable
{
    //public float $value;

    public function __construct(float $value) {}
    public static function fromBinary(string $value): FloatX {}
    public function jsonSerialize(): mixed {}
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
    public const CBOR_TAG_DATETIME = 0;
    public const CBOR_TAG_BIGNUM_U = 2;
    public const CBOR_TAG_BIGNUM_N = 3;
    public const CBOR_TAG_DECIMAL = 4;
    / tag constants end */

    public int $tag;
    public mixed $content;

    public function __construct(int $tag, mixed $content) {}
}

final class Shareable implements \JsonSerializable
{
    public mixed $value;

    public function __construct(mixed $value) {}
    public function jsonSerialize(): mixed {}
}
