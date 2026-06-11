# PHP ext/cbor

CBOR codec extension for PHP

This extension makes it possible to encode/decode CBOR data defined in [RFC 8949](https://datatracker.ietf.org/doc/html/rfc8949) on PHP.

This extension itself is not a PHP instance serializer, just as the CBOR data format itself is not.

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
Encodes to or decodes from a CBOR data item string.

```php
try {
    echo bin2hex(cbor_encode(['binary', 1, 2, null])), "\n";
    // 844662696e6172790102f6
    var_export(cbor_decode(hex2bin('844662696e6172790102f6')));
    // array (
    //  0 => 'binary',
    //  1 => 1,
    //  2 => 2,
    //  3 => NULL,
    // )
} catch (Cbor\Exception $e) {
    echo $e->getMessage(), "\n";
}
```

When decoding, the CBOR data item must be a single item, or the function will throw an exception with code `CBOR_ERROR_EXTRANEOUS_DATA`.
This means this function cannot decode the CBOR sequences format defined in [RFC 8742](https://datatracker.ietf.org/doc/html/rfc8742).
See `Decoder` class for sequences and progressive decoding.

`$options` array elements are:

- `'max_depth'` (default:`64`; range: `0`..`10000`)

  The maximum number of nesting levels to process.
  To handle structured elements such as arrays, maps and tags, at least 1 depth is required.

- `'max_size'`:
  - Decode: default:`65536`; range: `0`..`0xffffffff`

  The maximum number of elements in arrays and maps to process.

  Depending on the actual limit set by PHP, the effective value may be lowered.

- `'offset'`:
  - Decode: default:`0`; range: `0`..`PHP_INT_MAX`

  The offset of the data to start decoding from. The offset cannot go beyond the length of the data.

- `'length'`:
  - Decode: default:`null`; range: `null` | `0`..`PHP_INT_MAX`

  The length of the data to decode from the offset. `null` means the whole remaining data. Length cannot go beyond the available length of the data.

  Although you cannot decode an empty string, `0` is valid as an option value.

See "Supported Tags" below for the following options:

- `'datetime'`, `'bignum'`, `'decimal'`:
  - Encode: default: `true`; values: `bool`

- `'string_ref'`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

- `'shared_ref'`:
  - Encode: default: `false`; values: `bool` | `'unsafe_ref'`
  - Decode: default: `false`; values: `bool` | `'shareable'` | `'shareable_only'` | `'unsafe_ref'`

Unknown key names are silently ignored.

### Core Deterministic Encoding

You can use `CBOR_CDE` encoding flag to make encoded data satisfy core deterministic encoding requirements.

CBOR types can have multiple binary sequences to express the same value, but the extension encodes values in the smallest possible form (well-formed) regardless of the flag.
The exception is for `float` values (see below).

The `CBOR_CDE` flag enforces the `CBOR_MAP_NO_DUP_KEY`, `CBOR_FLOAT16` and `CBOR_FLOAT32` flags, while the `CBOR_UNSAFE_TEXT` flag can be used regardless.

The flag cannot be used with options that make CBOR data contextual, which are mentioned later.

### Diagnostic Notation

With the decoding flag `CBOR_EDN`, CBOR data (in binary) is decoded to Extended Diagnostic Notation (EDN) `string` defined in [RFC 8610 appendix G](https://datatracker.ietf.org/doc/html/rfc8610#appendix-G). (It does _not_ decode EDN string.)
This can be used to inspect CBOR data if something is wrong.

```php
var_dump(cbor_decode(hex2bin('83010243030405'), CBOR_EDN)); // string(17) "[1, 2, h'030405']"
```
`$flags` except `CBOR_SELF_DESCRIBE` and `$options` for decoding are ignored for this mode.

Formatting `$options` can be specified:

- `'indent'` (default: `false`; values: `false` | `0`..`16` | `"\t"`)
  Number of space characters for indentation.
  `false` for no pretty-printing (one-line).
  `"\t"` to use a single tab character.

- `'space'` (default: `true`; values: `bool`)
  Whether to insert a space character after separators to improve readability.

- `'byte_space'` (default: `0`; values: `0`..`63`)
  Add a space for every 1/2/4/8/16/32 bytes of a `byte string`.
  For example, if the value `5` (`1 | 4`) is specified, `h'112233445566...'` will be `h'11 22 33 44  55 66 ...'`.

- `'byte_wrap'` (default: `false`; values: `false` | `1`..`1024`)
  Break down a `byte string` into multiple `h'...'` notation for every specified length.

Note that when `CBOR_EDN` is specified, the function will not throw an exception for data error (such as truncated data). It will instead print an error in the returned string as a comment.

Invalid UTF-8 sequences are represented as byte string fragments inside a text string, along with consecutive ASCII control characters.

### Classes

#### Serializable

When encoding classes that implement `Cbor\Serializable`, the encoder will call `cborSerialize()`.
Implementers may return a data structure to serialize the instance, or throw an exception to stop serializing.

Although some classes such as `Traversable` and PSR-7 `UriInterface` are serializable by default as described below, `Serializable` can take precedence.

`stdClass` plain objects are serialized to `map` by default.

#### EncodeParams

When the encoder encounters a `Cbor\EncodeParams` instance, it encodes the `$value` instance variable with the specified `$params` flags and options added to the current flags and options. After encoding the inner `$value`, those parameters are back to the previous state.
This can be useful when you want to enforce specific parameters partially.

`$params` array elements are:

- `'flags' => int`: Encoding flags to set.
- `'flags_clear' => int`: Encoding flags to clear.

   Flags in `'flags_clear'` are cleared first then flags in `'flags'` are set.
Note that you don't need to clear conflicting string flags, `CBOR_TEXT` is cleared when setting `CBOR_BYTE`, and vice versa. The same applies for `CBOR_KEY_*` string flags.

  You cannot clear `CBOR_CDE` flag.
- Other `$options` values for encoding.

  You cannot change `'max_depth'` or options that make CBOR data contextual.

Unknown or unsupported key names are silently ignored.


#### Decoder

The class `Cbor\Decoder` can do what `cbor_decode()` does in a more controlled way.

Instantiate `Decoder` with the optional `$flags` and `$options`, then feed CBOR data using the `add()` method. `Decoder` will append the passed data to the internal buffer.

The `'offset'` and `'length'` options have no effects for this class. You may specify those as parameters of `add()` instead.
The parameters act like those of the `substr()` PHP function.

To process data in the buffer, call `process()`. It will return `true` if a data item is decoded. You can also test it with the `hasValue()` method. Call `getValue()` to retrieve the decoded value.
If another data item follows (CBOR sequences), call `add()` and/or `process()` again.

`process()` will not return `true` until a complete item is decoded. Until then, `isPartial()` returns `true`, and you need to feed more data to complete the decoding.
`getBuffer()` returns a copy of the internal buffer. Note that it doesn't contain data that is partially consumed by `process()`.

**Progressive decoding**

With the class, large data can be decoded progressively without loading the whole data in memory at once:

```php
if (!($fp = fopen($filePath, 'rb'))) {
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

If decoding data contains an out-of-range value, an exception is thrown.

#### Floating-Point Numbers

CBOR `float` has three sizes. 64-bit values are translated to PHP `float`.

32-bit values and 16-bit values are decoded to PHP `Cbor\Float32` and `Cbor\Float16` respectively (derived from `Cbor\FloatX`).
But if the flags `CBOR_FLOAT32` and/or `CBOR_FLOAT16` are passed, they are decoded to PHP `float`.

When encoding PHP `float`, values are stored as a 64-bit value.
If either of the flags is specified, values are coerced and stored in that size.
If both flags are set, the smallest possible type is used; therefore, no informational loss is expected.

For `Cbor\Float32` type, the size of the value is retained even if it can be expressed in a half-precision type on encoding. The `CBOR_CDE` flag overrides this behavior to enforce the CDE requirements.

#### Strings

CBOR has two types of strings: `byte string` (binary octets) and `text string` (UTF-8 encoded octets).
PHP `string` type does not have this distinction.

If you specify the `CBOR_BYTE` flag (default) and/or the `CBOR_TEXT` flag on decoding, those strings are decoded to PHP `string`. If the flags are not specified, strings are decoded to `Cbor\Byte` and `Cbor\Text` objects respectively (derived from `Cbor\XString`).

On encoding PHP `string`, you must specify either of the flags so that the extension can encode strings to CBOR strings. The default value for the flags parameter is `CBOR_BYTE`.

The flags `CBOR_KEY_BYTE` and `CBOR_KEY_TEXT` are for strings of CBOR `map` keys.

If a `text string` is not a valid UTF-8 sequence, an exception is thrown unless you pass the `CBOR_UNSAFE_TEXT` flag.

#### Arrays

CBOR `array` is translated to PHP `array`.

If PHP `array` is not a "list" but a hash (that is, it has holes, `string` keys or unordered keys), it is encoded to CBOR `map`.

#### Maps

CBOR `map` is translated to PHP `stdClass` object.
If the `CBOR_MAP_AS_ARRAY` flag is passed when decoding, it is translated to PHP `array` instead.

Keys must be of CBOR `string` type.

The extension may accept CBOR `integer` keys if the `CBOR_INT_KEY` flag is passed. Likewise with the flag, it will encode PHP `int` key (including integer numeric `string` keys in the range of CBOR `integer`) as CBOR `integer` key.

If the `CBOR_MAP_NO_DUP_KEY` flag is specified on decoding, encountering a duplicated key will throw an exception instead of overriding the former value. This may happen on valid CBOR `map`; e.g. all of unsigned integer `1`, text string `"1"`, and byte string `'1'` may be the same key for PHP.

If the `CBOR_CDE` flag is specified on encoding, keys are sorted in the bytewise lexicographic order.

#### Tags

CBOR `tag` is translated to PHP `Cbor\Tag(int $tag, mixed $content)` object.

A tag is a marker to mark data (including another tag) as some type using an `unsigned integer`.
You can consult [CBOR tag registry](https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml) for valid tags.

Tags consume one `'max_depth'` for each nesting.

Also see "Supported Tags" below.

#### Null and Undefined

CBOR has `null` and `undefined` values, but PHP does not have an `undefined` value.

CBOR `undefined` is translated to PHP `Cbor\Undefined` singleton object.
This object is evaluated as `false` in a boolean context. It is not equal to `null`.

```php
$undefined = Cbor\Undefined::get();
var_dump($undefined === clone $undefined); // true
```

#### PHP `Traversable`s

An instance of a class that implements `Traversable` is encoded to `map`.

If the class does not implement `Countable`, the instance is encoded to an indefinite-length `map` unless the `CBOR_CDE` flag is specified.

The `CBOR_INT_KEY` flag does not take effect on encoding `Traversable` objects, and the key is encoded according to the actual type.


## Supported Tags

Note: In this section, tag names are written as {tag-name} for clarity.

### tag(0): date/time string

Option:
- `'datetime'`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::DATETIME`

If the option is enabled on encoding, an instance of `DateTimeInterface` is encoded as a `text string` with a {date/time} tag.

Since the tag only stores datetimes with timezone offsets, regional information will be lost.

### tag(2) tag(3): bignum

Option:
- `'bignum'`:
  - Encode: default: `true`; values: `bool`

Constants:
- `Cbor\Tag::BIGNUM_U`
- `Cbor\Tag::BIGNUM_N`

If the option is enabled on encoding, an instance of `GMP` is encoded as a `byte string` with {bignum} tag.

Note: If the value is within CBOR `integer` range, it is encoded as an `integer`. (preferred serialization)

### tag(4) decimal

Option:
- `'decimal'`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::DECIMAL`

If the option is enabled on encoding, an instance of `Decimal\Decimal` (`Decimal`) is encoded as an `array` of integer mantissa and exponent with {decimal} tag.

Although the precision is retained, the maximum precision specified on instance creation is lost.

### tag(32) uri

Option:
- `'uri'`:
  - Encode: default: `true`; values: `bool`

Constant:
- `Cbor\Tag::URI`

If the option is enabled on encoding, an instance of class that implements PSR-7 `UriInterface` is encoded as a `text string` with {uri} tag.

### tag(55799): Self-Described CBOR

Flag:
- `CBOR_SELF_DESCRIBE`
  - default: `false`

Constants:
- `Cbor\Tag::SELF_DESCRIBE`
- `CBOR_TAG_SELF_DESCRIBE_DATA`

Self-Described CBOR is CBOR data that begins with this tag.
This 3-byte binary string (magic pattern) can be used to distinguish CBOR from other data, including Unicode text encodings. This is useful if a data loader needs to identify the format by the data itself.
```php
$isCbor = str_starts_with($data, CBOR_TAG_SELF_DESCRIBE_DATA);
```

On encoding, if the flag is set, the tag is prepended to the encoded data.
On decoding, if the flag is _not_ set, the tag is skipped even if one exists. If the flag _is_ set, the tag is retained in the decoded value, meaning you need to test if the root is this tag to extract the real content.

If the tag is to be prepended/skipped, it is handled specially and not counted as a `'max_depth'` level.

### tag(256): stringref-namespace, tag(25): stringref

\* This tag is not in the RFC but registered in the CBOR Tags registry.

Option:
- `'string_ref'`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

Constants:
- `Cbor\Tag::STRING_REF_NS`
- `Cbor\Tag::STRING_REF`

The tag {stringref} is like a compression that "references" a string previously appearing inside a {stringref-namespace} tag. Note that this differs from PHP's reference to `string`; it is _not_ the `$stringRef = &$string` concept.

On encoding, it can save payload size by replacing a string already seen with the tag + index (or, at worst, increase by 3 bytes overall when single-namespaced).

Note that if a smaller payload is desired, it would perform better to apply data compression instead of using this tag.
Also, for maps, small `integer` keys (`1`..`23`, `-1`..`-24` are all 1-byte keys) are often used to minimize a payload. You may want to avoid defining key `0` as it sometimes means an empty value as a data definition convention. And also because when working with the `CBOR_MAP_AS_ARRAY` flag, it can transform an array into a "list" array in PHP, which cannot be encoded back to a CBOR `map`.

On decoding, the use of the tag can save some memory consumption because of copy-on-write; PHP can share identical `string` sequences until one of them is modified.

For decoding, the option is enabled as `true` by default, while encoding it should be specified explicitly.

If `true` is specified on encoding, data is always wrapped with a {stringref-namespace} tag. It initializes the string index table (like compression dictionary) for the content inside the tag.
The {stringref-namespace} tag added implicitly is handled specially and not counted as `'max_depth'` level.
Similarly, `'explicit'` makes {stringref} active but the root namespace is not implicitly created, meaning a {stringref} is not created on its own.

The use of this tag makes CBOR contextual.
CBOR data that uses {stringref} can be embedded in other CBOR. However, data that doesn't use it cannot always be safely embedded in {stringref} CBOR, as it will corrupt the reference indices of subsequent strings.
(As of now, the extension cannot embed raw CBOR data on encoding though.)

Decoders without the support of this tag cannot decode data using {stringref} correctly.

It is recommended to explicitly enable the `string_ref` option on decoding if you are sure of the use of {stringref}, so that readers of the code will know of it.

### tag(28): shareable, tag(29): sharedref

\* This tag is not in the RFC but registered in the CBOR Tags registry.

Option:
- `'shared_ref'`:
  - Encode: default: `false`; values: `bool` | `'unsafe_ref'`
  - Decode: default: `false`; values: `bool` | `'shareable'` | `'shareable_only'` | `'unsafe_ref'`

Constants:
- `Cbor\Tag::SHAREABLE`
- `Cbor\Tag::SHARED_REF`

The tag {sharedref} can refer the previously-defined data.

If the option is enabled, CBOR maps tagged as {shareable} once decoded into PHP object will share the instance among {sharedref} references. If another type of value, including a CBOR array or tag, is tagged {shareable}, it triggers an error. See other option values for possible workarounds.

On encoding, a PHP `stdClass` object is tagged as {shareable} if it may be referenced by multiple variables. When such object is reused, {sharedref} tag is emitted. A reference to variable is dereferenced.

If `'shareable'` is specified, values tagged as {shareable} which are decoded to non-object are wrapped into `Cbor\Shareable` object on decoding, and the instances are reused on {sharedref} tag.
On encoding, an instance of `Cbor\Shareable` is tagged {shareable} regardless of the option value, unless the flag `CBOR_CDE` is specified. In which case, the encoder throws an exception.

`'shareable_only'` works similar to `'shareable'`. But values tagged {shareable} are always wrapped into `Cbor\Shareable`.

If `'unsafe_ref'` is specified, {shareable}-tagged data that decodes to a non-object becomes a PHP `&` reference. On encoding, a reference to a variable is also tagged {shareable}.
At first glance it may seem natural to use PHP reference for shared scalars and arrays. However, this may cause unwanted side effects when the decoded structure contains references that you don't expect. If you replace a single scalar value, values elsewhere are also changed!

Note that decoder's return value (decoding root value) cannot be a PHP reference. Moreover, a reference to a PHP object cannot be described even with this option.

The use of this tag makes CBOR contextual.
