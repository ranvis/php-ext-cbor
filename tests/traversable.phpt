--TEST--
traversables
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

function gen(): Generator
{
    yield 1;
    yield 2;
    yield 4;
    yield 8;
}

function genKeyed(): Generator
{
    yield 1 => 1;
    yield '2' => 2;
    yield true => 4;
    yield [[]] => 8;
}

function genPartialKey(): Generator
{
    yield 5 => 1;
    yield 2;
    yield 5 => 4;
    yield 8;
}

function genThrows(): Generator
{
    yield 1;
    throw new RuntimeException();
}

class CountedGenerator implements IteratorAggregate, Countable
{
    public function __construct(
        protected int $count,
        protected Generator $gen,
    ) {
    }

    public function count(): int
    {
        return $this->count;
    }

    public function getIterator(): Generator
    {
        return $this->gen;
    }
}

class SerializeGenerator extends CountedGenerator implements Cbor\Serializable
{
    public function cborSerialize(): mixed
    {
        return iterator_to_array($this->gen, false);
    }
}

run(function () {
    eq('0xbf0001010202040308ff', cenc(gen()));
    eq('0xbf0101413202f504818008ff', cenc(genKeyed()));
    eq('0xbf0501060205040708ff', cenc(genPartialKey()));
    eq('0xa40001010202040308', cenc(new CountedGenerator(4, gen())));
    cencThrows(CBOR_ERROR_EXTRANEOUS_DATA, new CountedGenerator(3, gen()));
    cencThrows(CBOR_ERROR_TRUNCATED_DATA, new CountedGenerator(5, gen()));
    cencThrows(RuntimeException::class, genThrows());
    // Serializable has precedence
    eq('0x8401020408', cenc(new SerializeGenerator(4, gen())));
});

?>
--EXPECT--
Done.
