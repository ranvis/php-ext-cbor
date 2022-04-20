<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

namespace PHPSTORM_META {
    registerArgumentsSet('CborEncodeFlags',
/* encode flags start */
CBOR_SELF_DESCRIBE | CBOR_BYTE | CBOR_TEXT | CBOR_INT_KEY | CBOR_KEY_BYTE | CBOR_KEY_TEXT | CBOR_UNSAFE_TEXT | CBOR_FLOAT16 | CBOR_FLOAT32
/* encode flags end */
    );
    registerArgumentsSet('CborDecodeFlags',
/* decode flags start */
CBOR_SELF_DESCRIBE | CBOR_BYTE | CBOR_TEXT | CBOR_INT_KEY | CBOR_KEY_BYTE | CBOR_KEY_TEXT | CBOR_UNSAFE_TEXT | CBOR_MAP_AS_ARRAY | CBOR_MAP_NO_DUP_KEY | CBOR_FLOAT16 | CBOR_FLOAT32
/* decode flags end */
    );

    registerArgumentsSet('CborEncodeOptions', [
        'max_depth' => 64,
        'string_ref' => false,
        'shared_ref' => false,
        'datetime' => true,
        'bignum' => true,
        'decimal' => true,
    ]);
    registerArgumentsSet('CborDecodeOptions', [
        'max_depth' => 64,
        'max_size' => 65536,
        'string_ref' => true,
        'shared_ref' => false,
    ]);

    expectedArguments(\cbor_encode(), 1, argumentsSet('CborEncodeFlags'));
    expectedArguments(\cbor_encode(), 2, argumentsSet('CborEncodeOptions'));

    expectedArguments(\cbor_decode(), 1, argumentsSet('CborDecodeFlags'));
    expectedArguments(\cbor_decode(), 2, argumentsSet('CborDecodeOptions'));
}
