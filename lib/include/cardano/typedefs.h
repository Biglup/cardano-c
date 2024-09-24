/**
 * \file typedefs.h
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TYPEDEFS_H_
#define BIGLUP_LABS_INCLUDE_CARDANO_TYPEDEFS_H_

/* INCLUDES ******************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* DEFINES ******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* -E> compliant MC3R1.D4.6 2 Type byte_t shall be used everywhere where a pointer to bytes is needed to follow Strict Aliasing Rule */
// More on the topic can be found here: https://gist.github.com/jibsen/da6be27cde4d526ee564
typedef unsigned char byte_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CARDANO_TYPEDEFS_H_ */
