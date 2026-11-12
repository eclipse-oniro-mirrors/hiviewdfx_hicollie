/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "watchdog_sanitizer_unwind.h"

#if defined(__aarch64__)
#include <atomic>
#include <dlfcn.h>
#include <memory>
#include <mutex>
#include <string>

#include "fp_backtrace.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int GWP_ASAN_UNWIND_ABI_VERSION = 1;
constexpr const char* ASYNC_STACK_LIB = "libasync_stack.z.so";
constexpr size_t GWP_ASAN_MAX_DEPTH = 128;
constexpr size_t ASYNC_LINK_LAYER = 1;
constexpr size_t ASYNC_MAX_CONTENT = 16;
constexpr int SANITIZER_UNWIND_REGISTER_NO_CALLBACK = -2;
std::unique_ptr<OHOS::HiviewDFX::FpBacktrace> g_fpBacktraceOwner = nullptr;
std::atomic<OHOS::HiviewDFX::FpBacktrace*> g_fpBacktrace{nullptr};
std::atomic<StitchXsansAsyncFn> g_stitchXsansAsyncFn{nullptr};
std::atomic<DfxXsanRegisterUnwinderFn> g_xsanRegisterUnwinder{nullptr};
std::atomic<SanitizerDwarfUnwindFn> g_sanitizerDwarfUnwind{nullptr};
}

void LoadSanitizerDwarfUnwind()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        auto fn = reinterpret_cast<SanitizerDwarfUnwindFn>(
            dlsym(RTLD_DEFAULT, "SanitizerDwarfUnwind"));
        if (fn == nullptr) {
            XCOLLIE_LOGE("Failed to dlsym SanitizerDwarfUnwind.");
            return;
        }
        g_sanitizerDwarfUnwind.store(fn, std::memory_order_release);
    });
}

void LoadStitchFunc()
{
    void* asyncStackLibHandle = dlopen(ASYNC_STACK_LIB, RTLD_LAZY);
    if (asyncStackLibHandle == nullptr) {
        XCOLLIE_LOGE("Failed to dlopen %{public}s.", ASYNC_STACK_LIB);
        return;
    }
    auto stitchXsansAsync = reinterpret_cast<StitchXsansAsyncFn>(
        dlsym(asyncStackLibHandle, "StitchXsansAsyncStacks"));
    if (stitchXsansAsync == nullptr) {
        XCOLLIE_LOGE("Failed to dlsym stitch functions.");
        dlclose(asyncStackLibHandle);
        return;
    }
    g_stitchXsansAsyncFn.store(stitchXsansAsync, std::memory_order_release);
}

size_t InitFpBacktraceAndStitch()
{
    static std::once_flag onceFlag;
    static size_t initResult = 0;
    std::call_once(onceFlag, [] {
        g_fpBacktraceOwner = std::unique_ptr<OHOS::HiviewDFX::FpBacktrace>(
            OHOS::HiviewDFX::FpBacktrace::CreateInstance());
        if (g_fpBacktraceOwner != nullptr) {
            LoadStitchFunc();
            g_fpBacktrace.store(g_fpBacktraceOwner.get(), std::memory_order_release);
            initResult = 1;
        }
    });
    return initResult;
}

