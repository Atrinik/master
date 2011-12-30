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
 * Packet construction/management.
 *
 * @author Alex Tokar */

#include <global.h>
#include <zlib.h>

/**
 * The packets memory pool. */
static mempool_struct *pool_packets;

/**
 * Initialize the packet API.
 * @internal */
void toolkit_packet_init(void)
{
	TOOLKIT_INIT_FUNC_START(packet)
	{
		toolkit_import(mempool);
		pool_packets = mempool_create("packets", PACKET_EXPAND, sizeof(packet_struct), 0, NULL, NULL, NULL, NULL);
	}
	TOOLKIT_INIT_FUNC_END()
}

/**
 * Deinitialize the packet API.
 * @internal */
void toolkit_packet_deinit(void)
{
	mempool_free(pool_packets);
}

/**
 * Allocates a new packet.
 * @param type The packet's command type.
 * @param size Initial number of bytes to allocate for the packet's
 * data.
 * @param expand The minimum size to expand by when there is not enough
 * bytes allocated.
 * @return The allocated packet. */
packet_struct *packet_new(uint8 type, size_t size, size_t expand)
{
	packet_struct *packet;

	packet = get_poolchunk(pool_packets);
	packet->next = packet->prev = NULL;
	packet->pos = 0;
	packet->size = size;
	packet->expand = expand;
	packet->len = 0;
	packet->data = NULL;

	/* Allocate the initial data block. */
	if (packet->size)
	{
		packet->data = malloc(packet->size);
	}

	packet->ndelay = 0;
	packet->type = type;

	return packet;
}

/**
 * Free a previously allocated data packet.
 * @param packet Packet to free. */
void packet_free(packet_struct *packet)
{
	if (packet->data)
	{
		free(packet->data);
	}

	return_poolchunk(packet, pool_packets);
}

/**
 * Compress a data packet, if possible.
 * @param packet Packet to try to compress. */
void packet_compress(packet_struct *packet)
{
#if defined(COMPRESS_DATA_PACKETS) && COMPRESS_DATA_PACKETS
	if (packet->len > COMPRESS_DATA_PACKETS_SIZE && packet->type != CLIENT_CMD_DATA)
	{
		size_t new_size = compressBound(packet->len);
		uint8 *dest;

		dest = malloc(new_size + 5);
		dest[0] = packet->type;
		/* Add original length of the packet. */
		dest[1] = (packet->len >> 24) & 0xff;
		dest[2] = (packet->len >> 16) & 0xff;
		dest[3] = (packet->len >> 8) & 0xff;
		dest[4] = (packet->len) & 0xff;
		packet->size = new_size + 5;
		/* Compress it. */
		compress2((Bytef *) dest + 5, (uLong *) &new_size, (const unsigned char FAR *) packet->data, packet->len, Z_BEST_COMPRESSION);

		free(packet->data);
		packet->data = dest;
		packet->len = new_size + 5;
		packet->type = CLIENT_CMD_COMPRESSED;
	}
#else
	(void) packet;
#endif
}

/**
 * Enables NDELAY on the specified packet. */
void packet_enable_ndelay(packet_struct *packet)
{
	packet->ndelay = 1;
}

void packet_set_pos(packet_struct *packet, size_t pos)
{
	packet->len = pos;
}

size_t packet_get_pos(packet_struct *packet)
{
	return packet->len;
}

packet_struct *packet_dup(packet_struct *packet)
{
	packet_struct *cp;

	cp = packet_new(packet->type, packet->size, packet->expand);
	cp->ndelay = packet->ndelay;
	packet_append_data_len(cp, packet->data, packet->len);

	return cp;
}

void packet_delete(packet_struct *packet, size_t pos, size_t len)
{
	if (packet->len - len + pos)
	{
		memmove(packet->data + pos, packet->data + pos + len, packet->len - len + pos);
	}

	packet->len -= len;
}

