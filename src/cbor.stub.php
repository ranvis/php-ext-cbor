<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 * @generate-function-entries
 */

// For depth, beware that tag data also counted as 1.

function cbor_encode(mixed $value, int $flags = CBOR_BYTE | CBOR_KEY_BYTE, int $depth = 64): string {}
function cbor_decode(string $data, int $flags = CBOR_BYTE | CBOR_KEY_BYTE, int $depth = 64): mixed {}
