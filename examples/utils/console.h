/**
 * \file console.h
 *
 * \author angel.castillo
 * \date   Oct 04, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CONSOLE_H
#define BIGLUP_LABS_INCLUDE_CONSOLE_H

/* ENUMERATIONS **************************************************************/

/**
 * \brief The console color.
 */
typedef enum
{
  /**
   * \brief Black color.
   */
  CONSOLE_COLOR_BLACK = 0x00,

  /**
   * \brief Red color.
   */
  CONSOLE_COLOR_RED = 0x01,

  /**
   * \brief Green color.
   */
  CONSOLE_COLOR_GREEN = 0x02,

  /**
   * \brief Yellow color.
   */
  CONSOLE_COLOR_YELLOW = 0x03,

  /**
   * \brief Blue color.
   */
  CONSOLE_COLOR_BLUE = 0x04,

  /**
   * \brief Purple color.
   */
  CONSOLE_COLOR_PURPLE = 0x05,

  /**
   * \brief Cyan color.
   */
  CONSOLE_COLOR_CYAN = 0x06,

  /**
   * \brief Light gray color.
   */
  CONSOLE_COLOR_LIGHT_GRAY = 0x07,

  /**
   * \brief Dark gray color.
   */
  CONSOLE_COLOR_DEFAULT = 0x09
} console_color_t;

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Writes the specified string value to the standard output stream
 * as a info message.
 *
 * \param format printf style format.
 * \param ... variadic parameters to be use in the format.
 */
void console_info(const char* format, ...);

/**
 * Writes the specified string value to the standard output stream
 * as a debug message.
 *
 * \param format printf style format.
 * \param ... variadic parameters to be use in the format.
 */
void console_debug(const char* format, ...);

/**
 * Writes the specified string value to the standard output stream
 * as a warn message.
 *
 * \param format printf style format.
 * \param ... variadic parameters to be use in the format.
 */
void console_warn(const char* format, ...);

/**
 * Writes the specified string value to the standard output stream
 * as a error message.
 *
 * \param format printf style format.
 * \param ... variadic parameters to be use in the format.
 */
void console_error(const char* format, ...);

/**
 * Writes the specified string value to the standard output stream.
 *
 * \param format printf style format.
 * \param ... variadic parameters to be use in the format.
 */
void console_write(const char* format, ...);

/**
 * Writes the specified string value to the standard output stream.
 *
 * \param format printf style format.
 * \param ... variadic parameters to be use in the format.
 */
void console_write_line(const char* format, ...);

/**
 * Sets the background color of the console.
 *
 * \param color A value that specifies the background color of the console; that is, the color that appears
 * behind each character. The default is black.
 */
void console_set_background_color(console_color_t color);

/**
 * Sets the foreground color of the console.
 *
 * \param color A ConsoleColor that specifies the foreground color of the console; that is, the color of each
 * character that is displayed. The default is gray.
 */
void console_set_foreground_color(console_color_t color);

/**
 * Gets the background color of the console.
 *
 * \return A ConsoleColor that specifies the current background color of the console.
 */
console_color_t console_get_background_color();

/**
 * Gets the foreground color of the console.
 *
 * \return A ConsoleColor that specifies the current foreground color of the console.
 */
console_color_t console_get_foreground_color();

/**
 * Sets the foreground and background console colors to their defaults.
 */
void console_reset_color();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CONSOLE_H