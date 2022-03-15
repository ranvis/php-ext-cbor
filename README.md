# PHP ext/cbor

CBOR codec extension for PHP


## License

BSD 2-Clause License


## What is CBOR?

CBOR, Concise Binary Object Representation is a binary data format designed to be small message size.

See [RFC8949](https://datatracker.ietf.org/doc/html/rfc8949) for the details and why we are having yet another binary serialization format.


## Installation

```
phpize
./configure --with-cbor
make install
```

This extension requires [libcbor](https://github.com/PJK/libcbor/) for low-level processing of CBOR data.


## Quick Guide

### Interface

```php
function cbor_encode(
    mixed $value,
    int $flags = CBOR_BYTE | CBOR_KEY_BYTE,
    ?array $options,
): string;

function cbor_decode(
    string $data,
    int $flags = CBOR_BYTE | CBOR_KEY_BYTE,
    ?array $options,
): mixed;
```
`$options` are:

- `max_depth` (default:`64`; range: `0`..`10000`)
  Maximum number of nesting level to process.
  To handle arrays/maps/tags, at least 1 depth is required.

- `max_size`: (default:`65536`; range: `0`..`0xffffffff`)
  Maximum number of elements to process for array and map when decoding.

See "Supported Tags" below for the following option:

- `string_ref`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

- `shared_ref`:
  - Encode: default: `false`; values: `bool` | `'unsafe_ref'`
  - Decode: default: `true`; values: `bool` | `'shareable'` | `'unsafe_ref'`

### Types of CBOR and PHP

#### Integers

CBOR `unsigned integer` and `negative integer` is translated to PHP `int`.
The value must be within the range PHP can handle.

#### Floating-Point Numbers

CBOR `float` has three sizes. 64 bits value is translated to PHP `float`.

32 bits value and 16 bits value are translated to PHP `Cbor\Float32` and `Cbor\Float16` respectively.
But if the flags `CBOR_FLOAT32` and/or `CBOR_FLOAT16` is passed, they are treated as PHP `float`.

#### Strings

CBOR has two type of strings: `byte string` (binary data) and `text string` (UTF-8 encoded string).
PHP `string` type does not have this distinction.
If you specify `CBOR_BYTE` flag (default) and/or `CBOR_TEXT` flag on decoding, those strings are decoded to PHP `string`. If the flag is not specified, it is decoded to `Cbor\Byte` and `Cbor\Text` object respectively.
On encoding PHP `string`, you must specify either of the flag so that the extension knows to which you want your string to be encoded.

`CBOR_KEY_BYTE` and `CBOR_KEY_TEXT` are for strings of CBOR `map` keys.

If `text` string is not a valid UTF-8 sequence, an error is thrown unless you pass `PHP_CBOR_UNSAFE_TEXT` flag.

#### Arrays

CBOR `array` is translated to PHP `array`.

If PHP `array` has holes or `string` keys (i.e. array is not not "list"), it is encoded to CBOR `map`.

Number of elements in an array must be under 2**32.

#### Maps

CBOR map is translated to PHP `stdClass` object.
If `CBOR_MAP_AS_ARRAY` flag is passed when decoding, it is translated to PHP `array` instead.

Key must be of string type.
The extension may accept CBOR `unsigned integer` key if `CBOR_INT_KEY` flag is passed. Also the flag will encode PHP unsigned integer key as CBOR `unsigned integer` key.

Number of properties in an object must be under 2**32.

#### Tags

CBOR `tag` is translated to PHP `Cbor\Tag(int $tag, mixed $content)` object.

Tag is a marker to mark data (including another tag) as some type using unsigned number.
You can consult [CBOR tag registry](https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml) for valid tags.

Also see "Supported Tags" below.

#### Undefined

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
  - default: `false`; values: `bool`

Constant:
- `CBOR_TAG_SELF_DESCRIBE`

Data:
- `CBOR_TAG_SELF_DESCRIBE_DATA`

Self-Described CBOR is CBOR data that has this tag for the data.
This 3-byte binary string can be used to distinguish CBOR from other data including Unicode text encodings (so-called magic). This is useful if reader need to identify the format by data itself.
```
$isCbor = str_starts_with($data, CBOR_TAG_SELF_DESCRIBE_DATA);
```

On encoding if the flag is set, the tag is prepended to the encoded data.
On decoding if the flag is set, the tag is _retained_ in the decoded value, meaning you need to test if the root is this tag to extract the real content.

If the tag is to be prepended/skipped, it is handled specially and not counted as `max_depth` level.

### tag(256): stringref-namespace, tag(25): stringref

Options:
- `string_ref`:
  - Encode: default: `false`; values: `bool` | `'explicit'`
  - Decode: default: `true`; values: `bool`

Constants:
-	`CBOR_TAG_STRING_REF_NS`
- `CBOR_TAG_STRING_REF`

The tag {stringref} is like a compression, that "references" the string previously appeared inside {stringref-namespace} tag. Note that it differs from PHP's reference to `string`, i.e. not the concept of `$stringRef = &$string`.
On encode, it can save payload size by replacing the string already seen with the tag + index (or at the worst case increase by 3-bytes overall when single-namespaced). On decode it can save memory on decode becasue PHP can reference count the same `string`. At the cost of keeping hash table of the strings for both encoding and decoding.

For decoding, the option is enabled by default, while encoding it should be specified explicitly.
If `true` is specified on encoding, data is always wrapped with {stringref-namespace} tag. It resets the string index (like compression dictionary) for the content inside the tag. `'explicit'` makes {stringref} active but the root namespace is not implicitly created, meaning {stringref} is not created on its own.

On encoding, the {stringref-namespace} tag added implicitly is handled specially and not counted as `max_depth` level.

Note that CBOR data that use {stringref} can be embedded in other CBOR. But data that doesn't use cannot always be embedded safely in {stringref} CBOR, because it will corrupt reference index of the following strings.
(As of now, the extension cannot embed raw CBOR data on encoding.)

Decoders without the support of this tag cannot decode data correctly.

### tag(28): shareable, tag(29): sharedref

Options:
- `shared_ref`:
  - Encode: default: `false`; values: `bool` | `'unsafe_ref'`
  - Decode: default: `true`; values: `bool` | `'shareable'` | `'unsafe_ref'`

Constants:
-	`CBOR_TAG_SHAREABLE`
- `CBOR_TAG_SHARED_REF`

The tag {sharedref} can refer the previously-defined data.

If the option is enabled, CBOR maps tagged as {shareable} and decoded into PHP object will share the instance among {sharedref} references. If other type of values like CBOR array is tagged {shareable}, it triggers an error. See other option values for possible workarounds.

On encoding, potentially shared PHP objects (i.e. there are variables holding it somewhere) are tagged {shareable}, and once reused, {sharedref} tag is emitted.

If `'shareable'` is specified, non-object CBOR values tagged as {shareable} is wrapped into `Cbor\Shareable` object on decoding and the instance is reused on {sharedref} tag. On encoding, `Cbor\Shareable` value is tagged {shareable} unless `shared_ref` is `false` (error in that case).

If `'unsafe_ref'` is specified, {shareable} tagged data that decoded to non-object becomes PHP `&` reference.
At first glance it may seem natural to use PHP reference for shared scalars and arrays. But this will cause unexpected side effects when the decoded structure contains references that you don't expect. You change a single scalar value, and somewhere else is changed too!

Note that decoder's return value cannot be a PHP reference. Moreover, a reference to a PHP object cannot be described even with this option.
The option is enabled by default on decoding.

## Examples

```php
TBD.
```
