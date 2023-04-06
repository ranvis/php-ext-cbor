--TEST--
traversables
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

class NoKey {}

function makeGen(array $list): callable
{
    return fn () => genFrom($list);
}

function genFrom(array $list): Generator
{
    foreach ($list as [$key, $value]) {
        if ($key instanceof NoKey) {
            yield $value;
        } else {
            yield $key => $value;
        }
    }
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
    $noKey = new NoKey();
    $items = makeGen([
        [$noKey, 1],
        [$noKey, 2],
        [$noKey, 4],
        [$noKey, 8],
    ]);
    eq('0xbf0001010202040308ff', cenc($items()));
    $keyedItems = makeGen([
        [1, 1],
        ['2', 2],
        [true, 4],
        [[[]], 8],
    ]);
    eq('0xbf0101413202f504818008ff', cenc($keyedItems()));
    $partialKeyedItems = makeGen([
        [5, 1],
        [$noKey, 2],
        [5, 4],
        [$noKey, 8],
    ]);
    eq('0xbf0501060205040708ff', cenc($partialKeyedItems()));
    eq('0xa40001010202040308', cenc(new CountedGenerator(4, $items())));
    cencThrows(CBOR_ERROR_EXTRANEOUS_DATA, new CountedGenerator(3, $items()));
    cencThrows(CBOR_ERROR_TRUNCATED_DATA, new CountedGenerator(5, $items()));
    cencThrows(RuntimeException::class, genThrows());
    // Serializable has precedence
    eq('0x8401020408', cenc(new SerializeGenerator(4, $items())));
    //  duplicate key
    cencThrows(CBOR_ERROR_DUPLICATE_KEY, $partialKeyedItems(), CBOR_MAP_NO_DUP_KEY);
    $zeroOneItems = makeGen([
        [0, true],
        [-17, true], // 0x30
        [new Cbor\Byte('0'), true],
        [new Cbor\Text('0'), true],
        [new Cbor\Float16(0.0), true],
        [1, true],
        [-18, true], // 0x31
        [new Cbor\Byte('1'), true],
        [new Cbor\Text('1'), true],
        [new Cbor\Float16(1.0), true],
        [Cbor\Undefined::get(), true],
        [null, true],
        [false, true],
        [true, true],
    ]);
    eq('0xbf00f530f54130f56130f5f90000f501f531f54131f56131f5f93c00f5f7f5f6f5f4f5f5f5ff', cenc($zeroOneItems(), CBOR_MAP_NO_DUP_KEY));
});

?>
--EXPECT--
Done.
