# PHP ext/cbor

CBOR codec extension for PHP

This extension makes it possible to encode/decode CBOR data defined in [RFC 8949](https://datatracker.ietf.org/doc/html/rfc8949) on PHP.


- [License](#license)
- [Installation](#installation)
- [Quick Guide](#quick-guide)
  - [Functions](#functions)
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

When decoding, CBOR data item must be a single item, or emits an error.
This means this function cannot decode CBOR sequence format defined in [RFC 8742](https://datatracker.ietf.org/doc/html/rfc8742).

`$options` are:

- `max_depth` (default:`64`; range: `0`..`10000`)
  Maximum number of nesting level to process.
  To handle arrays/maps/tags, at least 1 depth is required.

- `max_size`: (default:`65536`; range: `0`..`0xffffffff`)
  Maximum number of elements to process for definite-length array and map when decoding.

See "Supported Tags" below for the following options:

- `string_ref`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

- `shared_ref`:
  - Encode: default: `false`; values: `bool` | `'unsafe_ref'`
  - Decode: default: `true`; values: `bool` | `'shareable'` | `'unsafe_ref'`

- `datetime`, `bignum`, `decimal`:
  - Encode: default: `true`; values: `bool`

### Classes

#### Serializable

When encoding classes that implements `Cbor\Serializable`, the encoder will call `cborSerialize()`.
Implementors may return data structure to serialize the instance, or throw an Exception to stop serializing.
Classes that does not implement this interface cannot be serialized (aside from `stdClass` plain object).

Although some classes such as PSR-7 `UriInterface` are serializable by default as described below, `Serializable` can take precedence.

#### EncodeParams

When the encoder encounters an `Cbor\EncodeParams` instance, it encodes `$value` with the specified `$params` flags and options added to the current flags and options. After encoding inner `$value`, those parameters are back to the previous state.
This can be useful when you want to enforce specific parameters to the surrounding data.

`$params` are:

- `'flags' => int`: Encoding flags to set.
- `'flags_clear' => int`: Encoding flags to clear.
Flags in `'flags_clear'` are cleared first then flags in `'flags'` are set.
Note that you don't need to clear conflicting string flags, i.e. `CBOR_TEXT` is cleared when setting `CBOR_BYTE` and vice versa. The same applies for `CBOR_KEY_*` string flags.
- Other `$options` values for encoding.
You cannot change `'max_depth'` or options that makes CBOR data contextual.

### Types of CBOR and PHP

#### Integers

CBOR `unsigned integer` and `negative integer` are translated to PHP `int`.
The value must be within the range PHP can handle (`PHP_INT_MIN`..`PHP_INT_MAX`).
This is -2\**63..2\**63-1 on 64-bit PHP, which is narrower than CBOR's -2\**64..2\**64-1.
If decoding data contains out-of-range value, an exception is thrown.

#### Floating-Point Numbers

CBOR `float` has three sizes. 64-bit value is translated to PHP `float`.

32-bit values and 16-bit values are translated to PHP `Cbor\Float32` and `Cbor\Float16` respectively.
But if the flags `CBOR_FLOAT32` and/or `CBOR_FLOAT16` is passed, they are treated as PHP `float`.

#### Strings

CBOR has two types of strings: `byte string` (binary octets) and `text string` (UTF-8 encoded octets).
PHP `string` type does not have this distinction.

If you specify `CBOR_BYTE` flag (default) and/or `CBOR_TEXT` flag on decoding, those strings are decoded to PHP `string`. If the flags are not specified, strings are decoded to `Cbor\Byte` and `Cbor\Text` object respectively.

On encoding PHP `string`, you must specify either of the flag so that the extension knows to which you want your strings to be encoded.

`CBOR_KEY_BYTE` and `CBOR_KEY_TEXT` are for strings of CBOR `map` keys.

If `text` string is not a valid UTF-8 sequence, an error is thrown unless you pass `CBOR_UNSAFE_TEXT` flag.

#### Arrays

CBOR `array` is translated to PHP `array`.

If PHP `array` has holes or `string` keys (i.e. the array is not a "list"), it is encoded to CBOR `map`.

Number of elements in an array must be under 2**32.

#### Maps

CBOR `map` is translated to PHP `stdClass` object.
If `CBOR_MAP_AS_ARRAY` flag is passed when decoding, it is translated to PHP `array` instead.

Keys must be of CBOR `string` type.

The extension may accept CBOR `integer` key if `CBOR_INT_KEY` flag is passed. Also the flag will encode PHP `int` key (including integer numeric `string` keys in the range of CBOR `integer`) as CBOR `integer` key.

Number of properties in an object must be under 2**32.

#### Tags

CBOR `tag` is translated to PHP `Cbor\Tag(int $tag, mixed $content)` object.

Tag is a marker to mark data (including another tag) as some type using an unsigned number.
You can consult [CBOR tag registry](https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml) for valid tags.

Also see "Supported Tags" below.

#### Null and Undefined

CBOR has `null` and `undefined` value, but PHP does not have `undefined` value.

CBOR `undefined` is translated to PHP `Cbor\Undefined` singleton object.
This object is evaluated as `false` in boolean context. It is not equal to `null`.

```php
$undefined = Cbor\Undefined::get();
var_dump($undefined === clone $undefined); // true
```


## Supported Tags

### tag(55799): Self-Described CBOR

Flag:
- `CBOR_SELF_DESCRIBE`
  - default: `false`

Constants:
- `Cbor\Tag::SELF_DESCRIBE`
- `CBOR_TAG_SELF_DESCRIBE_DATA`

Self-Described CBOR is CBOR data that has this tag for the data.
This 3-byte binary string can be used to distinguish CBOR from other data including Unicode text encodings (so-called magic). This is useful if data loader need to identify the format by data itself.
```php
$isCbor = str_starts_with($data, CBOR_TAG_SELF_DESCRIBE_DATA);
```

On encoding if the flag is set, the tag is prepended to the encoded data.
On decoding if the flag is _not_ set, the tag is skipped if one exists. If the flag _is_ set, the tag is _retained_ in the decoded value, meaning you need to test if the root is this tag to extract the real content.

If the tag is to be prepended/skipped, it is handled specially and not counted as `max_depth` level.

### tag(256): stringref-namespace, tag(25): stringref

Option:
- `string_ref`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

Constants:
- `Cbor\Tag::STRING_REF_NS`
- `Cbor\Tag::STRING_REF`

The tag {stringref} is like a compression, that "references" the string previously appeared inside {stringref-namespace} tag. Note that it differs from PHP's reference to `string`, i.e. _not_ the concept of `$stringRef = &$string`.

On encode, it can save payload size by replacing the string already seen with the tag + index (or at the worst case increase by 3-bytes overall when single-namespaced).
But if smaller payload is desired, it should perform better to apply a data compression instead.

On decode it can save memory of decoded value because PHP can share the identical `string` sequences until one of them is going to be modified (copy-on-write).

For decoding, the option is enabled as `true` by default, while encoding it should be specified explicitly.

If `true` is specified on encoding, data is always wrapped with {stringref-namespace} tag. It initializes the string index (like compression dictionary) for the content inside the tag.
The {stringref-namespace} tag added implicitly is handled specially and not counted as `max_depth` level.
`'explicit'` makes {stringref} active but the root namespace is not implicitly created, meaning {stringref} is not created on its own.

The use of this tag makes CBOR contextual.
CBOR data that use {stringref} can be embedded in other CBOR. But data that doesn't use cannot always be embedded safely in {stringref} CBOR, because it will corrupt reference indices of the following strings.
(As of now, the extension cannot embed raw CBOR data on encoding though.)

Decoders without the support of this tag cannot decode data using {stringref} correctly.
It is recommended to explicity enable `string_ref` option on decoding if you are sure of the use of {stringref}, so that readers of the code will know of it.

### tag(28): shareable, tag(29): sharedref

Option:
- `shared_ref`:
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
At first glance it may seem natural to use PHP reference for shared scalars and arrays. But this will cause unexpected side effects when the decoded structure contains references that you don't expect. You replace a single scalar value, and somewhere else is changed too!

Note that decoder's return value cannot be a PHP reference. Moreover, a reference to a PHP object cannot be described even with this option.

The use of this tag makes CBOR contextual.

### tag(0): date/time string

Option:
- `datetime`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::CBOR_TAG_DATETIME`

If the option is enabled, an instance of `DateTimeInterface` is encoded as a `text string` with a {date/time} tag.

### tag(2) tag(3): bignum

Option:
- `bignum`:
  - Encode: default: `true`; values: `bool`

Constants:
- `Cbor\Tag::CBOR_TAG_BIGNUM_U`
- `Cbor\Tag::CBOR_TAG_BIGNUM_N`

If the option is enabled, an instance of `GMP` is encoded as a `byte string` with {bignum} tag.

### tag(4) decimal

Option:
- `decimal`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::CBOR_TAG_DECIMAL`

If the option is enabled, an instance of `Decimal` is encoded as an `array` of integer mantissa and exponent with {decimal} tag.

Although the precision is retained, the maximum precision specified on instance creation is lost.

### tag(32) uri

Option:
- `uri`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::CBOR_TAG_URI`

If the option is enabled, an instance of class that implements PSR-7 `UriInterface` is encoded as an `text string` with {uri} tag.
