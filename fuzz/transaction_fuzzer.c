#include <cardano/cardano.h>
#include <stdlib.h>
#include <string.h>

int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex((const char*)data, size);

  cardano_transaction_t* transaction = NULL;

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader);
    cardano_transaction_unref(&transaction);

    return 0;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_transaction_clear_cbor_cache(transaction);

  result = cardano_transaction_to_cbor(transaction, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader);
    cardano_transaction_unref(&transaction);
    cardano_cbor_writer_unref(&writer);

    return 0;
  }

  const size_t cbor_hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex      = (char*)malloc(cbor_hex_size);

  result = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_hex_size);
  CARDANO_UNUSED(result);

  cardano_json_writer_t* json_writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  result                             = cardano_transaction_to_cip116_json(transaction, json_writer);
  CARDANO_UNUSED(result);

  const size_t json_size = cardano_json_writer_get_encoded_size(json_writer);
  char*        json_str  = (char*)malloc(json_size);

  result = cardano_json_writer_encode(json_writer, json_str, json_size);
  CARDANO_UNUSED(result);

  free(json_str);
  free(cbor_hex);
  cardano_json_writer_unref(&json_writer);
  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
  cardano_cbor_writer_unref(&writer);

  return 0;
}