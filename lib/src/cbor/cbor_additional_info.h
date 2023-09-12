/**
 * Represents the 5-bit additional information included in a CBOR initial byte.
 */
typedef enum
{
  CBOR_ADDITIONAL_INFO_FALSE             = 20,
  CBOR_ADDITIONAL_INFO_TRUE              = 21,
  CBOR_ADDITIONAL_INFO_NULL              = 22,
  CBOR_ADDITIONAL_INFO_8BIT_DATA         = 24,
  CBOR_ADDITIONAL_INFO_16BIT_DATA        = 25,
  CBOR_ADDITIONAL_INFO_INFO_32BIT_DATA   = 26,
  CBOR_ADDITIONAL_INFO_64BIT_DATA        = 27,
  CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH = 31
} cbor_additional_info_t;
