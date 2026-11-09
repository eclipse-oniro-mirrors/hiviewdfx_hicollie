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

#ifndef RELIABILITY_WATCHDOG_UNWIND_INIT_H
#define RELIABILITY_WATCHDOG_UNWIND_INIT_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace OHOS {
namespace HiviewDFX {
using GwpAsanUnwindCallback = size_t (*)(size_t*, size_t, size_t, size_t, size_t, size_t);
struct GwpAsanUnwindCallbacks {
    uint32_t structSize;
    uint32_t abiVersion;
    GwpAsanUnwindCallback unwind;
};
using SanitizerFpUnwindCallback = bool (*)(void*, void**, int, int*);
using SanitizerDwarfUnwindCallback = bool (*)(void**, int, int*);
struct SanitizerUnwindCallbacks {
    uint32_t structSize;
    uint32_t abiVersion;
    SanitizerFpUnwindCallback fp_unwind;
    SanitizerDwarfUnwindCallback dwarf_unwind;
};
enum SanitizerUnwindRegisterResult {
    SANITIZER_UNWIND_REGISTER_OK = 0,
    SANITIZER_UNWIND_REGISTER_INVALID_ARGUMENT = 1,
    SANITIZER_UNWIND_REGISTER_UNSUPPORTED_ABI = 2
};
using GetSubmitterStackIdFn = uint64_t (*)(void);
using GetPcsByStackIdFn = bool (*)(uint64_t, void**, size_t);
using StitchSyncAndAsyncFn = size_t (*)(size_t*, size_t, size_t);
using StitchXsansAsyncFn = size_t (*)(void**, size_t, size_t);
using DfxXsanRegisterUnwinderFn = int (*)(const SanitizerUnwindCallbacks*);

#if defined(__aarch64__)
/**
 * @brief Create the frame pointer unwinder and load the async-stack stitch helper.
 *
 * Must be called before the unwind callbacks are registered, so that the
 * callback can append the async stack at unwind time.
 *
 * @return 1 on success, 0 on failure.
 */
size_t InitFpBacktraceAndStitch();

/**
 * @brief Register the GWP-ASan unwind callback.
 */
void InitGwpAsanUnwinderIfNeed(const std::string& bundleName);

/**
 * @brief Register the XSAN unwind callbacks.
 */
void InitXsansUnwindIfNeed(const std::string& bundleName);

/**
 * @brief Register the XSAN unwind callbacks, saving the caller supplied
 *        register function for later internal use.
 *
 * @note This interface is reserved for the sanitizer runtime only and is not
 *       intended for third-party callers. When the process provides
 *       __sanitizer_set_unwind_callbacks, that symbol is preferred and the
 *       saved function is used only as a fallback. Callers outside the
 *       sanitizer runtime must not pass arbitrary function pointers here.
 */
extern "C" int RegisterXsansUnwindCallbacks(DfxXsanRegisterUnwinderFn registerUnwinder);
#endif
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif // RELIABILITY_WATCHDOG_UNWIND_INIT_H
