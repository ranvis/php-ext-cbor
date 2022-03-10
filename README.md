# PHP ext/cbor

CBOR codec extension for PHP


## License

BSD 2-Clause License


## What is CBOR?

CBOR, Concise Binary Object Representation is a binary data format designed to be small message size.

See [RFC8949](https://datatracker.ietf.org/doc/html/rfc8949) for the details and why we are having yet another binary message format.


## Installation

```
phpize
./configure --with-cbor
make install
```

This extension requires [libcbor](https://github.com/PJK/libcbor/) for paring and encoding CBOR data.


## Quick Guide

### API

```php
function cbor_encode(
    mixed $value,
    int $flags = CBOR_BYTE | CBOR_KEY_BYTE,
    ?array $options = [
        'max_depth' => 64,
    ],
): string;

function cbor_decode(
    string $data,
    int $flags = CBOR_BYTE | CBOR_KEY_BYTE,
    ?array $options = [
        'max_depth' => 64,
        'max_size' => 65535,
    ],
): mixed;
```


### Types of CBOR and PHP

#### Integers

CBOR `unsigned integer` and `negative integer` is translated to PHP `int`.
The value must be within the range PHP can handle.

#### Floating-Point Numbers

CBOR `float` has three sizes. 64 bits value is translated to PHP `float`.

32 bits value and 16 bits value are translated to PHP `Cbor\Float32` and `Cbor\Float16` respectively.
But if flags `CBOR_FLOAT32` and/or `CBOR_FLOAT16` is passed, they are treated as PHP `float`.

#### Strings

CBOR has two type of strings: `byte string` (binary data) and `text string` (UTF-8 encoded string).
PHP `string` type does not have this distinction.
If you specify `CBOR_BYTE` flag (default) and/or `CBOR_TEXT` flag on decoding, those strings are decoded to PHP `string`. If flag is not specified, it is decoded to `Cbor\Byte` and `Cbor\Text` object respectively.
On encoding PHP `string`, you must specify either of the flag so that the extension knows to which you want your string to be encoded.

`CBOR_KEY_BYTE` and `CBOR_KEY_TEXT` are for strings of CBOR `map` keys.

If `text` string is not a valid UTF-8 sequence, an error is thrown unless you pass `PHP_CBOR_UNSAFE_TEXT` flag.

#### Arrays

CBOR `array` is translated to PHP `array`.

If PHP `array` has holes or `string` keys, it is encoded to CBOR `map`.

Number of elements in an array must be under 2**32.

#### Maps

CBOR map is translated to PHP `stdClass` object.
If CBOR_MAP_AS_ARRAY flag is passed when decoding, it is translated to PHP `array` instead.

Key must be of string type.
The extension may accept CBOR `unsigned integer` key if `CBOR_INT_KEY` flag is passed.

Number of properties in an object must be under 2**32.

#### Tags

CBOR `tag` is translated to PHP `Cbor\Tag` object.

#### Undefined

CBOR has `null` and `undefined` value, but PHP does not have `undefined` value.

CBOR `undefined` is translated to PHP `Cbor\Undefined` singleton object.
This object is evaluated as `false` in boolean context.
But it is not equal to `null`.

## Examples

```php
TBD.
```
