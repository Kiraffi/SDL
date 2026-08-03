/*
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include <SDL3/SDL_gpu_timestamp_ext.h>
#include <SDL3/SDL_main.h>

static bool(SDLCALL *getTimestampProperties)(SDL_GPUDevice *, CARP_SDL_GPUTimestampProperties *) = CARP_SDL_GetGPUTimestampProperties;
static CARP_SDL_GPUTimestampQueryPool *(SDLCALL *createTimestampQueryPool)(SDL_GPUDevice *, Uint32) = CARP_SDL_CreateGPUTimestampQueryPool;
static void(SDLCALL *releaseTimestampQueryPool)(SDL_GPUDevice *, CARP_SDL_GPUTimestampQueryPool *) = CARP_SDL_ReleaseGPUTimestampQueryPool;
static bool(SDLCALL *writeTimestamp)(SDL_GPUCommandBuffer *, CARP_SDL_GPUTimestampQueryPool *, Uint32) = CARP_SDL_WriteGPUTimestamp;
static bool(SDLCALL *copyTimestampResults)(SDL_GPUCopyPass *, CARP_SDL_GPUTimestampQueryPool *, Uint32, Uint32, SDL_GPUBuffer *, Uint32) = CARP_SDL_CopyGPUTimestampResults;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (CARP_SDL_GPU_TIMESTAMP_EXT_VERSION != 1) {
        return 1;
    }

    return getTimestampProperties != NULL &&
                   createTimestampQueryPool != NULL &&
                   releaseTimestampQueryPool != NULL &&
                   writeTimestamp != NULL &&
                   copyTimestampResults != NULL
               ? 0
               : 1;
}
