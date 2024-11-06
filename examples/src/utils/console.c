/**
 * \file console.c
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

/* INCLUDES ******************************************************************/

#include "console.h"
#include "utils.h"
#include <cardano/typedefs.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

/* CONSTANTS *****************************************************************/

static const char* FOREGROUND_COLOR_FORMAT = "\033[3%dm";
static const char* BACKGROUND_COLOR_FORMAT = "\033[4%dm";
static const char* LOG_SEVERITY_ENV        = "LOG_SEVERITY";
static const char* LOG_SEVERITY_INFO_STR   = "info";
static const char* LOG_SEVERITY_ERROR_STR  = "error";
static const char* LOG_SEVERITY_WARN_STR   = "warn";
static const char* LOG_SEVERITY_DEBUG_STR  = "debug";

static const uint8_t LOG_SEVERITY_UNKNOWN = 0;
static const uint8_t LOG_SEVERITY_INFO    = 1;
static const uint8_t LOG_SEVERITY_ERROR   = 2;
static const uint8_t LOG_SEVERITY_WARN    = 3;
static const uint8_t LOG_SEVERITY_DEBUG   = 4;

/* STATIC VARIABLES **********************************************************/

static console_color_t g_foreground_color = CONSOLE_COLOR_DEFAULT;
static console_color_t g_background_color = CONSOLE_COLOR_DEFAULT;

/* STATIC FUNCTIONS **********************************************************/

/**
 * Sets the current terminal color.
 */
static void
set_color()
{
  printf(FOREGROUND_COLOR_FORMAT, g_foreground_color);
  printf(BACKGROUND_COLOR_FORMAT, g_background_color);
  fflush(stdout);
}

/**
 * Resets the current terminal color to default.
 */
static void
reset_color()
{
  printf(FOREGROUND_COLOR_FORMAT, CONSOLE_COLOR_DEFAULT);
  printf(BACKGROUND_COLOR_FORMAT, CONSOLE_COLOR_DEFAULT);
  fflush(stdout);
}

/**
 * Writes a color-coded log line to the terminal.
 *
 * \param color The color to be used to render the log line.
 * \param format printf style format.
 * \param ... variadic parameters to be used in the format.
 */
static void
write_log_line(console_color_t color, const char* format, va_list args)
{
  g_foreground_color = color;
  set_color();

  vprintf(format, args);
  va_end(args);

  printf("\n");
  reset_color();
}

/**
 * Writes a color-coded log line to the terminal.
 *
 * \param color The color to be used to render the log line.
 * \param message The message to be printed.
 */
static void
write_log_lineMessage(console_color_t color, const char* message)
{
  g_foreground_color = color;
  set_color();

  printf("%s\n", message);
  reset_color();
}

/**
 * Gets log severity from the environment.
 *
 * \return The log severity.
 */
uint8_t
get_log_severity()
{
  static uint8_t log_severity = LOG_SEVERITY_UNKNOWN;

  if (log_severity == LOG_SEVERITY_UNKNOWN)
  {
    const char* env_str = getenv(LOG_SEVERITY_ENV);

    if (env_str == NULL)
    {
      log_severity = LOG_SEVERITY_WARN;
      return log_severity;
    }

    if (strcmp(env_str, LOG_SEVERITY_INFO_STR) == 0)
    {
      log_severity = LOG_SEVERITY_INFO;
    }
    else if (strcmp(env_str, LOG_SEVERITY_ERROR_STR) == 0)
    {
      log_severity = LOG_SEVERITY_ERROR;
    }
    else if (strcmp(env_str, LOG_SEVERITY_DEBUG_STR) == 0)
    {
      log_severity = LOG_SEVERITY_DEBUG;
    }
    else
    {
      log_severity = LOG_SEVERITY_WARN; // Default severity
    }
  }

  return log_severity;
}

/* PUBLIC FUNCTIONS **********************************************************/

void
console_info(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  write_log_line(CONSOLE_COLOR_DEFAULT, format, args);
  va_end(args);
}

