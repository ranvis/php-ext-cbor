# Changelog

[Unreleased]: https://github.com/ranvis/php-ext-cbor/compare/v0.4.1...HEAD
## [Unreleased]
### Added
### Changed
### Removed
### Fixed
- Stub: 'uri' option was missing.
### Security
### Deprecated

[0.4.1]: https://github.com/ranvis/php-ext-cbor/compare/v0.4.0...v0.4.1
## [0.4.1] - 2022-04-25
### Added
- Add `Decoder` class.
### Fixed
- Fix memory leak on CBOR_ERROR_EXTRANEOUS_DATA.
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
- Encode int `string` key as CBOR `integer` with `CBOR_INT_KEY`.
- Decode of int keys of negative or outside zend_long range.
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
- Fix crash on encoding `Shareable` with `shared_ref` off.

## 0.2.2a1 - 2022-03-15
### Added
- Add `shared_ref` encode/decode option.
- Add `shared_ref` => 'unsafe_ref' option value.
### Changed
- Disable reference value sharing by default.
### Fixed
- Fix crash on some sort of decode error.
- Validate decoded map key for property.
- Fix error code of undecodable shareable content.

## 0.2.1a1 - 2022-03-12
### Added
- Add `string_ref` encode option.
### Changed
- Rename namespace tag to `CBOR_TAG_STRING_REF_NS` from `CBOR_TAG_STRING_REF_NAMESPACE`.
- Rename `Tag->data` to `Tag->content`.

## 0.2.0a2 - 2022-03-11
### Added
- Implement `FloatX` cast to `float`.
- Add `string_ref` decode option.
### Fixed
- Fix error code of broken map.
- Fix depth calculation.
### Security
- Limit preallocation size of array.

## 0.2.0a1 - 2022-03-10
### Added
- Add `PHP_CBOR_MAP_AS_ARRAY` decode flag to decode CBOR `map` into PHP `array`.
### Changed
- Replace depth argument with `max_depth` option array item.
### Fixed
- Fix error code when allowed depth is too large.
### Security
- Add sanity check for array and map decode.

## 0.1.0a1 - 2022-03-07
### Added
- Basic encode and decode functionality is implemented.
