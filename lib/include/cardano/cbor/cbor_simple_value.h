/**
 * Represents a CBOR simple value (major type 7).
 */
typedef enum
{
  /**
   * Represents the value 'false'.
   */
  CBOR_SIMPLE_VALUE_FALSE = 20,

  /**
   * Represents the value 'true'.
   */
  CBOR_SIMPLE_VALUE_TRUE = 21,

  /**
   * Represents the value 'null'.
   */
  CBOR_SIMPLE_VALUE_NULL = 22,

  /**
   * Represents an undefined value, to be used by an encoder as a substitute for a data item with an encoding problem.
   */
  CBOR_SIMPLE_VALUE_UNDEFINED = 23
} cbor_simple_value_t;