size_t GetGwpAsanFpFrames(void* frames, size_t capacity, size_t numFrames, size_t currentFrameAddr)
{
    if (frames == nullptr || capacity == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FpBacktrace* fpBacktrace = g_fpBacktrace.load(std::memory_order_acquire);
    if (fpBacktrace == nullptr) {
        return 0;
    }
    uint32_t requestCount = (capacity > ASYNC_LINK_LAYER + ASYNC_MAX_CONTENT) ?
        static_cast<uint32_t>(capacity - ASYNC_LINK_LAYER - ASYNC_MAX_CONTENT) : 0;
    if (requestCount > GWP_ASAN_MAX_DEPTH) {
        requestCount = GWP_ASAN_MAX_DEPTH;
    }
    void* pcArray[GWP_ASAN_MAX_DEPTH] = {nullptr};
    void* frameAddr = (currentFrameAddr == 0) ? __builtin_frame_address(0) :
        reinterpret_cast<void*>(currentFrameAddr);
    uint32_t syncCount = fpBacktrace->BacktraceFromFp(frameAddr, pcArray, requestCount, true);
    size_t* gwpFrames = static_cast<size_t*>(frames);
    for (size_t i = 0; i < syncCount && numFrames < capacity; ++i) {
        gwpFrames[numFrames] = reinterpret_cast<size_t>(pcArray[i]);
        ++numFrames;
    }
    return numFrames;
}

size_t GwpAsanUnwindImpl(size_t* frame_buf, size_t num_frames, size_t max_record_stack,
    size_t current_frame_addr, size_t stack_start, size_t stack_end)
{
    if (frame_buf == nullptr || max_record_stack <= 0) {
        return 0;
    }
    if (stack_end <= stack_start || stack_end - stack_start < sizeof(void*)) {
        return num_frames;
    }
    if (g_fpBacktrace.load(std::memory_order_acquire) == nullptr) {
        return 0;
    }
    size_t numFrames = GetGwpAsanFpFrames(frame_buf, max_record_stack, num_frames, current_frame_addr);
    void** frames = reinterpret_cast<void**>(frame_buf);
    StitchXsansAsyncFn stitchXsansAsync = g_stitchXsansAsyncFn.load(std::memory_order_acquire);
    if (stitchXsansAsync != nullptr) {
        return stitchXsansAsync(frames, max_record_stack, numFrames);
    }
    return numFrames;
}

void InitGwpAsanUnwinderIfNeed(const std::string& bundleName)
{
    if (bundleName.empty()) {
        return;
    }
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        XCOLLIE_LOGD("Start init GwpAsan unwind callbacks.");
        using RegisterUnwinderFn = int (*)(GwpAsanUnwindCallbacks*);
        auto registerUnwinder = reinterpret_cast<RegisterUnwinderFn>(
            dlsym(RTLD_DEFAULT, "libc_gwp_asan_set_unwind_callbacks"));
        if (registerUnwinder == nullptr) {
            XCOLLIE_LOGE("Failed to dlsym register_unwind.");
            return;
        }
        GwpAsanUnwindCallbacks callbacks = {};
        callbacks.structSize = (uint32_t)sizeof(callbacks);
        callbacks.abiVersion = GWP_ASAN_UNWIND_ABI_VERSION;
        callbacks.unwind = GwpAsanUnwindImpl;
        int regRc = registerUnwinder(&callbacks);
        if (regRc != SANITIZER_UNWIND_REGISTER_OK) {
            // No log after registration: hilog here would re-enter our unwind
            // callback via __sanitizer::TryCustomFpUnwind and can recurse.
            XCOLLIE_LOGE("Failed to register GwpAsan unwind callbacks, ret: %{public}d", regRc);
            return;
        }
        // Deliberately no "Finish" log: same reason as above.
    });
}

bool FpXsanFuncPtr(void* fp, void** buffer, int maxDepth, int* realDepth)
{
    if (buffer == nullptr || maxDepth <= 0 || realDepth == nullptr) {
        return false;
    }
    OHOS::HiviewDFX::FpBacktrace* fpBacktrace = g_fpBacktrace.load(std::memory_order_acquire);
    if (fpBacktrace == nullptr) {
        *realDepth = 0;
        return false;
    }
    size_t capacity = static_cast<size_t>(maxDepth);
    void* frameAddr = (fp == nullptr) ? __builtin_frame_address(0) : fp;
    uint32_t syncCount = fpBacktrace->BacktraceFromFp(
        frameAddr, buffer, static_cast<uint32_t>(maxDepth), true);
    size_t numFrames = syncCount;
    StitchXsansAsyncFn stitchXsansAsync = g_stitchXsansAsyncFn.load(std::memory_order_acquire);
    if (stitchXsansAsync != nullptr) {
        numFrames = stitchXsansAsync(buffer, capacity, numFrames);
    }
    *realDepth = static_cast<int>(numFrames);
    return numFrames > 0;
}

