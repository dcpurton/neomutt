/**
 * @file
 * Path manipulation functions
 *
 * @authors
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mutt/mutt.h"
#include "email/email.h"
#include "alias.h"
#include "globals.h"
#include "hook.h"
#include "imap/imap.h"
#include "mx.h"

/**
 * mutt_path_tidy_slash - Remove unnecessary slashes and dots
 * @param[in,out] buf Path to modify
 * @retval true Success
 *
 * Collapse repeated '//' and '/./'
 */
bool mutt_path_tidy_slash(char *buf)
{
  if (!buf)
    return false;

  char *r = buf;
  char *w = buf;

  while (*r != '\0')
  {
    *w++ = *r++;

    if (r[-1] == '/')
    {
      for (; (r[0] == '/') || (r[0] == '.'); r++)
      {
        if (r[0] == '/')
          continue;
        if (r[0] == '.')
        {
          if (r[1] == '/')
          {
            r++;
            continue;
          }
          else if (r[1] == '\0')
          {
            r[0] = '\0';
          }
          break;
        }
      }
    }
  }

  if ((w[-1] == '/') && (w != (buf + 1)))
    w--;

  *w = '\0';

  return true;
}

/**
 * mutt_path_tidy_dotdot - Remove dot-dot-slash from a path
 * @param[in,out] buf Path to modify
 * @retval true Success
 *
 * Collapse dot-dot patterns, like '/dir/../'
 */
bool mutt_path_tidy_dotdot(char *buf)
{
  if (!buf || (buf[0] != '/'))
    return false;

  char *dd = buf;

  mutt_debug(3, "Collapse path: %s\n", buf);
  while ((dd = strstr(dd, "/..")))
  {
    if (dd[3] == '/') /* paths follow dots */
    {
      char *dest = NULL;
      if (dd != buf) /* not at start of string */
      {
        dd[0] = '\0';
        dest = strrchr(buf, '/');
      }
      if (!dest)
        dest = buf;

      memmove(dest, dd + 3, strlen(dd + 3) + 1);
    }
    else if (dd[3] == '\0') /* dots at end of string */
    {
      if (dd == buf) /* at start of string */
      {
        dd[1] = '\0';
      }
      else
      {
        dd[0] = '\0';
        char *s = strrchr(buf, '/');
        if (s == buf)
          s[1] = '\0';
        else if (s)
          s[0] = '\0';
      }
    }
    else
    {
      dd += 3; /* skip over '/..dir/' */
      continue;
    }

    dd = buf; /* restart at the beginning */
  }

  mutt_debug(3, "Collapsed to:  %s\n", buf);
  return true;
}

/**
 * mutt_path_tidy - Remove unnecessary parts of a path
 * @param[in,out] buf Path to modify
 * @retval true Success
 *
 * Remove unnecessary dots and slashes from a path
 */
bool mutt_path_tidy(char *buf)
{
  if (!buf || (buf[0] != '/'))
    return false;

  if (!mutt_path_tidy_slash(buf))
    return false;

  return mutt_path_tidy_dotdot(buf);
}