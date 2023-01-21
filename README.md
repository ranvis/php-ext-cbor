# PHP ext/cbor

CBOR codec extension for PHP

This extension makes it possible to encode/decode CBOR data defined in [RFC 8949](https://datatracker.ietf.org/doc/html/rfc8949) on PHP.

This extension itself is not a PHP instance serializer as much as CBOR data format itself is not.

- [License](#license)
- [Installation](#installation)
- [Quick Guide](#quick-guide)
  - [Functions](#functions)
  - [Diagnostic Notation](#diagnostic-notation)
  - [Classes](#classes)
  - [Types of CBOR and PHP](#types-of-cbor-and-php)
- [Supported Tags](#supported-tags)


## License

BSD 2-Clause License


## Installation

```
phpize
./configure --enable-cbor
make install
```

See [Releases](https://github.com/ranvis/php-ext-cbor/releases) for the Windows binaries.


## Quick Guide

### Functions

```php
function cbor_encode(
    mixed $value,
    int $flags = CBOR_BYTE | CBOR_KEY_BYTE,
    ?array $options = null,
): string;

function cbor_decode(
    string $data,
    int $flags = CBOR_BYTE | CBOR_KEY_BYTE,
    ?array $options = null,
): mixed;
```
Encodes to or decodes from a CBOR data item.

```php
try {
    echo bin2hex(cbor_encode(['binary', 1, 2, null])) . "\n";
    // 844662696e6172790102f6
    var_export(cbor_decode(hex2bin('844662696e6172790102f6')));
    // array (
    //  0 => 'binary',
    //  1 => 1,
    //  2 => 2,
    //  3 => NULL,
    // )
} catch (Cbor\Exception $e) {
    echo $e->getMessage();
}
```

When decoding, the CBOR data item must be a single item, or the function will throw an exception with code `CBOR_ERROR_EXTRANEOUS_DATA`.
This means this function cannot decode the CBOR sequences format defined in [RFC 8742](https://datatracker.ietf.org/doc/html/rfc8742).
See `Decoder` class for sequences and progressive decoding.

`$options` array elements are:

- `'max_depth'` (default:`64`; range: `0`..`10000`)
  Maximum number of nesting levels to process.
  To handle arrays/maps/tags, at least 1 depth is required.

- `'max_size'`: (default:`65536`; range: `0`..`0xffffffff`)
  Decode: Maximum number of elements for array and map to be processed.

  Depending on the actual limit set by PHP, the value may be lowered.

- `'offset'` (default:`0`; range: `0`..`PHP_INT_MAX`)
  Decode: Offset of the data to start decoding from. Offset cannot go beyond the length of the data.

- `'length'` (default:`null`; range: `null` | `0`..`PHP_INT_MAX`)
  Decode: Length of the data to decode from the offset. `null` means the whole remaining data. Length cannot go beyond the available length of the data.

  Although you cannot decode an empty string, `0` is valid as an option value.

See "Supported Tags" below for the following options:

- `'string_ref'`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

- `'shared_ref'`:
  - Encode: default: `false`; values: `bool` | `'unsafe_ref'`
  - Decode: default: `true`; values: `bool` | `'shareable'` | `'unsafe_ref'`

- `'datetime'`, `'bignum'`, `'decimal'`:
  - Encode: default: `true`; values: `bool`

Unknown key names are silently ignored.

### Diagnostic Notation

With the decoding flag `CBOR_EDN`, CBOR data (in binary) is decoded to Extended Diagnostic Notation (EDN) `string` defined in [RFC 8610 appendix G](https://datatracker.ietf.org/doc/html/rfc8610#appendix-G). (It does _not_ decode EDN string.)
This can be used to inspect CBOR data if something is wrong with it.

```php
var_dump(cbor_decode(hex2bin('83010243030405'), CBOR_EDN)); // string(17) "[1, 2, h'030405']"
```
`$flags` except `CBOR_SELF_DESCRIBE` and `$options` for decoding are ignored for this mode.

Formatting `$options` may be specified:

- `'indent'` (default: `false`; values: `false` | `0`..`16` | `"\t"`)
  Number of space characters for indentation.
  `false` for no pretty-printing (one-line).
  `"\t"` to use a single tab character.

- `'space'` (default: `true`; values: `bool`)
  Whether to insert a space character after separators to improve readability.

- `'byte_space'` (default: `0`; values: `0`..`63`)
  Add space for every 1/2/4/8/16/32 bytes of `byte string`.
  For example, if `5` (i.e. `1 | 4`) is specified, `h'112233445566...'` will be `h'11 22 33 44  55 66 ...'`.

- `'byte_wrap'` (default: `false`; values: `false` | `1`..`1024`)
  Break down `byte string` into multiple `h'...'` notation for every specified length.

Note that when `CBOR_EDN` is specified, the function will not throw an exception for data error (such as truncated data). It will instead print an error in the returned string as a comment.

Invalid UTF-8 sequences are represented as byte string fragments inside text string, along with consecutive ASCII control characters.

### Classes

#### Serializable

When encoding classes that implement `Cbor\Serializable`, the encoder will call `cborSerialize()`.
Implementors may return data structure to serialize the instance, or throw an Exception to stop serializing.

Although some classes such as `Traversable` and PSR-7 `UriInterface` are serializable by default as described below, `Serializable` can take precedence.

`stdClass` plain object is serialized to `map` by default.

#### EncodeParams

When the encoder encounters a `Cbor\EncodeParams` instance, it encodes the `$value` instance variable with the specified `$params` flags and options added to the current flags and options. After encoding the inner `$value`, those parameters are back to the previous state.
This can be useful when you want to enforce specific parameters partially.

`$params` array elements are:

- `'flags' => int`: Encoding flags to set.
- `'flags_clear' => int`: Encoding flags to clear.
Flags in `'flags_clear'` are cleared first then flags in `'flags'` are set.
Note that you don't need to clear conflicting string flags, i.e. `CBOR_TEXT` is cleared when setting `CBOR_BYTE` and vice versa. The same applies for `CBOR_KEY_*` string flags.
- Other `$options` values for encoding.
You cannot change `'max_depth'` or options that make CBOR data contextual.

Unknown or unsupported key names are silently ignored.

#### Decoder

The class `Cbor\Decoder` can do what `cbor_decode()` does in a more controlled way.

Instantiate `Decoder` with the optional `$flags` and `$options`, then feed CBOR data with `add()` method. `Decoder` will append passed data to the internal buffer.

The `'offset'` and `'length'` options have no effects for this class. You may specify those as parameters of `add()` instead.
The parameters act like those of `substr()` PHP function.

To process data in the buffer, call `process()`. It will return `true` if a data item is decoded. You can test it with `hasValue()` method too. Call `getValue()` to retrieve the decoded value.
If another data item follows (CBOR sequences), call `add()` and/or `process()` again.

`process()` will not return `ture` until a complete item is decoded. Until that, `isPartial()` returns `true` and you need to feed more data to complete the decode.
`getBuffer()` returns a copy of the internal buffer. Note that it doesn't contain data that is partially consumed by `process()`.

**Progressive decoding**

With the class, large data can be decoded progressively without loading the whole data in memory at once:

```php
if (($fp = fopen($filePath, 'rb')) === false) {
    throw new RuntimeException('Cannot open file');
}
$decoder = new Cbor\Decoder();
while (!feof($fp)) {
    $data = fread($fp, 32768);
    if ($data === false) {
        throw new RuntimeException('Cannot read file');
    }
    $decoder->add($data);
    if ($decoder->process()) {
        break;
    }
}
if (!feof($fp) || $decoder->getBuffer() !== '') {
    throw new RuntimeException('Extraneous data.');
}
if (!$decoder->hasValue() || $decoder->isPartial()) {
    throw new RuntimeException('Data is truncated.');
}
fclose($fp);
$value = $decoder->getValue();
var_dump($value);
```

**Decode CBOR sequences**

`Decoder` can decode CBOR sequences which `cbor_decode()` complains:
```php
function cbor_decode_seq(string $data, int $flags = CBOR_BYTE | CBOR_KEY_BYTE, ?array $options = null): Generator
{
    $decoder = new Cbor\Decoder($flags, $options);
    $decoder->add($data);
    while ($decoder->process()) {
        yield $decoder->getValue();
    }
    if ($decoder->isPartial()) {
        throw new Cbor\Exception('Data is truncated.', CBOR_ERROR_TRUNCATED_DATA);
    }
}
```

### Types of CBOR and PHP

#### Integers

CBOR `unsigned integer` and `negative integer` are translated to PHP `int`.
The value must be within the range PHP can handle (`PHP_INT_MIN`..`PHP_INT_MAX`).
This is -2\**63..2\**63-1 on 64-bit PHP, which is narrower than CBOR's -2\**64..2\**64-1.
If decoding data contains out-of-range value, an exception is thrown.

#### Floating-Point Numbers

CBOR `float` has three sizes. 64-bit values are translated to PHP `float`.

32-bit values and 16-bit values are decoded to PHP `Cbor\Float32` and `Cbor\Float16` respectively.
But if the flags `CBOR_FLOAT32` and/or `CBOR_FLOAT16` are passed, they are decoded to PHP `float`.

When encoding PHP `float`, Values are stored with the smallest possible type if a flag is set and no informational loss is expected.

#### Strings

CBOR has two types of strings: `byte string` (binary octets) and `text string` (UTF-8 encoded octets).
PHP `string` type does not have this distinction.

If you specify the `CBOR_BYTE` flag (default) and/or the `CBOR_TEXT` flag on decoding, those strings are decoded to PHP `string`. If the flags are not specified, strings are decoded to `Cbor\Byte` and `Cbor\Text` objects respectively.

On encoding PHP `string`, you must specify either of the flags so that the extension knows to which you want your strings to be encoded. The default is `CBOR_BYTE`.

`CBOR_KEY_BYTE` and `CBOR_KEY_TEXT` are for strings of CBOR `map` keys.

If `text string` is not a valid UTF-8 sequence, an exception is thrown unless you pass `CBOR_UNSAFE_TEXT` flag.

#### Arrays

CBOR `array` is translated to PHP `array`.

If PHP `array` has holes or `string` keys (i.e. the array is not a "list"), it is encoded to CBOR `map`.

#### Maps

CBOR `map` is translated to PHP `stdClass` object.
If the `CBOR_MAP_AS_ARRAY` flag is passed when decoding, it is translated to PHP `array` instead.

Keys must be of CBOR `string` type.

The extension may accept CBOR `integer` keys if the `CBOR_INT_KEY` flag is passed. Likewise with the flag, it will encode PHP `int` key (including integer numeric `string` keys in the range of CBOR `integer`) as CBOR `integer` key.

If the `CBOR_MAP_NO_DUP_KEY` flag is specified on decoding, a duplicated key will throw an exception instead of overriding the former value. This may happen on valid CBOR `map`; e.g. all of unsigned integer `1`, text string `"1"`, and byte string `b'31'` may be the same key for PHP.

#### Tags

CBOR `tag` is translated to PHP `Cbor\Tag(int $tag, mixed $content)` object.

Tag is a marker to mark data (including another tag) as some type using an `unsigned integer`.
You can consult [CBOR tag registry](https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml) for valid tags.

Tags consume one `'max_level'` for each nesting.

Also see "Supported Tags" below.

#### Null and Undefined

CBOR has `null` and `undefined` value, but PHP does not have `undefined` value.

CBOR `undefined` is translated to PHP `Cbor\Undefined` singleton object.
This object is evaluated as `false` in a boolean context. It is not equal to `null`.

```php
$undefined = Cbor\Undefined::get();
var_dump($undefined === clone $undefined); // true
```

#### PHP `Traversable`s

Classes that implement `Traversable` interface are encoded to `map`.

If the class does not implement `Countable` interface, it is encoded to an indefinite-length `map`.

The `CBOR_INT_KEY` flag does not take effect on encoding `Traversable` objects, and the key is encoded according to the actual type.


## Supported Tags

Note: In this section, tag names are written as {tag-name} for clarity.

### tag(55799): Self-Described CBOR

Flag:
- `CBOR_SELF_DESCRIBE`
  - default: `false`

Constants:
- `Cbor\Tag::SELF_DESCRIBE`
- `CBOR_TAG_SELF_DESCRIBE_DATA`

Self-Described CBOR is CBOR data that has this tag for the data.
This 3-byte binary string (magic pattern) can be used to distinguish CBOR from other data including Unicode text encodings. This is useful if data loader need to identify the format by data itself.
```php
$isCbor = str_starts_with($data, CBOR_TAG_SELF_DESCRIBE_DATA);
```

On encoding if the flag is set, the tag is prepended to the encoded data.
On decoding if the flag is _not_ set, the tag is skipped even if one exists. If the flag _is_ set, the tag is _retained_ in the decoded value, meaning you need to test if the root is this tag to extract the real content.

If the tag is to be prepended/skipped, it is handled specially and not counted as a `'max_depth'` level.

### tag(256): stringref-namespace, tag(25): stringref

Option:
- `'string_ref'`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

Constants:
- `Cbor\Tag::STRING_REF_NS`
- `Cbor\Tag::STRING_REF`

The tag {stringref} is like a compression, that "references" the string previously appeared inside {stringref-namespace} tag. Note that it differs from PHP's reference to `string`; i.e. _not_ the concept of `$stringRef = &$string`.

On encode, it can save payload size by replacing the string already seen with the tag + index (or at the worst case increase by 3-bytes overall when single-namespaced).
Note that if smaller payload is desired, it should perform better to apply a data compression instead of this tag.

On decode, the use of tag can save memory of decoded value because PHP can share the identical `string` sequences until one of them is going to be modified (copy-on-write).

For decoding, the option is enabled as `true` by default, while encoding it should be specified explicitly.

If `true` is specified on encoding, data is always wrapped with {stringref-namespace} tag. It initializes the string index table (like compression dictionary) for the content inside the tag.
The {stringref-namespace} tag added implicitly is handled specially and not counted as `'max_depth'` level.
Similarly, `'explicit'` makes {stringref} active but the root namespace is not implicitly created, meaning {stringref} is not created on its own.

The use of this tag makes CBOR contextual.
CBOR data that use {stringref} can be embedded in other CBOR. But data that doesn't use it cannot always be embedded safely in {stringref} CBOR, because it will corrupt reference indices of the following strings.
(As of now, the extension cannot embed raw CBOR data on encoding though.)

Decoders without the support of this tag cannot decode data using {stringref} correctly.
It is recommended to explicity enable the `string_ref` option on decoding if you are sure of the use of {stringref}, so that readers of the code will know of it.

### tag(28): shareable, tag(29): sharedref

Option:
- `'shared_ref'`:
  - Encode: default: `false`; values: `bool` | `'unsafe_ref'`
  - Decode: default: `true`; values: `bool` | `'shareable'` | `'unsafe_ref'`

Constants:
- `Cbor\Tag::SHAREABLE`
- `Cbor\Tag::SHARED_REF`

The tag {sharedref} can refer the previously-defined data.

If the option is enabled, CBOR maps tagged as {shareable} once decoded into PHP object will share the instance among {sharedref} references. If other type of values like CBOR array is tagged {shareable}, it triggers an error. See other option values for possible workarounds.

The option is enabled by default on decoding.

On encoding, potentially shared PHP objects (i.e. there are variables holding the object somewhere) are tagged {shareable}, and once reused, {sharedref} tag is emitted. A reference to variable is dereferenced.

If `'shareable'` is specified, non-object CBOR values tagged as {shareable} is wrapped into `Cbor\Shareable` object on decoding and the instance is reused on {sharedref} tag. On encoding, an instance of `Cbor\Shareable` is tagged {shareable} regardless of the option value.

If `'unsafe_ref'` is specified, {shareable} tagged data that decoded to non-object becomes PHP `&` reference. On encoding a reference to variable is tagged {shareable} too.
At first glance it may seem natural to use PHP reference for shared scalars and arrays. But this may cause unwanted side effects when the decoded structure contains references that you don't expect. You replace a single scalar value, and somewhere else is changed too!

Note that decoder's return value (decoding root value) cannot be a PHP reference. Moreover, a reference to a PHP object cannot be described even with this option.

The use of this tag makes CBOR contextual.

### tag(0): date/time string

Option:
- `'datetime'`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::DATETIME`

If the option is enabled, an instance of `DateTimeInterface` is encoded as a `text string` with a {date/time} tag.

### tag(2) tag(3): bignum

Option:
- `'bignum'`:
  - Encode: default: `true`; values: `bool`

Constants:
- `Cbor\Tag::BIGNUM_U`
- `Cbor\Tag::BIGNUM_N`

If the option is enabled, an instance of `GMP` is encoded as a `byte string` with {bignum} tag.

Note: If the value is within CBOR `integer` range, it is encoded as an `integer`. (preferred serialization)

### tag(4) decimal

Option:
- `'decimal'`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::DECIMAL`

If the option is enabled, an instance of `Decimal` is encoded as an `array` of integer mantissa and exponent with {decimal} tag.

Although the precision is retained, the maximum precision specified on instance creation is lost.

### tag(32) uri

Option:
- `'uri'`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::URI`

If the option is enabled, an instance of class that implements PSR-7 `UriInterface` is encoded as a `text string` with {uri} tag.
