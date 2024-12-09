#include <cardano/cardano.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  cardano_json_object_t* object = cardano_json_object_parse((const char*)data, size);

  if (object == NULL)
  {
    return 0;
  }

  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  if (writer == NULL)
  {
    cardano_json_object_unref(&object);

    return 0;
  }

  cardano_json_writer_write_object(writer, object);
  const size_t buffer_size = cardano_json_writer_get_encoded_size(writer);
  char*        buffer      = (char*)malloc(buffer_size);

  if (buffer == NULL)
  {
    cardano_json_object_unref(&object);
    cardano_json_writer_unref(&writer);

    return 0;
  }

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, buffer_size);

  if (result != CARDANO_SUCCESS)
  {
    free(buffer);
    cardano_json_object_unref(&object);
    cardano_json_writer_unref(&writer);

    fprintf(stderr, "Round trip validation failed.\n");
    abort();

    return -1;
  }

  free(buffer);
  cardano_json_object_unref(&object);
  cardano_json_writer_unref(&writer);

  return 0;
}