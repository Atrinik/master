/*************************************************************************
 *           Atrinik, a Multiplayer Online Role Playing Game             *
 *                                                                       *
 *   Copyright (C) 2009-2014 Alex Tokar and Atrinik Development Team     *
 *                                                                       *
 * Fork from Crossfire (Multiplayer game for X-windows).                 *
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
 * Memory pools API header file.
 */

#ifndef MEMPOOL_H
#define MEMPOOL_H

/**
 * Minimalistic memory management data for a single chunk of memory.
 *
 * It is up to the application to keep track of which pool it belongs
 * to.
 */
typedef struct mempool_chunk_struct {
    /* This struct must always be padded for longword alignment of the data
     * coming behind it.
     *
     * Not a problem as long as we only keep a single pointer here, but be
     * careful when adding more data. */

    /**
     * Used for the free list and the limbo list. NULL if this memory chunk has
     * been allocated and is in use. */
    struct mempool_chunk_struct *next;
} mempool_chunk_struct;

/**
 * Optional initialisator to be called when expanding.
 */
typedef void (*chunk_initialisator)(void *ptr);
/**
 * Optional deinitialisator to be called when freeing.
 */
typedef void (*chunk_deinitialisator)(void *ptr);
/**
 * Optional constructor to be called when getting chunks.
 */
typedef void (*chunk_constructor)(void *ptr);
/**
 * Optional destructor to be called when returning chunks.
 */
typedef void (*chunk_destructor)(void *ptr);
/**
 * Optional debugger to be called when debugging chunks.
 */
typedef void (*chunk_debugger)(void *ptr, char *buf, size_t size);

/* Definitions used for array handling */
#define MEMPOOL_NROF_FREELISTS 8

/** Data for a single memory pool */
typedef struct mempool_struct {
    /**
     * Description of chunks. Mostly for debugging.
     */
    const char *chunk_description;

    /**
     * How many chunks to allocate at each expansion.
     */
    size_t expand_size;

    /**
     * Size of chunks, excluding sizeof(mempool_chunk) and padding.
     */
    size_t chunksize;

    /**
     * Special handling flags. See @ref mempool_flags
     */
    uint32 flags;

    /**
     * First free chunk.
     */
    mempool_chunk_struct *freelist[MEMPOOL_NROF_FREELISTS];

    /**
     * Number of free.
     */
    size_t nrof_free[MEMPOOL_NROF_FREELISTS];

    /**
     * Number of allocated.
     */
    size_t nrof_allocated[MEMPOOL_NROF_FREELISTS];

#ifndef NDEBUG
    /**
     * Holds all the allocated chunks in an array of pointers.
     */
    mempool_chunk_struct **chunks;

    /**
     * Number of allocated ::chunks.
     */
    size_t chunks_num;
#endif

    chunk_initialisator initialisator; ///< @copydoc chunk_initialisator
    chunk_deinitialisator deinitialisator; ///< @copydoc chunk_deinitialisator
    chunk_constructor constructor; ///< @copydoc chunk_constructor
    chunk_destructor destructor; ///< @copydoc chunk_destructor

#ifndef NDEBUG
    chunk_debugger debugger; ///< @copydoc chunk_debugger;
#endif
} mempool_struct;

/**
 * Get the memory management struct for a chunk of memory
 */
#define MEM_POOLDATA(ptr) (((mempool_chunk_struct *)(ptr)) - 1)
/**
 * Get the actual user data area from a mempool reference
 */
#define MEM_USERDATA(ptr) ((void *)(((mempool_chunk_struct *)(ptr)) + 1))
/**
 * Check that a chunk of memory is in the free (or removed for objects) list
 */
#define CHUNK_FREE(ptr) (MEM_POOLDATA(ptr)->next != NULL)

/**
 * @defgroup mempool_flags Mempool flags
 * Mempool flags.
 *@{*/

/** Don't use pooling, but only malloc/free instead */
#define MEMPOOL_BYPASS_POOLS  1
/*@}*/

#define get_poolchunk(_pool) get_poolchunk_array_real((_pool), 0)
#define get_poolarray(_pool, _arraysize) get_poolchunk_array_real((_pool), nearest_pow_two_exp(_arraysize))

#define return_poolchunk(_pool, _data) return_poolchunk_array_real((_pool), 0, (_data))
#define return_poolarray(_pool, _arraysize, _data) return_poolchunk_array_real((_pool), nearest_pow_two_exp(_arraysize), (_data))

#endif
