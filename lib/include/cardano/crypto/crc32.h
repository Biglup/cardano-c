/**
 * \file crc32.h
 *
 * \author angel.castillo
 * \date   Mar 22, 2024
 *
 * Copyright 2024 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_CRC32_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_CRC32_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Computes the CRC32 checksum for the given data.
 *
 * This function calculates a 32-bit cyclic redundancy check (CRC32) checksum, used to detect
 * accidental changes to raw data. CRC32 checksums are a common method for ensuring the integrity
 * of data by producing a short, fixed-size checksum based on the input data.
 *
 * \param[in] data A pointer to the data block for which the CRC32 checksum is to be computed.
 * \param[in] size The size of the data block in bytes.
 *
 * \return Returns a 32-bit unsigned integer representing the CRC32 checksum of the provided data.
 *
 * \note This function does not modify the input data. If the data pointer is NULL or size is zero,
 * the function should return 0 as the checksum, which must be handled by the caller.
 *
 * Usage Example:
 * \code{.c}
 * const char* sample_data = "Hello, world!";
 * size_t data_size = strlen(sample_data);
 *
 * uint32_t checksum = cardano_checksum_crc32(sample_data, data_size);
 *
 * printf("CRC32 Checksum: %u\n", checksum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint32_t cardano_checksum_crc32(const byte_t* data, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_CRC32_H