/**
 * \file bench_io.c
 *
 * \author angel.castillo
 * \date   Jul 02 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include "bench_io.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Upper bound on the number of .flat files listed from one directory.
 */
static const size_t MAX_FILES = 4096U;

/**
 * \brief Upper bound on the size of a single input file.
 */
static const size_t MAX_FILE_SIZE = 10U * 1024U * 1024U;

/**
 * \brief Length of the ".flat" file name suffix.
 */
static const size_t FLAT_SUFFIX_LEN = 5U;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief qsort comparator ordering C strings lexicographically.
 *
 * \param[in] a Pointer to the first string pointer.
 * \param[in] b Pointer to the second string pointer.
 *
 * \return Negative, zero or positive per \c strcmp of the pointed-to strings.
 */
static int
compare_strings(const void* a, const void* b)
{
  return strcmp(*(const char* const*)a, *(const char* const*)b);
}

/**
 * \brief Tests whether a file name ends in ".flat".
 *
 * \param[in] name The file name to test.
 *
 * \return 1 when the name carries the suffix, 0 otherwise.
 */
static int
has_flat_suffix(const char* name)
{
  const size_t len = strlen(name);

  return ((len > FLAT_SUFFIX_LEN) && (strcmp(&name[len - FLAT_SUFFIX_LEN], ".flat") == 0)) ? 1 : 0;
}

/* DEFINITIONS ***************************************************************/

byte_t*
cardano_bench_io_read_file(const char* path, size_t* out_size)
{
  FILE* file = fopen(path, "rb");

  if (file == NULL)
  {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  const long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if ((size <= 0) || ((size_t)size > MAX_FILE_SIZE))
  {
    fclose(file);
    return NULL;
  }

  byte_t* buffer = (byte_t*)malloc((size_t)size);

  if (buffer == NULL)
  {
    fclose(file);
    return NULL;
  }

  if (fread(buffer, 1U, (size_t)size, file) != (size_t)size)
  {
    free(buffer);
    fclose(file);
    return NULL;
  }

  fclose(file);
  *out_size = (size_t)size;

  return buffer;
}

size_t
cardano_bench_io_list_flat_files(const char* dir_path, char*** out_files)
{
  *out_files = NULL;

  DIR* dir = opendir(dir_path);

  if (dir == NULL)
  {
    return 0U;
  }

  char** files = (char**)malloc(MAX_FILES * sizeof(char*));

  if (files == NULL)
  {
    closedir(dir);
    return 0U;
  }

  size_t         count = 0U;
  struct dirent* entry = NULL;

  while (((entry = readdir(dir)) != NULL) && (count < MAX_FILES))
  {
    if (has_flat_suffix(entry->d_name))
    {
      files[count] = strdup(entry->d_name);
      ++count;
    }
  }

  closedir(dir);

  if (count == 0U)
  {
    free(files);
    return 0U;
  }

  qsort(files, count, sizeof(char*), compare_strings);

  *out_files = files;

  return count;
}

void
cardano_bench_io_free_file_list(char** files, const size_t count)
{
  if (files == NULL)
  {
    return;
  }

  for (size_t i = 0U; i < count; ++i)
  {
    free(files[i]);
  }

  free(files);
}
