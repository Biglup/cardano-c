/**
 * \file crc21.c
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

/* INCLUDES ******************************************************************/

#include <cardano/crypto/crc32.h>

/* MACROS ********************************************************************/

#ifdef __GNUC__
#ifdef __cplusplus
#define CARDANO_32_ALIGN alignas(32)
#else
#define CARDANO_32_ALIGN __attribute__((aligned(32))) const
#endif
#else
#define CARDANO_32_ALIGN
#endif

/* STATIC CONSTANTS **********************************************************/

/* clang-format off */

// We suppress the MISRA C:2012 rule 8.9 because the table inlining the table inside the function
// would make the code less readable.

// cppcheck-suppress misra-c2012-8.9
CARDANO_32_ALIGN static uint32_t s_crc32_table[256] =
{
    (uint32_t)(0x00000000), (uint32_t)(0x77073096), (uint32_t)(0xee0e612c), (uint32_t)(0x990951ba),
    (uint32_t)(0x076dc419), (uint32_t)(0x706af48f), (uint32_t)(0xe963a535), (uint32_t)(0x9e6495a3),
    (uint32_t)(0x0edb8832), (uint32_t)(0x79dcb8a4), (uint32_t)(0xe0d5e91e), (uint32_t)(0x97d2d988),
    (uint32_t)(0x09b64c2b), (uint32_t)(0x7eb17cbd), (uint32_t)(0xe7b82d07), (uint32_t)(0x90bf1d91),
    (uint32_t)(0x1db71064), (uint32_t)(0x6ab020f2), (uint32_t)(0xf3b97148), (uint32_t)(0x84be41de),
    (uint32_t)(0x1adad47d), (uint32_t)(0x6ddde4eb), (uint32_t)(0xf4d4b551), (uint32_t)(0x83d385c7),
    (uint32_t)(0x136c9856), (uint32_t)(0x646ba8c0), (uint32_t)(0xfd62f97a), (uint32_t)(0x8a65c9ec),
    (uint32_t)(0x14015c4f), (uint32_t)(0x63066cd9), (uint32_t)(0xfa0f3d63), (uint32_t)(0x8d080df5),
    (uint32_t)(0x3b6e20c8), (uint32_t)(0x4c69105e), (uint32_t)(0xd56041e4), (uint32_t)(0xa2677172),
    (uint32_t)(0x3c03e4d1), (uint32_t)(0x4b04d447), (uint32_t)(0xd20d85fd), (uint32_t)(0xa50ab56b),
    (uint32_t)(0x35b5a8fa), (uint32_t)(0x42b2986c), (uint32_t)(0xdbbbc9d6), (uint32_t)(0xacbcf940),
    (uint32_t)(0x32d86ce3), (uint32_t)(0x45df5c75), (uint32_t)(0xdcd60dcf), (uint32_t)(0xabd13d59),
    (uint32_t)(0x26d930ac), (uint32_t)(0x51de003a), (uint32_t)(0xc8d75180), (uint32_t)(0xbfd06116),
    (uint32_t)(0x21b4f4b5), (uint32_t)(0x56b3c423), (uint32_t)(0xcfba9599), (uint32_t)(0xb8bda50f),
    (uint32_t)(0x2802b89e), (uint32_t)(0x5f058808), (uint32_t)(0xc60cd9b2), (uint32_t)(0xb10be924),
    (uint32_t)(0x2f6f7c87), (uint32_t)(0x58684c11), (uint32_t)(0xc1611dab), (uint32_t)(0xb6662d3d),
    (uint32_t)(0x76dc4190), (uint32_t)(0x01db7106), (uint32_t)(0x98d220bc), (uint32_t)(0xefd5102a),
    (uint32_t)(0x71b18589), (uint32_t)(0x06b6b51f), (uint32_t)(0x9fbfe4a5), (uint32_t)(0xe8b8d433),
    (uint32_t)(0x7807c9a2), (uint32_t)(0x0f00f934), (uint32_t)(0x9609a88e), (uint32_t)(0xe10e9818),
    (uint32_t)(0x7f6a0dbb), (uint32_t)(0x086d3d2d), (uint32_t)(0x91646c97), (uint32_t)(0xe6635c01),
    (uint32_t)(0x6b6b51f4), (uint32_t)(0x1c6c6162), (uint32_t)(0x856530d8), (uint32_t)(0xf262004e),
    (uint32_t)(0x6c0695ed), (uint32_t)(0x1b01a57b), (uint32_t)(0x8208f4c1), (uint32_t)(0xf50fc457),
    (uint32_t)(0x65b0d9c6), (uint32_t)(0x12b7e950), (uint32_t)(0x8bbeb8ea), (uint32_t)(0xfcb9887c),
    (uint32_t)(0x62dd1ddf), (uint32_t)(0x15da2d49), (uint32_t)(0x8cd37cf3), (uint32_t)(0xfbd44c65),
    (uint32_t)(0x4db26158), (uint32_t)(0x3ab551ce), (uint32_t)(0xa3bc0074), (uint32_t)(0xd4bb30e2),
    (uint32_t)(0x4adfa541), (uint32_t)(0x3dd895d7), (uint32_t)(0xa4d1c46d), (uint32_t)(0xd3d6f4fb),
    (uint32_t)(0x4369e96a), (uint32_t)(0x346ed9fc), (uint32_t)(0xad678846), (uint32_t)(0xda60b8d0),
    (uint32_t)(0x44042d73), (uint32_t)(0x33031de5), (uint32_t)(0xaa0a4c5f), (uint32_t)(0xdd0d7cc9),
    (uint32_t)(0x5005713c), (uint32_t)(0x270241aa), (uint32_t)(0xbe0b1010), (uint32_t)(0xc90c2086),
    (uint32_t)(0x5768b525), (uint32_t)(0x206f85b3), (uint32_t)(0xb966d409), (uint32_t)(0xce61e49f),
    (uint32_t)(0x5edef90e), (uint32_t)(0x29d9c998), (uint32_t)(0xb0d09822), (uint32_t)(0xc7d7a8b4),
    (uint32_t)(0x59b33d17), (uint32_t)(0x2eb40d81), (uint32_t)(0xb7bd5c3b), (uint32_t)(0xc0ba6cad),
    (uint32_t)(0xedb88320), (uint32_t)(0x9abfb3b6), (uint32_t)(0x03b6e20c), (uint32_t)(0x74b1d29a),
    (uint32_t)(0xead54739), (uint32_t)(0x9dd277af), (uint32_t)(0x04db2615), (uint32_t)(0x73dc1683),
    (uint32_t)(0xe3630b12), (uint32_t)(0x94643b84), (uint32_t)(0x0d6d6a3e), (uint32_t)(0x7a6a5aa8),
    (uint32_t)(0xe40ecf0b), (uint32_t)(0x9309ff9d), (uint32_t)(0x0a00ae27), (uint32_t)(0x7d079eb1),
    (uint32_t)(0xf00f9344), (uint32_t)(0x8708a3d2), (uint32_t)(0x1e01f268), (uint32_t)(0x6906c2fe),
    (uint32_t)(0xf762575d), (uint32_t)(0x806567cb), (uint32_t)(0x196c3671), (uint32_t)(0x6e6b06e7),
    (uint32_t)(0xfed41b76), (uint32_t)(0x89d32be0), (uint32_t)(0x10da7a5a), (uint32_t)(0x67dd4acc),
    (uint32_t)(0xf9b9df6f), (uint32_t)(0x8ebeeff9), (uint32_t)(0x17b7be43), (uint32_t)(0x60b08ed5),
    (uint32_t)(0xd6d6a3e8), (uint32_t)(0xa1d1937e), (uint32_t)(0x38d8c2c4), (uint32_t)(0x4fdff252),
    (uint32_t)(0xd1bb67f1), (uint32_t)(0xa6bc5767), (uint32_t)(0x3fb506dd), (uint32_t)(0x48b2364b),
    (uint32_t)(0xd80d2bda), (uint32_t)(0xaf0a1b4c), (uint32_t)(0x36034af6), (uint32_t)(0x41047a60),
    (uint32_t)(0xdf60efc3), (uint32_t)(0xa867df55), (uint32_t)(0x316e8eef), (uint32_t)(0x4669be79),
    (uint32_t)(0xcb61b38c), (uint32_t)(0xbc66831a), (uint32_t)(0x256fd2a0), (uint32_t)(0x5268e236),
    (uint32_t)(0xcc0c7795), (uint32_t)(0xbb0b4703), (uint32_t)(0x220216b9), (uint32_t)(0x5505262f),
    (uint32_t)(0xc5ba3bbe), (uint32_t)(0xb2bd0b28), (uint32_t)(0x2bb45a92), (uint32_t)(0x5cb36a04),
    (uint32_t)(0xc2d7ffa7), (uint32_t)(0xb5d0cf31), (uint32_t)(0x2cd99e8b), (uint32_t)(0x5bdeae1d),
    (uint32_t)(0x9b64c2b0), (uint32_t)(0xec63f226), (uint32_t)(0x756aa39c), (uint32_t)(0x026d930a),
    (uint32_t)(0x9c0906a9), (uint32_t)(0xeb0e363f), (uint32_t)(0x72076785), (uint32_t)(0x05005713),
    (uint32_t)(0x95bf4a82), (uint32_t)(0xe2b87a14), (uint32_t)(0x7bb12bae), (uint32_t)(0x0cb61b38),
    (uint32_t)(0x92d28e9b), (uint32_t)(0xe5d5be0d), (uint32_t)(0x7cdcefb7), (uint32_t)(0x0bdbdf21),
    (uint32_t)(0x86d3d2d4), (uint32_t)(0xf1d4e242), (uint32_t)(0x68ddb3f8), (uint32_t)(0x1fda836e),
    (uint32_t)(0x81be16cd), (uint32_t)(0xf6b9265b), (uint32_t)(0x6fb077e1), (uint32_t)(0x18b74777),
    (uint32_t)(0x88085ae6), (uint32_t)(0xff0f6a70), (uint32_t)(0x66063bca), (uint32_t)(0x11010b5c),
    (uint32_t)(0x8f659eff), (uint32_t)(0xf862ae69), (uint32_t)(0x616bffd3), (uint32_t)(0x166ccf45),
    (uint32_t)(0xa00ae278), (uint32_t)(0xd70dd2ee), (uint32_t)(0x4e048354), (uint32_t)(0x3903b3c2),
    (uint32_t)(0xa7672661), (uint32_t)(0xd06016f7), (uint32_t)(0x4969474d), (uint32_t)(0x3e6e77db),
    (uint32_t)(0xaed16a4a), (uint32_t)(0xd9d65adc), (uint32_t)(0x40df0b66), (uint32_t)(0x37d83bf0),
    (uint32_t)(0xa9bcae53), (uint32_t)(0xdebb9ec5), (uint32_t)(0x47b2cf7f), (uint32_t)(0x30b5ffe9),
    (uint32_t)(0xbdbdf21c), (uint32_t)(0xcabac28a), (uint32_t)(0x53b39330), (uint32_t)(0x24b4a3a6),
    (uint32_t)(0xbad03605), (uint32_t)(0xcdd70693), (uint32_t)(0x54de5729), (uint32_t)(0x23d967bf),
    (uint32_t)(0xb3667a2e), (uint32_t)(0xc4614ab8), (uint32_t)(0x5d681b02), (uint32_t)(0x2a6f2b94),
    (uint32_t)(0xb40bbe37), (uint32_t)(0xc30c8ea1), (uint32_t)(0x5a05df1b), (uint32_t)(0x2d02ef8d)
};

/* clang-format on */

/* DEFINITIONS ****************************************************************/

uint32_t
cardano_checksum_crc32(const byte_t* data, const size_t size)
{
  if ((data == NULL) || (size == 0U))
  {
    return 0;
  }

  uint32_t crc = 0xFFFFFFFFU;

  for (size_t i = 0; i < size; ++i)
  {
    byte_t k = (data)[i];
    crc      = s_crc32_table[((byte_t)(crc) ^ k) & 0xFFU] ^ (crc >> 8);
  }
  return (crc ^ 0xFFFFFFFF);
}