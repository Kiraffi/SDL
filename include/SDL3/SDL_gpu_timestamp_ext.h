/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* This is a project-specific experimental extension, not an upstream SDL API. */

#ifndef SDL_gpu_timestamp_ext_h_
#define SDL_gpu_timestamp_ext_h_

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>

#include <SDL3/SDL_begin_code.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CARP_SDL_GPU_TIMESTAMP_EXT_VERSION 1

typedef struct CARP_SDL_GPUTimestampQueryPool CARP_SDL_GPUTimestampQueryPool;

typedef struct CARP_SDL_GPUTimestampProperties
{
    double nanoseconds_per_tick;
    Uint32 valid_bits;
} CARP_SDL_GPUTimestampProperties;

/**
 * Queries timestamp support and conversion properties for a GPU device.
 *
 * \param device the GPU device to query.
 * \param out_properties the timestamp properties to populate.
 * \returns true when timestamps are supported, or false on failure; call
 *          SDL_GetError() for more information.
 *
 * \since This function is available in CARP SDL GPU timestamp extension 1.
 */
extern SDL_DECLSPEC bool SDLCALL CARP_SDL_GetGPUTimestampProperties(
    SDL_GPUDevice *device,
    CARP_SDL_GPUTimestampProperties *out_properties);

/**
 * Creates a fixed-size GPU timestamp query pool.
 *
 * \param device the GPU device that will own the query pool.
 * \param query_count the number of timestamp query slots to allocate.
 * \returns a query pool on success, or NULL on failure; call SDL_GetError()
 *          for more information.
 *
 * \since This function is available in CARP SDL GPU timestamp extension 1.
 */
extern SDL_DECLSPEC CARP_SDL_GPUTimestampQueryPool *SDLCALL CARP_SDL_CreateGPUTimestampQueryPool(
    SDL_GPUDevice *device,
    Uint32 query_count);

/**
 * Releases a timestamp query pool when submitted uses have completed.
 *
 * \param device the GPU device that owns the query pool.
 * \param query_pool the timestamp query pool to release.
 *
 * \since This function is available in CARP SDL GPU timestamp extension 1.
 */
extern SDL_DECLSPEC void SDLCALL CARP_SDL_ReleaseGPUTimestampQueryPool(
    SDL_GPUDevice *device,
    CARP_SDL_GPUTimestampQueryPool *query_pool);

/**
 * Resets and writes one timestamp outside any active GPU pass.
 *
 * \param command_buffer the command buffer that records the timestamp.
 * \param query_pool the timestamp query pool to write.
 * \param query_index the query slot to reset and write.
 * \returns true on success, or false on failure; call SDL_GetError() for
 *          more information.
 *
 * \since This function is available in CARP SDL GPU timestamp extension 1.
 */
extern SDL_DECLSPEC bool SDLCALL CARP_SDL_WriteGPUTimestamp(
    SDL_GPUCommandBuffer *command_buffer,
    CARP_SDL_GPUTimestampQueryPool *query_pool,
    Uint32 query_index);

/**
 * Copies tightly packed 64-bit timestamp results into a GPU buffer.
 *
 * \param copy_pass the active copy pass that records the result copy.
 * \param query_pool the timestamp query pool to copy.
 * \param first_query the first query slot to copy.
 * \param query_count the number of query slots to copy.
 * \param destination the destination GPU buffer.
 * \param destination_offset the aligned byte offset in the destination.
 * \returns true on success, or false on failure; call SDL_GetError() for
 *          more information.
 *
 * \since This function is available in CARP SDL GPU timestamp extension 1.
 */
extern SDL_DECLSPEC bool SDLCALL CARP_SDL_CopyGPUTimestampResults(
    SDL_GPUCopyPass *copy_pass,
    CARP_SDL_GPUTimestampQueryPool *query_pool,
    Uint32 first_query,
    Uint32 query_count,
    SDL_GPUBuffer *destination,
    Uint32 destination_offset);

#ifdef __cplusplus
}
#endif
#include <SDL3/SDL_close_code.h>

#endif /* SDL_gpu_timestamp_ext_h_ */
