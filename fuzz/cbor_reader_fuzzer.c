#include <cardano/cardano.h>

int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, size);

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_FINISHED)
  {
    cardano_error_t result = cardano_cbor_reader_skip_value(reader);

    if (result != CARDANO_SUCCESS)
    {
      break;
    }

    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      break;
    }
  }

  cardano_cbor_reader_unref(&reader);
  return 0;
}