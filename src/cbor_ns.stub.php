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

final class EncodeParams
{
    public mixed $value;
    public array $params;

    public function __construct(mixed $value, array $params) {}
}

final class Undefined implements \JsonSerializable
{
    private function __construct() {}
    public function __destruct() {}
    public static function __set_state(array $properties): object {}

    public static function get(): Undefined {}
    //public function __clone(): self {}
    //public function __toBool(): false {}
    public function jsonSerialize(): mixed {}
}

/** @internal */
abstract class XString implements \JsonSerializable
{
    //public string $value;

    public function __construct(string $value) {}
    public static function __set_state(array $properties): XString {}
    public function __unserialize(array $data): void {}
    public function jsonSerialize(): mixed {}
    //public function __toString(): string {}
}

final class Byte extends XString
{
}

final class Text extends XString
{
}

/** @internal */
abstract class FloatX implements \JsonSerializable
{
    //public float $value;

    public function __construct(float $value) {}
    public static function fromBinary(string $value): FloatX {}
    public static function __set_state(array $properties): FloatX {}
    public function __unserialize(array $data): void {}
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
    /* tag constants start */
    //public const SELF_DESCRIBE = 55799;
    //public const STRING_REF_NS = 256;
    //public const STRING_REF = 25;
    //public const SHAREABLE = 28;
    //public const SHARED_REF = 29;
    //public const DATETIME = 0;
    //public const EPOCH = 1;
    //public const BIGNUM_U = 2;
    //public const BIGNUM_N = 3;
    //public const DECIMAL = 4;
    //public const BIGFLOAT = 5;
    //public const TO_BASE64URL = 21;
    //public const TO_BASE64 = 22;
    //public const TO_HEX = 23;
    //public const CBOR_DATA = 24;
    //public const URI = 32;
    //public const BASE64URL = 33;
    //public const BASE64 = 34;
    //public const PCRE_REGEX = 35;
    //public const MIME = 36;
    /* tag constants end */

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
