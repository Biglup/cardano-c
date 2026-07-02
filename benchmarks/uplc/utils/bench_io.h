/**
 * \file bench_io.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BENCH_IO_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BENCH_IO_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

#include <stddef.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Reads a whole file into a newly allocated buffer.
 *
 * Files larger than an internal sanity limit (10 MiB, far above any real
 * Plutus script) are rejected rather than loaded.
 *
 * \param[in] path The path of the file to read.
 * \param[out] out_size On success, set to the number of bytes read.
 *
 * \return The malloc-allocated file contents, or \c NULL if the file cannot
 *         be opened, is empty, exceeds the size limit, or a read fails. The
 *         caller releases the buffer with \c free.
 */
byte_t*
cardano_bench_io_read_file(const char* path, size_t* out_size);

/**
 * \brief Lists the .flat files of a directory in lexicographic order.
 *
 * Scans \p dir_path (non-recursively) for file names ending in ".flat" and
 * returns them sorted so benchmark output is stable across runs and file
 * systems.
 *
 * \param[in] dir_path The directory to scan.
 * \param[out] out_files On success, set to a malloc-allocated array of
 *             malloc-allocated file names. The caller releases it with
 *             \ref cardano_bench_io_free_file_list.
 *
 * \return The number of file names in \p out_files, or 0 if the directory
 *         cannot be opened or holds no .flat files (in which case
 *         \p out_files is set to \c NULL).
 */
size_t
cardano_bench_io_list_flat_files(const char* dir_path, char*** out_files);

/**
 * \brief Releases a file list produced by \ref cardano_bench_io_list_flat_files.
 *
 * \param[in] files The file name array to release. May be NULL.
 * \param[in] count The number of names in \p files.
 */
void
cardano_bench_io_free_file_list(char** files, size_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_BENCH_IO_H */
