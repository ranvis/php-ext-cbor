# Changelog

[Unreleased]: https://github.com/ranvis/php-ext-cbor/compare/v0.4.4...HEAD
## [Unreleased]
### Added
### Changed
### Removed
### Fixed
### Security
### Deprecated

[0.4.4]: https://github.com/ranvis/php-ext-cbor/compare/v0.4.3...v0.4.4
## [0.4.4] - 2023-02-28
### Fixed
- Fix infinite loop when array_walk() is applied on CBOR value types.
- Fix memory leak when decoding undefined value.
- Fix memory leak when encoding `Traversable`.

[0.4.3]: https://github.com/ranvis/php-ext-cbor/compare/v0.4.2...v0.4.3
## [0.4.3] - 2023-01-28
### Added
- Add `'offset'` and `'length'` options to cbor_decode().
- Add `$offset` and `$length` parameters to Cbor\Decoder::add().
- Add encoding of `Traversable` to `map`.
### Changed
- '`max_size`' on decode is enforced for indefinite-length elements too.
### Fixed
- Fix redundant representation of EDN string if text string has some sort of invalid UTF-8 sequences.

[0.4.2]: https://github.com/ranvis/php-ext-cbor/compare/v0.4.1...v0.4.2
## [0.4.2] - 2022-12-10
### Added
- Add `CBOR_EDN` flag to decode to Extended Diagnostic Notation string.
### Fixed
- Stub: `'uri'` option was missing.
- Fix `CBOR_INT_KEY` overhead on encoding.

[0.4.1]: https://github.com/ranvis/php-ext-cbor/compare/v0.4.0...v0.4.1
## [0.4.1] - 2022-04-25
### Added
- Add `Decoder` class.
### Fixed
- Fix memory leak on `CBOR_ERROR_EXTRANEOUS_DATA` error.
### Security
- Fix use-after-free on XString member access or string cast.

[0.4.0]: https://github.com/ranvis/php-ext-cbor/compare/v0.3.2...v0.4.0
## [0.4.0] - 2022-04-20
### Added
- Add encoding of PSR-7 `UriInterface`.
### Changed
- Rename `Tag::MIME` to `Tag::MIME_MSG`.

[0.3.2]: https://github.com/ranvis/php-ext-cbor/compare/v0.3.1...v0.3.2
## [0.3.2] - 2022-04-19
### Added
- Add `CBOR_MAP_NO_DUP_KEY` decode flag.
- Add stub for IDE.
- Add more detailed exception message.
### Fixed
- Remove redundant zval deref.

[0.3.1]: https://github.com/ranvis/php-ext-cbor/compare/v0.3.0...v0.3.1
## [0.3.1] - 2022-04-13
### Fixed
- Fix CBOR_FLOAT* flags check on decoding.

[0.3.0]: https://github.com/ranvis/php-ext-cbor/releases/tag/v0.3.0
## [0.3.0] - 2022-04-11
### Added
- Add encoding of `\DateTimeInterface`, `\GMP`, `\Decimal`.
- Add interface `Serializable`.
- Add `EncodeParams` class.
- Add custom processing of half-precision float.
- Imlement encode to compact float mode by specifying both `PHP_CBOR_FLOAT16` and `PHP_CBOR_FLOAT32`.
- Add some constants for tags defined in spec.
- Encode integer `string` key as CBOR `integer` with `CBOR_INT_KEY`.
- Decode of `integer` keys of negative or outside PHP `int` range.
- Add type handlers and custom serialization for CBOR types.
- Implement `\JsonSerializable` for CBOR types.
- Implement low-level encoder/decoder.
- Implement reserved simple value.
### Changed
- Use custom property handler for `FloatX` to avoid unnecessary value translation.
- Renumber flags.
### Fixed
- Fix error code of shareable.
- Fix `XString` possible interned string.
- Add `FloatX` restoration error handling.
- Fix error message generation.
- Remove numeric key check for object.
- Clean up allocations.

## 0.2.2a2 - 2022-03-16
### Fixed
- Fix crash on encoding `Shareable` with `'shared_ref'` off.

## 0.2.2a1 - 2022-03-15
### Added
- Add `'shared_ref'` encode/decode option.
- Add `'shared_ref'` => `'unsafe_ref'` option value.
### Changed
- Disable reference value sharing by default.
### Fixed
- Fix crash on some sort of decode error.
- Validate decoded map key for property.
- Fix error code of undecodable shareable content.

## 0.2.1a1 - 2022-03-12
### Added
- Add `'string_ref'` encode option.
### Changed
- Rename namespace tag to `CBOR_TAG_STRING_REF_NS` from `CBOR_TAG_STRING_REF_NAMESPACE`.
- Rename `Tag->data` to `Tag->content`.

## 0.2.0a2 - 2022-03-11
### Added
- Implement `FloatX` cast to `float`.
- Add `'string_ref'` decode option.
### Fixed
- Fix error code of broken map.
- Fix depth calculation.
### Security
- Limit preallocation size of array.

## 0.2.0a1 - 2022-03-10
### Added
- Add `PHP_CBOR_MAP_AS_ARRAY` decode flag to decode CBOR `map` into PHP `array`.
### Changed
- Replace depth argument with `'max_depth'` option array item.
### Fixed
- Fix error code when allowed depth is too large.
### Security
- Add sanity check for array and map decode.

## 0.1.0a1 - 2022-03-07
### Added
- Basic encode and decode functionality is implemented.