bool DwarfXsanFuncPtr(void** buffer, int maxDepth, int* realDepth)
{
    if (buffer == nullptr || maxDepth <= 0 || realDepth == nullptr) {
        return false;
    }
    if (g_fpBacktrace.load(std::memory_order_acquire) == nullptr) {
        *realDepth = 0;
        return false;
    }
    size_t capacity = static_cast<size_t>(maxDepth);
    size_t maxSyncDepth = capacity;
    size_t numFrames = 0;
    LoadSanitizerDwarfUnwind();
    SanitizerDwarfUnwindFn dwarfUnwindFn = g_sanitizerDwarfUnwind.load(std::memory_order_acquire);
    size_t syncCount = 0;
    if (dwarfUnwindFn != nullptr && dwarfUnwindFn(buffer, maxSyncDepth, &syncCount)) {
        if (syncCount > capacity) {
            syncCount = capacity;
        }
        StitchXsansAsyncFn stitchXsansAsync = g_stitchXsansAsyncFn.load(std::memory_order_acquire);
        if (stitchXsansAsync != nullptr) {
            numFrames = stitchXsansAsync(buffer, capacity, syncCount);
        } else {
            numFrames = syncCount;
        }
    }
    *realDepth = static_cast<int>(numFrames);
    return numFrames > 0;
}

// Reserved for the sanitizer runtime only: registerUnwinder is expected to be
// the sanitizer's own registration entry. It is used as a fallback only when
// __sanitizer_set_unwind_callbacks cannot be resolved, so callers outside the
// sanitizer runtime must not pass arbitrary function pointers here.
extern "C" int RegisterXsansUnwindCallbacks(DfxXsanRegisterUnwinderFn registerUnwinder)
{
    if (registerUnwinder != nullptr) {
        g_xsanRegisterUnwinder.store(registerUnwinder, std::memory_order_release);
    }
    auto xsanRegisterUnwinder = reinterpret_cast<DfxXsanRegisterUnwinderFn>(
        dlsym(RTLD_DEFAULT, "__sanitizer_set_unwind_callbacks"));
    if (xsanRegisterUnwinder == nullptr) {
        xsanRegisterUnwinder = g_xsanRegisterUnwinder.load(std::memory_order_acquire);
        if (xsanRegisterUnwinder == nullptr) {
            // Only report when there is really no way to register; the saved
            // fallback (if any) is a legitimate path and must not look like an error.
            XCOLLIE_LOGE("dlsym __sanitizer_set_unwind_callbacks failed, XSAN not enabled.");
            return SANITIZER_UNWIND_REGISTER_NO_CALLBACK;
        }
    }
    OHOS::HiviewDFX::SanitizerUnwindCallbacks callbacks = {};
    callbacks.structSize = (uint32_t)sizeof(callbacks);
    callbacks.abiVersion = GWP_ASAN_UNWIND_ABI_VERSION;
    callbacks.fp_unwind = FpXsanFuncPtr;
    callbacks.dwarf_unwind = DwarfXsanFuncPtr;
    int ret = xsanRegisterUnwinder(&callbacks);
    if (ret != OHOS::HiviewDFX::SANITIZER_UNWIND_REGISTER_OK) {
        // Registration failed, so no callback was installed and it is safe to log.
        // Do not log on success: hilog would re-enter our unwind callback via
        // __sanitizer::TryCustomFpUnwind and can recurse.
        XCOLLIE_LOGE("Failed to register XSAN unwind callbacks, ret: %{public}d", ret);
    }
    return ret;
}

void InitXsansUnwindIfNeed(const std::string& bundleName)
{
    if (bundleName.empty()) {
        return;
    }
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        // Internal trigger: no external register function, reuse the one saved
        // from the external caller (may be nullptr, then only dlsym is used).
        RegisterXsansUnwindCallbacks(nullptr);
    });
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
