<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

/* classes start */
namespace Cbor;

/**
 * Exception thrown on CBOR handling.
 */
class Exception extends \Exception
{
}

/**
 * Interface for object to CBOR serialization.
 */
interface Serializable
{
    /**
     * Serialize object to CBOR-ready value.
     * @return mixed The value of object in serializable structure.
     */
    public function cborSerialize(): mixed;
}

/**
 * Instructs encoder to use the specified parameters.
 */
final class EncodeParams
{
    public mixed $value;
    public array $params;

    /**
     * @param mixed $value
     * @param array $params
     */
    public function __construct(mixed $value, array $params) {}
}

/**
 * CBOR undefined value
 */
final class Undefined implements \JsonSerializable
{
    private function __construct() {}
    public function __destruct() {}
    public static function __set_state(array $properties): object {}

    /**
     * Retrieve the undefined object.
     * @return Undefined The undefined object.
     */
    public static function get(): Undefined {}

    public function __clone(): self {}
    public function __toBool(): false {}
    public function jsonSerialize(): mixed {}
}

/** @internal */
abstract class XString implements \JsonSerializable
{
    public string $value;

    /**
     * @param string $value
     */
    public function __construct(string $value) {}

    public static function __set_state(array $properties): XString {}
    public function __unserialize(array $data): void {}
    public function jsonSerialize(): mixed {}
    public function __toString(): string {}
}

/**
 * CBOR Binary string
 */
final class Byte extends XString
{
}

/**
 * CBOR UTF-8 encoded string
 */
final class Text extends XString
{
}

/** @internal */
abstract class FloatX implements \JsonSerializable
{
    public float $value;

    /**
     * @param float $value
     */
    public function __construct(float $value) {}

    /**
     * Create float value from binary data.
     * @param string $value A binary value
     * @return FloatX
     */
    public static function fromBinary(string $value): FloatX {}

    public static function __set_state(array $properties): FloatX {}
    public function __unserialize(array $data): void {}
    public function jsonSerialize(): mixed {}
}

/**
 * CBOR Half-precision (16-bit) float value
 */
final class Float16 extends FloatX
{
    public function __toFloat(): float {}
}

/**
 * CBOR Single-precision (32-bit) float value
 */
final class Float32 extends FloatX
{
    public function __toFloat(): float {}
}

/**
 * CBOR Tagged data
 */
final class Tag
{
    /* tag constants start */
    public const SELF_DESCRIBE = 55799;
    public const STRING_REF_NS = 256;
    public const STRING_REF = 25;
    public const SHAREABLE = 28;
    public const SHARED_REF = 29;
    public const DATETIME = 0;
    public const EPOCH = 1;
    public const BIGNUM_U = 2;
    public const BIGNUM_N = 3;
    public const DECIMAL = 4;
    public const BIGFLOAT = 5;
    public const TO_BASE64URL = 21;
    public const TO_BASE64 = 22;
    public const TO_HEX = 23;
    public const CBOR_DATA = 24;
    public const URI = 32;
    public const BASE64URL = 33;
    public const BASE64 = 34;
    public const PCRE_REGEX = 35;
    public const MIME_MSG = 36;
    /* tag constants end */

    public int $tag;
    public mixed $content;

    /**
     * @param int $tag A tag number
     * @param mixed $content
     */
    public function __construct(int $tag, mixed $content) {}
}

/**
 * CBOR Pass-by-reference wrapper
 */
final class Shareable implements \JsonSerializable
{
    public mixed $value;

    /**
     * @param mixed $value The value to be shared
     */
    public function __construct(mixed $value) {}
    public function jsonSerialize(): mixed {}
}
/* classes end */
