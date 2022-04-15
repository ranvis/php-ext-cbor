<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 * @generate-function-entries
 */

/*//
 * Encode value to CBOR string.
 * @param mixed $value A value to encode
 * @param int $flags Configuration flags
 * @param array|null $options configuration options
 * @return string CBOR string
 * @throws Cbor\Exception
 */
function cbor_encode(mixed $value, int $flags = CBOR_BYTE | CBOR_KEY_BYTE, ?array $options = null): string {}

/*//
 * Decode CBOR string.
 * @param string $data CBOR string to decode
 * @param int $flags Configuration flags
 * @param array|null $options Configuration options
 * @return mixed The decoded value
 * @throws Cbor\Exception
 */
function cbor_decode(string $data, int $flags = CBOR_BYTE | CBOR_KEY_BYTE, ?array $options = null): mixed {}
