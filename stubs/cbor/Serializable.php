<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

namespace Cbor;

/**
 * Interface for object to CBOR serialization.
 */
interface Serializable
{
    /**
     * Serialize object to CBOR-ready value.
     * @return mixed The value of object in serializable structure.
     */
    public function cborSerialize(): mixed;
}
