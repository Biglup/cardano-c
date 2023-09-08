/**
 * Specifies the state of a CborReader instance.
 */
typedef enum
{
  /**
   * Indicates the undefined state.
   */
  CBOR_READER_STATE_UNDEFINED = 0,

  /**
   * Indicates that the next CBOR data item is an unsigned integer (major type 0).
   */
  CBOR_READER_STATE_UNSIGNED_INTEGER,

  /**
   * Indicates that the next CBOR data item is a negative integer (major type 1).
   */
  CBOR_READER_STATE_NEGATIVE_INTEGER,

  /**
   * Indicates that the next CBOR data item is a byte string (major type 2).
   */
  CBOR_READER_STATE_BYTESTRING,

  /**
   * Indicates that the next CBOR data item denotes the start of an indefinite-length byte string (major type 2).
   */
  CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTE_STRING,

  /**
   * Indicates that the reader is at the end of an indefinite-length byte string (major type 2).
   */
  CBOR_READER_STATE_END_INDEFINITE_LENGTH_BYTE_STRING,

  /**
   * Indicates that the next CBOR data item is a UTF-8 string (major type 3).
   */
  CBOR_READER_STATE_TEXT_STRING,

  /**
   * Indicates that the next CBOR data item denotes the start of an indefinite-length UTF-8 text string (major type 3).
   */
  CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXT_STRING,

  /**
   * Indicates that the reader is at the end of an indefinite-length UTF-8 text string (major type 3).
   */
  CBOR_READER_STATE_END_INDEFINITE_LENGTH_TEXT_STRING,

  /**
   * Indicates that the next CBOR data item denotes the start of an array (major type 4).
   */
  CBOR_READER_STATE_START_ARRAY,

  /**
   * Indicates that the reader is at the end of an array (major type 4).
   */
  CBOR_READER_STATE_END_ARRAY,

  /**
   * Indicates that the next CBOR data item denotes the start of a map (major type 5).
   */
  CBOR_READER_STATE_START_MAP,

  /**
   * Indicates that the reader is at the end of a map (major type 5).
   */
  CBOR_READER_STATE_END_MAP,

  /**
   * Indicates that the next CBOR data item is a semantic tag (major type 6).
   */
  CBOR_READER_STATE_TAG,

  /**
   * Indicates that the next CBOR data item is a simple value (major type 7).
   */
  CBOR_READER_STATE_SIMPLE_VALUE,

  /**
   * Indicates that the next CBOR data item is an IEEE 754 Half-Precision float (major type 7).
   */
  CBOR_READER_STATE_HALF_PRECISION_FLOAT,

  /**
   * Indicates that the next CBOR data item is an IEEE 754 Single-Precision float (major type 7).
   */
  CBOR_READER_STATE_SINGLE_PRECISION_FLOAT,

  /**
   * Indicates that the next CBOR data item is an IEEE 754 Double-Precision float (major type 7).
   */
  CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT,

  /**
   * Indicates that the next CBOR data item is a null literal (major type 7).
   */
  CBOR_READER_STATE_NULL,

  /**
   * Indicates that the next CBOR data item encodes a bool value (major type 7).
   */
  CBOR_READER_STATE_BOOLEAN,

  /**
   * Indicates that the reader has completed reading a full CBOR document.
   */
  CBOR_READER_STATE_FINISHED
} cbor_reader_state_t;
