/*
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include <math.h>

#include <SDL3/SDL_gpu_timestamp_ext.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>

int main(int argc, char *argv[])
{
    SDL_GPUDevice *device = NULL;
    CARP_SDL_GPUTimestampQueryPool *queryPool = NULL;
    SDL_GPUBuffer *resultBuffer = NULL;
    SDL_GPUBuffer *workBuffer = NULL;
    SDL_GPUTransferBuffer *downloadBuffer = NULL;
    SDL_GPUTransferBuffer *uploadBuffer = NULL;
    SDL_GPUFence *fence = NULL;
    int testResult = 1;

    (void)argc;
    (void)argv;

    if (CARP_SDL_GPU_TIMESTAMP_EXT_VERSION != 1) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Unexpected timestamp extension version");
        goto cleanup;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SKIP: SDL video initialization failed: %s", SDL_GetError());
        return 0;
    }

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
    if (device == NULL) {
        SDL_Log("SKIP: Vulkan GPU device unavailable: %s", SDL_GetError());
        testResult = 0;
        goto cleanup;
    }

    CARP_SDL_GPUTimestampProperties properties;
    if (!CARP_SDL_GetGPUTimestampProperties(device, &properties)) {
        SDL_Log("SKIP: Vulkan GPU timestamps unavailable: %s", SDL_GetError());
        testResult = 0;
        goto cleanup;
    }
    if (!isfinite(properties.nanoseconds_per_tick)
        || properties.nanoseconds_per_tick <= 0.0
        || properties.valid_bits == 0
        || properties.valid_bits > 64) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Invalid timestamp properties");
        goto cleanup;
    }

    queryPool = CARP_SDL_CreateGPUTimestampQueryPool(device, 2);
    if (queryPool == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not create timestamp query pool: %s", SDL_GetError());
        goto cleanup;
    }

    const SDL_GPUBufferCreateInfo gpuBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
        .size = 2 * sizeof(Uint64),
    };
    resultBuffer = SDL_CreateGPUBuffer(device, &gpuBufferInfo);
    workBuffer = SDL_CreateGPUBuffer(device, &gpuBufferInfo);

    const SDL_GPUTransferBufferCreateInfo downloadInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = 2 * sizeof(Uint64),
    };
    downloadBuffer = SDL_CreateGPUTransferBuffer(device, &downloadInfo);

    const SDL_GPUTransferBufferCreateInfo uploadInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = 2 * sizeof(Uint64),
    };
    uploadBuffer = SDL_CreateGPUTransferBuffer(device, &uploadInfo);
    if (resultBuffer == NULL || workBuffer == NULL
        || downloadBuffer == NULL || uploadBuffer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not create timestamp test buffers: %s", SDL_GetError());
        goto cleanup;
    }

    Uint64 *uploadData = SDL_MapGPUTransferBuffer(device, uploadBuffer, false);
    if (uploadData == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not map timestamp upload buffer: %s", SDL_GetError());
        goto cleanup;
    }
    uploadData[0] = 0x0123456789abcdefULL;
    uploadData[1] = 0xfedcba9876543210ULL;
    SDL_UnmapGPUTransferBuffer(device, uploadBuffer);

    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not acquire timestamp command buffer: %s", SDL_GetError());
        goto cleanup;
    }
    if (CARP_SDL_WriteGPUTimestamp(commandBuffer, queryPool, 2)) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Out-of-range timestamp write unexpectedly succeeded");
        SDL_CancelGPUCommandBuffer(commandBuffer);
        goto cleanup;
    }
    SDL_ClearError();

    if (!CARP_SDL_WriteGPUTimestamp(commandBuffer, queryPool, 0)) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not record first timestamp: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        goto cleanup;
    }

    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (CARP_SDL_WriteGPUTimestamp(commandBuffer, queryPool, 1)) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "In-pass timestamp write unexpectedly succeeded");
        SDL_EndGPUCopyPass(copyPass);
        SDL_CancelGPUCommandBuffer(commandBuffer);
        goto cleanup;
    }
    SDL_ClearError();
    const SDL_GPUTransferBufferLocation uploadSource = {
        .transfer_buffer = uploadBuffer,
        .offset = 0,
    };
    const SDL_GPUBufferRegion workDestination = {
        .buffer = workBuffer,
        .offset = 0,
        .size = 2 * sizeof(Uint64),
    };
    SDL_UploadToGPUBuffer(copyPass, &uploadSource, &workDestination, false);
    SDL_EndGPUCopyPass(copyPass);

    if (!CARP_SDL_WriteGPUTimestamp(commandBuffer, queryPool, 1)) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not record second timestamp: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        goto cleanup;
    }

    copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!CARP_SDL_CopyGPUTimestampResults(
            copyPass,
            queryPool,
            0,
            2,
            resultBuffer,
            0)) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not copy timestamp results: %s", SDL_GetError());
        SDL_EndGPUCopyPass(copyPass);
        SDL_CancelGPUCommandBuffer(commandBuffer);
        goto cleanup;
    }
    const SDL_GPUBufferRegion resultSource = {
        .buffer = resultBuffer,
        .offset = 0,
        .size = 2 * sizeof(Uint64),
    };
    const SDL_GPUTransferBufferLocation downloadDestination = {
        .transfer_buffer = downloadBuffer,
        .offset = 0,
    };
    SDL_DownloadFromGPUBuffer(copyPass, &resultSource, &downloadDestination);
    SDL_EndGPUCopyPass(copyPass);

    /* Release before submit to prove command-buffer query-pool retention. */
    CARP_SDL_ReleaseGPUTimestampQueryPool(device, queryPool);
    queryPool = NULL;

    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);
    if (fence == NULL || !SDL_WaitForGPUFences(device, true, &fence, 1)) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Timestamp submission failed: %s", SDL_GetError());
        goto cleanup;
    }

    const Uint64 *timestamps = SDL_MapGPUTransferBuffer(device, downloadBuffer, false);
    if (timestamps == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Could not map timestamp results: %s", SDL_GetError());
        goto cleanup;
    }
    Uint64 elapsedTicks = timestamps[1] - timestamps[0];
    if (properties.valid_bits < 64) {
        elapsedTicks &= (1ULL << properties.valid_bits) - 1ULL;
    }
    const double elapsedMs =
        (double)elapsedTicks * properties.nanoseconds_per_tick / 1000000.0;
    SDL_UnmapGPUTransferBuffer(device, downloadBuffer);
    if (!isfinite(elapsedMs) || elapsedMs < 0.0) {
        SDL_LogError(SDL_LOG_CATEGORY_TEST, "Invalid elapsed timestamp duration");
        goto cleanup;
    }

    SDL_Log("GPU timestamp extension measured %.6f ms", elapsedMs);
    testResult = 0;

cleanup:
    if (fence != NULL) {
        SDL_ReleaseGPUFence(device, fence);
    }
    if (queryPool != NULL) {
        CARP_SDL_ReleaseGPUTimestampQueryPool(device, queryPool);
    }
    if (uploadBuffer != NULL) {
        SDL_ReleaseGPUTransferBuffer(device, uploadBuffer);
    }
    if (downloadBuffer != NULL) {
        SDL_ReleaseGPUTransferBuffer(device, downloadBuffer);
    }
    if (workBuffer != NULL) {
        SDL_ReleaseGPUBuffer(device, workBuffer);
    }
    if (resultBuffer != NULL) {
        SDL_ReleaseGPUBuffer(device, resultBuffer);
    }
    if (device != NULL) {
        SDL_DestroyGPUDevice(device);
    }
    SDL_Quit();
    return testResult;
}