/**
 * Ensure 'size' bytes are available for writing in the packet. If not,
 * will allocate more.
 * @param packet Packet.
 * @param size How many bytes we need. */
static void packet_ensure(packet_struct *packet, size_t size)
{
	if (packet->len + size < packet->size)
	{
		return;
	}

	packet->size += MAX(packet->expand, size);
	packet->data = realloc(packet->data, packet->size);

	if (!packet->data)
	{
		LOG(llevError, "packet_ensure(): Out of memory.\n");
	}
}

void packet_merge(packet_struct *src, packet_struct *dst)
{
	packet_append_data_len(dst, src->data, src->len);
}

void packet_append_uint8(packet_struct *packet, uint8 data)
{
	packet_ensure(packet, 1);

	packet->data[packet->len++] = data;
}

void packet_append_sint8(packet_struct *packet, sint8 data)
{
	packet_ensure(packet, 1);

	packet->data[packet->len++] = data;
}

void packet_append_uint16(packet_struct *packet, uint16 data)
{
	packet_ensure(packet, 2);

	packet->data[packet->len++] = (data >> 8) & 0xff;
	packet->data[packet->len++] = data & 0xff;
}

void packet_append_sint16(packet_struct *packet, sint16 data)
{
	packet_ensure(packet, 2);

	packet->data[packet->len++] = (data >> 8) & 0xff;
	packet->data[packet->len++] = data & 0xff;
}

void packet_append_uint32(packet_struct *packet, uint32 data)
{
	packet_ensure(packet, 4);

	packet->data[packet->len++] = (data >> 24) & 0xff;
	packet->data[packet->len++] = (data >> 16) & 0xff;
	packet->data[packet->len++] = (data >> 8) & 0xff;
	packet->data[packet->len++] = data & 0xff;
}

void packet_append_sint32(packet_struct *packet, sint32 data)
{
	packet_ensure(packet, 4);

	packet->data[packet->len++] = (data >> 24) & 0xff;
	packet->data[packet->len++] = (data >> 16) & 0xff;
	packet->data[packet->len++] = (data >> 8) & 0xff;
	packet->data[packet->len++] = data & 0xff;
}

void packet_append_uint64(packet_struct *packet, uint64 data)
{
	packet_ensure(packet, 8);

	packet->data[packet->len++] = (data >> 56) & 0xff;
	packet->data[packet->len++] = (data >> 48) & 0xff;
	packet->data[packet->len++] = (data >> 40) & 0xff;
	packet->data[packet->len++] = (data >> 32) & 0xff;
	packet->data[packet->len++] = (data >> 24) & 0xff;
	packet->data[packet->len++] = (data >> 16) & 0xff;
	packet->data[packet->len++] = (data >> 8) & 0xff;
	packet->data[packet->len++] = data & 0xff;
}

void packet_append_sint64(packet_struct *packet, sint64 data)
{
	packet_ensure(packet, 8);

	packet->data[packet->len++] = (data >> 56) & 0xff;
	packet->data[packet->len++] = (data >> 48) & 0xff;
	packet->data[packet->len++] = (data >> 40) & 0xff;
	packet->data[packet->len++] = (data >> 32) & 0xff;
	packet->data[packet->len++] = (data >> 24) & 0xff;
	packet->data[packet->len++] = (data >> 16) & 0xff;
	packet->data[packet->len++] = (data >> 8) & 0xff;
	packet->data[packet->len++] = data & 0xff;
}

void packet_append_data_len(packet_struct *packet, uint8 *data, size_t len)
{
	if (!data)
	{
		return;
	}

	packet_ensure(packet, len);
	memcpy(packet->data + packet->len, data, len);
	packet->len += len;
}

void packet_append_string(packet_struct *packet, const char *data)
{
	packet_append_data_len(packet, (uint8 *) data, strlen(data));
}

