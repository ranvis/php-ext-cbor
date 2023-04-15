<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 * @generate-class-entries
 */

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
    /*//
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

    /*//
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
    public static function __set_state(array $properties): object {}

    /*//
     * Retrieve the undefined object.
     * @return Undefined The undefined object.
     */
    public static function get(): Undefined {}

    //public function __clone() {}
    //public function __toBool(): false {}
    public function jsonSerialize(): mixed {}
}

/** @internal */
abstract class XString implements \JsonSerializable
{
    //public string $value;

    /*//
     * @param string $value
     */
    public function __construct(string $value) {}

    public static function __set_state(array $properties): XString {}
    public function __unserialize(array $data): void {}
    public function jsonSerialize(): mixed {}
    //public function __toString(): string {}
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
    //public float $value;

    /*//
     * @param float $value
     */
    public function __construct(float $value) {}

    /*//
     * Create float value from binary data.
     * @param string $value A binary value of binary16/binary32.
     * @return FloatX
     */
    public static function fromBinary(string $value): FloatX {}

    public static function __set_state(array $properties): FloatX {}
    public function __unserialize(array $data): void {}
    public function jsonSerialize(): mixed {}

    /*//
     * Convert value to binary data.
     * @return string A binary value of binary16/binary32.
     */
    public function toBinary(): string {}
}

/**
 * CBOR Half-precision (16-bit) float value
 */
final class Float16 extends FloatX
{
    //public function __toFloat(): float {}
}

/**
 * CBOR Single-precision (32-bit) float value
 *
 * The storage type on encoding can be changed by CBOR_CDE flag, but not by CBOR_FLOAT16 flag.
 */
final class Float32 extends FloatX
{
    //public function __toFloat(): float {}
}

/**
 * CBOR Tagged data
 */
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
    //public const MIME_MSG = 36;
    /* tag constants end */

    public int $tag;
    public mixed $content;

    /*//
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

    /*//
     * @param mixed $value The value to be shared
     */
    public function __construct(mixed $value) {}
    public function jsonSerialize(): mixed {}
}

/**
 * CBOR Decoder
 * @not-serializable
 */
class Decoder
{
    /*//
     * Create CBOR decoder instance.
     * @see cbor_decode()
     * @param int $flags Configuration flags
     * @param array|null $options Configuration options
     */
    public function __construct(int $flags = CBOR_BYTE | CBOR_KEY_BYTE, ?array $options = null) {}

    /*//
     * Decode CBOR data item string.
     * @param string $data A data item string to decode
     * @return mixed The decoded value
     * @throws Cbor\Exception
     */
    public function decode(string $data): mixed {}

    /*//
     * Append data to decoding buffer.
     * @param string $data A part of data item string to decode
     * @param int $offset Offset of the string to start decoding from.
     * A negative value means offset from the end of the string.
     * @param ?int $length Length of the string from the offset
     * A negative value means offset from the end of the string.
     * @return void
     */
    public function add(string $data, int $offset = 0, ?int $length = null): void {}

    /*//
     * Decode data in the buffer.
     *
     * Call getValue() to retrieve decoded value.
     * The value is replaced on next process() call.
     * @return bool True if a decoded item is ready
     * @throws Cbor\Exception
     */
    public function process(): bool {}

    /*//
     * Reset the decoder state and free up buffer memory.
     * @return void
     */
    public function reset(): void {}

    /*//
     * Get the decoded value state.
     * @return bool True if a decoded item is ready
     */
    public function hasValue(): bool {}

    /*//
     * Get the decoded data item.
     *
     * You should test the return value of add() or hasValue() before calling this method.
     * @param bool $clear True to clear the value afterwards
     * @return mixed The decoded value.
     */
    public function getValue(bool $clear = true): mixed {}

    /*//
     * Check if the data is partially decoded.
     * @return bool True if the decoder is holding incomplete value.
     */
    public function isPartial(): bool {}

    /*//
     * Check if the decoding is on the way
     * @return bool True if the decoder process() is in progress
     */
    public function isProcessing(): bool {}

    /*//
     * Get data in the decoding buffer.
     * @return string A copy of the unprocessed data
     */
    public function getBuffer(): string {}
}
