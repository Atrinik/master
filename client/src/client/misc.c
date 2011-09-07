/************************************************************************
*            Atrinik, a Multiplayer Online Role Playing Game            *
*                                                                       *
*    Copyright (C) 2009-2011 Alex Tokar and Atrinik Development Team    *
*                                                                       *
* Fork from Daimonin (Massive Multiplayer Online Role Playing Game)     *
* and Crossfire (Multiplayer game for X-windows).                       *
*                                                                       *
* This program is free software; you can redistribute it and/or modify  *
* it under the terms of the GNU General Public License as published by  *
* the Free Software Foundation; either version 2 of the License, or     *
* (at your option) any later version.                                   *
*                                                                       *
* This program is distributed in the hope that it will be useful,       *
* but WITHOUT ANY WARRANTY; without even the implied warranty of        *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
* GNU General Public License for more details.                          *
*                                                                       *
* You should have received a copy of the GNU General Public License     *
* along with this program; if not, write to the Free Software           *
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.             *
*                                                                       *
* The author can be reached at admin@atrinik.org                        *
************************************************************************/

/**
 * @file
 * Miscellaneous functions. */

#include <global.h>

/**
 * Computes the integer square root.
 * @param n Number of which to compute the root.
 * @return Integer square root. */
unsigned long isqrt(unsigned long n)
{
	unsigned long op = n, res = 0, one;

	/* "one" starts at the highest power of four <= than the argument. */
	one = 1 << 30;

	while (one > op)
	{
		one >>= 2;
	}

	while (one != 0)
	{
		if (op >= res + one)
		{
			op -= res + one;
			/* Faster than 2 * one. */
			res += one << 1;
		}

		res >>= 1;
		one >>= 2;
	}

	return res;
}

/**
 * Splits a string delimited by passed in sep value into characters into an array of strings.
 * @param str The string to be split; will be modified.
 * @param array The string array; will be filled with pointers into str.
 * @param array_size The number of elements in array; if <code>str</code> contains more fields
 * excess fields are not split but included into the last element.
 * @param sep Separator to use.
 * @return The number of elements found; always less or equal to <code>array_size</code>. */
size_t split_string(char *str, char *array[], size_t array_size, char sep)
{
	char *p;
	size_t pos;

	if (array_size <= 0)
	{
		return 0;
	}

	if (*str == '\0')
	{
		array[0] = str;
		return 1;
	}

	pos = 0;
	p = str;

	while (pos < array_size)
	{
		array[pos++] = p;

		while (*p != '\0' && *p != sep)
		{
			p++;
		}

		if (pos >= array_size)
		{
			break;
		}

		if (*p != sep)
		{
			break;
		}

		*p++ = '\0';
	}

	return pos;
}

/**
 * Like realloc(), but if more bytes are being allocated, they get set to
 * 0 using memset().
 * @param ptr Original pointer.
 * @param old_size Size of the pointer.
 * @param new_size New size the pointer should have.
 * @return Resized pointer, NULL on failure. */
void *reallocz(void *ptr, size_t old_size, size_t new_size)
{
	void *new_ptr = realloc(ptr, new_size);

	if (new_ptr && new_size > old_size)
	{
		memset(((char *) new_ptr) + old_size, 0, new_size - old_size);
	}

	return new_ptr;
}

/**
 * Replaces "\n" by a newline char.
 *
 * Since we are replacing 2 chars by 1, no overflow should happen.
 * @param line Text to replace into. */
void convert_newline(char *str)
{
	char *next, buf[HUGE_BUF * 10];

	while ((next = strstr(str, "\\n")))
	{
		*next = '\n';
		*(next + 1) = '\0';
		snprintf(buf, sizeof(buf), "%s%s", str, next + 2);
		strcpy(str, buf);
	}
}

/**
 * Opens an url in the system's default browser.
 * @param url URL to open. */
void browser_open(const char *url)
{
#if defined(LINUX)
	char buf[HUGE_BUF];

	snprintf(buf, sizeof(buf), "xdg-open \"%s\"", url);

	if (system(buf) != 0)
	{
		snprintf(buf, sizeof(buf), "x-www-browser \"%s\"", url);

		if (system(buf) != 0)
		{
			LOG(llevBug, "browser_open(): Could not open '%s'.\n", url);
		}
	}
#elif defined(WIN32)
	ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWDEFAULT);
#else
	LOG(llevDebug, "browser_open(): Unknown platform, cannot open '%s'.\n", url);
#endif
}

/**
 * Calculates a random number between min and max.
 *
 * It is suggested one uses this function rather than RANDOM()%, as it
 * would appear that a number of off-by-one-errors exist due to improper
 * use of %.
 *
 * This should also prevent SIGFPE.
 * @param min Starting range.
 * @param max Ending range.
 * @return The random number. */
int rndm(int min, int max)
{
	if (max < 1 || max - min + 1 < 1)
	{
		LOG(llevBug, "BUG: Calling rndm() with min=%d max=%d\n", min, max);
		return min;
	}

	return min + RANDOM() / (RAND_MAX / (max - min + 1) + 1);
}

/**
 * Get the full package version as string.
 *
 * If patch version is 0, it will not be appended to the version string.
 * @param dst Where to store the version.
 * @param dstlen Size of dst.
 * @return 'dst'. */
char *package_get_version_full(char *dst, size_t dstlen)
{
#if PACKAGE_VERSION_PATCH == 0
	package_get_version_partial(dst, dstlen);
#else
	snprintf(dst, dstlen, "%d.%d.%d", PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCH);
#endif
	return dst;
}

/**
 * Get the partial package version. This means that the patch version
 * will not be included, even if it's not 0.
 * @param dst Where to store the version.
 * @param dstlen Size of dst.
 * @return 'dst' */
char *package_get_version_partial(char *dst, size_t dstlen)
{
	/* Upgrader version will overrule the package version if the upgrader
	 * is currently doing its job. */
	if (upgrader_get_version_partial(dst, dstlen))
	{
		return dst;
	}

	snprintf(dst, dstlen, "%d.%d", PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR);
	return dst;
}

/**
 * Transforms a string to uppercase, in-place.
 * @param str String to transform, will be modified. */
void strtoupper(char *str)
{
	while (*str != '\0')
	{
		*str++ = toupper(*str);
	}
}

/**
 * Transforms a string to lowercase, in-place.
 * @param str String to transform, will be modified. */
void strtolower(char *str)
{
	while (*str != '\0')
	{
		*str++ = tolower(*str);
	}
}