void packet_append_string_terminated(packet_struct *packet, const char *data)
{
	packet_append_string(packet, data);
	packet_append_uint8(packet, '\0');
}

uint8 packet_to_uint8(uint8 *data, size_t len, size_t *pos)
{
	uint8 ret;

	if (len - *pos < 1)
	{
		*pos = len;
		return 0;
	}

	ret = data[*pos];
	*pos += 1;

	return ret;
}

sint8 packet_to_sint8(uint8 *data, size_t len, size_t *pos)
{
	sint8 ret;

	if (len - *pos < 1)
	{
		*pos = len;
		return 0;
	}

	ret = data[*pos];
	*pos += 1;

	return ret;
}

uint16 packet_to_uint16(uint8 *data, size_t len, size_t *pos)
{
	uint16 ret;

	if (len - *pos < 2)
	{
		*pos = len;
		return 0;
	}

	ret = (data[*pos] << 8) + data[*pos + 1];
	*pos += 2;

	return ret;
}

sint16 packet_to_sint16(uint8 *data, size_t len, size_t *pos)
{
	sint16 ret;

	if (len - *pos < 2)
	{
		*pos = len;
		return 0;
	}

	ret = (data[*pos] << 8) + data[*pos + 1];
	*pos += 2;

	return ret;
}

uint32 packet_to_uint32(uint8 *data, size_t len, size_t *pos)
{
	uint32 ret;

	if (len - *pos < 4)
	{
		*pos = len;
		return 0;
	}

	ret = (data[*pos] << 24) + (data[*pos + 1] << 16) + (data[*pos + 2] << 8) + data[*pos + 3];
	*pos += 4;

	return ret;
}

sint32 packet_to_sint32(uint8 *data, size_t len, size_t *pos)
{
	sint32 ret;

	if (len - *pos < 4)
	{
		*pos = len;
		return 0;
	}

	ret = (data[*pos] << 24) + (data[*pos + 1] << 16) + (data[*pos + 2] << 8) + data[*pos + 3];
	*pos += 4;

	return ret;
}

uint64 packet_to_uint64(uint8 *data, size_t len, size_t *pos)
{
	uint64 ret;

	if (len - *pos < 8)
	{
		*pos = len;
		return 0;
	}

	ret = ((uint64) data[*pos] << 56) + ((uint64) data[*pos + 1] << 48) + ((uint64) data[*pos + 2] << 40) + ((uint64) data[*pos + 3] << 32) + ((uint64) data[*pos + 4] << 24) + ((uint64) data[*pos + 5] << 16) + ((uint64) data[*pos + 6] << 8) + (uint64) data[*pos + 7];
	*pos += 8;

	return ret;
}

sint64 packet_to_sint64(uint8 *data, size_t len, size_t *pos)
{
	sint64 ret;

	if (len - *pos < 8)
	{
		*pos = len;
		return 0;
	}

	ret = ((sint64) data[*pos] << 56) + ((sint64) data[*pos + 1] << 48) + ((sint64) data[*pos + 2] << 40) + ((sint64) data[*pos + 3] << 32) + ((sint64) data[*pos + 4] << 24) + ((sint64) data[*pos + 5] << 16) + ((sint64) data[*pos + 6] << 8) + (sint64) data[*pos + 7];
	*pos += 8;

	return ret;
}

char *packet_to_string(uint8 *data, size_t len, size_t *pos, char *dest, size_t dest_size)
{
	size_t i = 0;
	char c;

	while (*pos < len && (c = (char) (data[(*pos)++])))
	{
		if (i < dest_size - 1)
		{
			dest[i++] = c;
		}
	}

	dest[i] = '\0';
	return dest;
}

void packet_to_stringbuffer(uint8 *data, size_t len, size_t *pos, StringBuffer *sb)
{
	char *str;

	str = (char *) (data + *pos);
	stringbuffer_append_string(sb, str);
	*pos += strlen(str) + 1;
}