void
console_debug(const char* format, ...)
{
  if (get_log_severity() < LOG_SEVERITY_DEBUG)
  {
    return;
  }

  va_list args;
  va_start(args, format);
  write_log_line(CONSOLE_COLOR_BLUE, format, args);
  va_end(args);
}

void
console_warn(const char* format, ...)
{
  if (get_log_severity() < LOG_SEVERITY_WARN)
  {
    return;
  }

  va_list args;
  va_start(args, format);
  write_log_line(CONSOLE_COLOR_YELLOW, format, args);
  va_end(args);
}

void
console_error(const char* format, ...)
{
  if (get_log_severity() < LOG_SEVERITY_ERROR)
  {
    return;
  }

  va_list args;
  va_start(args, format);
  write_log_line(CONSOLE_COLOR_RED, format, args);
  va_end(args);
}

void
console_write(const char* format, ...)
{
  set_color();

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  reset_color();
}

void
console_write_line(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  write_log_line(CONSOLE_COLOR_DEFAULT, format, args);
  va_end(args);
}

void
console_set_background_color(console_color_t color)
{
  g_background_color = color;
}

void
console_set_foreground_color(console_color_t color)
{
  g_foreground_color = color;
}

console_color_t
console_get_background_color()
{
  return g_background_color;
}

console_color_t
console_get_foreground_color()
{
  return g_foreground_color;
}

void
console_reset_color()
{
  g_foreground_color = CONSOLE_COLOR_DEFAULT;
  g_background_color = CONSOLE_COLOR_DEFAULT;
  reset_color();
}

void
console_read_line(char* buffer, size_t max_length)
{
  if (fgets(buffer, (int32_t)max_length, stdin) == NULL)
  {
    buffer[0] = '\0';
  }
  else
  {
    size_t len = cardano_utils_safe_strlen(buffer, max_length);

    if (len > 0 && buffer[len - 1] == '\n')
    {
      buffer[len - 1] = '\0';
    }
  }
}

int
console_read_key()
{
#if defined(_WIN32) || defined(_WIN64)
  return _getch();
#else
  struct termios oldt, newt;
  int            ch;

  // Save old terminal settings
  if (tcgetattr(STDIN_FILENO, &oldt) != 0)
  {
    return -1;
  }
  newt = oldt;

  // Disable canonical mode and echo
  newt.c_lflag &= ~(ICANON | ECHO);

  // Set new terminal settings
  if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0)
  {
    return -1;
  }

  // Read character
  ch = getchar();

  // Restore old terminal settings
  if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) != 0)
  {
    return -1;
  }

  return ch;
#endif
}

int
console_read_password(char* buffer, const size_t max_length)
{
#if defined(_WIN32) || defined(_WIN64)
  size_t i = 0;
  int    c;

  while (i < max_length - 1)
  {
    c = _getch();

    if (c == '\r' || c == '\n')
    {
      break;
    }
    else if (c == '\b')
    {
      if (i > 0)
      {
        --i;
      }
    }
    else
    {
      buffer[i++] = c;
    }
  }
  buffer[i] = '\0';
  printf("\n");

  return (int32_t)i;
#else
  struct termios oldt, newt;
  size_t         i = 0;

  if (tcgetattr(STDIN_FILENO, &oldt) != 0)
  {
    return -1;
  }

  // Disable echo
  newt          = oldt;
  newt.c_lflag &= ~(ECHO);

  if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0)
  {
    return -1;
  }

  // Read password
  while (i < max_length - 1)
  {
    int c = getchar();

    if (c == '\n' || c == EOF)
    {
      break;
    }
    else if (c == '\b' || c == 127)
    {
      if (i > 0)
      {
        --i;
      }
    }
    else
    {
      buffer[i++] = (char)c;
    }
  }
  buffer[i] = '\0';

  if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) != 0)
  {
    return -1;
  }

  printf("\n");

  return (int32_t)i;
#endif
}