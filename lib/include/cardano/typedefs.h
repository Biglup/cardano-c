/**
 * \file typedefs.h
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
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

#ifndef CARDANO_TYPEDEFS_H_
#define CARDANO_TYPEDEFS_H_

/* INCLUDES ******************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* DEFINES ******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef float         float32_t;
typedef double        float64_t;
typedef unsigned char byte_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CARDANO_TYPEDEFS_H_ */
