dnl config.m4 for extension cbor

PHP_ARG_ENABLE(cbor, whether to enable cbor support,
[  --enable-cbor           Enable cbor support])

if test "$PHP_CBOR" != "no"; then
  PHP_NEW_EXTENSION(cbor, src/cbor.c src/compatibility.c src/decode.c src/decoder.c src/di_encoder.c src/di_decoder.c src/encode.c src/functions.c src/options.c src/types.c src/utf8.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c99)
fi
