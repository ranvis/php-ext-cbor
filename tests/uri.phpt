--TEST--
URI tag
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

eval(<<<'END'
    namespace Psr\Http\Message;

    interface UriInterface
    {
        public function __toString(): string;
    }
    END);

class MyUri implements Psr\Http\Message\UriInterface
{
    public function __construct(private string $value)
    {
    }

    public function __toString(): string
    {
        if ($this->value === '') {
            throw new RuntimeException();
        }
        return $this->value;
    }
}

class MyUriCustom extends MyUri implements Cbor\Serializable
{
    public function cborSerialize(): mixed
    {
        return 1;
    }
}

run(function () {
    // RFC 8949
    eq('0xd82076687474703a2f2f7777772e6578616d706c652e636f6d', cenc(new MyUri('http://www.example.com')));

    cencThrows(CBOR_ERROR_UNSUPPORTED_TYPE, new MyUri('http://www.example.com'), options: ['uri' => false]);
    cencThrows(CBOR_ERROR_UTF8, new MyUri("http://example.com/\xc1\x80"));
    eq('0xd8207819687474703a2f2f6578616d706c652e636f6d2f256331253830', cenc(new MyUri("http://example.com/%c1%80")));
    cencThrows(RuntimeException::class, new MyUri(''));
    eq('0x01', cenc(new MyUriCustom("")));
});

?>
--EXPECT--
Done.
