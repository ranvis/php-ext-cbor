<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 * @generate-function-entries
 */

function cbor_encode(mixed $value, int $flags = CBOR_BYTE | CBOR_KEY_BYTE, ?array $options = [
    'max_depth' => 64,
    'string_ref' => false,
]): string {}

function cbor_decode(string $data, int $flags = CBOR_BYTE | CBOR_KEY_BYTE, ?array $options = [
    'max_depth' => 64,
    'max_size' => 65536,
    'string_ref' => true,
    'shared_ref' => true,
]): mixed {}
