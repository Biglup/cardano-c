/**
 * \file export.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_EXPORT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_EXPORT_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef CARDANO_EXPORT
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define CARDANO_EXPORT EMSCRIPTEN_KEEPALIVE extern
#elif defined(_MSC_VER) && defined(CARDANO_C_DLL)
#define CARDANO_EXPORT __declspec(dllexport) extern
#else
#define CARDANO_EXPORT extern
#endif
#endif
#ifdef __GNUC__
#define CARDANO_NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define CARDANO_NODISCARD
#else
#define CARDANO_NODISCARD
#endif

#define CARDANO_UNUSED(x) (void)(x)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_EXPORT_H */
