--TEST--
Cbor\Serializable
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

class Test implements Cbor\Serializable
{
    public function __construct(public mixed $value)
    {
    }

    public function cborSerialize(): mixed
    {
        if ($this->value === null) {
            throw new RuntimeException();
        }
        return ['v' => $this->value, 't' => new DateTimeImmutable('2022-01-01t01:01:01.01+01:00')];
    }
}

run(function () {
    $instance = new Test('value');
    eq('0x81a241764576616c75654174c0781c323032322d30312d30315430313a30313a30312e30312b30313a3030', cenc([$instance]));
    // throw inside
    $instance->value = null;
    throws(RuntimeException::class, fn () => cenc([$instance]));
    // recursion
    $instance->value = [$instance];
    cencThrows(CBOR_ERROR_RECURSION, [$instance]);

    // updating value
    $obj = (object)['a' => 0, 'b' => null, 'c' => 2];
    $instance = new class (function () use ($obj) {
        $obj->c = -2;
        $obj->d = 3;
    }) implements Cbor\Serializable {
        public function __construct(public $handler)
        {
        }

        public function cborSerialize(): mixed
        {
            ($this->handler)();
            return 1;
        }
    };
    $obj->b = $instance;
    eq('0x82a3416100416201416302a4416100416201416321416403', cenc([$obj, $obj]));
});

?>
--EXPECT--
Done.
