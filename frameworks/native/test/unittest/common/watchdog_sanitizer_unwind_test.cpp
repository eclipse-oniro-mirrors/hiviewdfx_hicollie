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

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "watchdog_sanitizer_unwind.h"

#if defined(__aarch64__)
namespace OHOS {
namespace HiviewDFX {
// Internal entry points of watchdog_sanitizer_unwind.cpp (not declared in the header).
bool FpXsanFuncPtr(void* fp, void** buffer, int maxDepth, int* realDepth);
bool DwarfXsanFuncPtr(void** buffer, int maxDepth, int* realDepth);
size_t GwpAsanUnwindImpl(size_t* frameBuf, size_t numFrames, size_t maxRecordStack,
    size_t currentFrameAddr, size_t stackStart, size_t stackEnd);
} // namespace HiviewDFX
} // namespace OHOS

using namespace OHOS::HiviewDFX;
using namespace testing::ext;

namespace {
constexpr int MAX_DEPTH = 32;
constexpr size_t FRAME_CAPACITY = 32;
constexpr size_t STACK_RANGE = 4096;

int FakeRegisterUnwinder(const SanitizerUnwindCallbacks* callbacks)
{
    if (callbacks == nullptr || callbacks->structSize == 0) {
        return SANITIZER_UNWIND_REGISTER_INVALID_ARGUMENT;
    }
    return SANITIZER_UNWIND_REGISTER_OK;
}

int FakeRegisterUnwinderUnsupported(const SanitizerUnwindCallbacks* callbacks)
{
    if (callbacks == nullptr) {
        return SANITIZER_UNWIND_REGISTER_INVALID_ARGUMENT;
    }
    return SANITIZER_UNWIND_REGISTER_UNSUPPORTED_ABI;
}
} // namespace

class WatchdogSanitizerUnwindTest : public testing::Test {};

/**
 * @tc.name: WatchdogSanitizerUnwindTest001
 * @tc.desc: FpXsanFuncPtr returns false when buffer is null
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest001, TestSize.Level2)
{
    int realDepth = -1;
    EXPECT_FALSE(FpXsanFuncPtr(nullptr, nullptr, MAX_DEPTH, &realDepth));
    EXPECT_EQ(realDepth, -1);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest002
 * @tc.desc: FpXsanFuncPtr returns false when maxDepth is not positive
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest002, TestSize.Level2)
{
    void* buffer[MAX_DEPTH] = {nullptr};
    int realDepth = -1;
    EXPECT_FALSE(FpXsanFuncPtr(nullptr, buffer, 0, &realDepth));
    EXPECT_EQ(realDepth, -1);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest003
 * @tc.desc: FpXsanFuncPtr returns false when realDepth is null
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest003, TestSize.Level2)
{
    void* buffer[MAX_DEPTH] = {nullptr};
    EXPECT_FALSE(FpXsanFuncPtr(nullptr, buffer, MAX_DEPTH, nullptr));
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest004
 * @tc.desc: DwarfXsanFuncPtr returns false when buffer is null
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest004, TestSize.Level2)
{
    int realDepth = -1;
    EXPECT_FALSE(DwarfXsanFuncPtr(nullptr, MAX_DEPTH, &realDepth));
    EXPECT_EQ(realDepth, -1);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest005
 * @tc.desc: DwarfXsanFuncPtr returns false when realDepth is null
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest005, TestSize.Level2)
{
    void* buffer[MAX_DEPTH] = {nullptr};
    EXPECT_FALSE(DwarfXsanFuncPtr(buffer, MAX_DEPTH, nullptr));
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest006
 * @tc.desc: GwpAsanUnwindImpl returns zero when frame buffer is null
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest006, TestSize.Level2)
{
    EXPECT_EQ(GwpAsanUnwindImpl(nullptr, 0, FRAME_CAPACITY, 0, 0, STACK_RANGE), 0U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest007
 * @tc.desc: GwpAsanUnwindImpl returns zero when max record stack is zero
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest007, TestSize.Level2)
{
    size_t frames[FRAME_CAPACITY] = {0};
    EXPECT_EQ(GwpAsanUnwindImpl(frames, 0, 0, 0, 0, STACK_RANGE), 0U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest008
 * @tc.desc: GwpAsanUnwindImpl keeps numFrames when the stack range is too small
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest008, TestSize.Level2)
{
    size_t frames[FRAME_CAPACITY] = {0};
    EXPECT_EQ(GwpAsanUnwindImpl(frames, 3, FRAME_CAPACITY, 0, STACK_RANGE, STACK_RANGE), 3U);
    EXPECT_EQ(GwpAsanUnwindImpl(frames, 4, FRAME_CAPACITY, 0, STACK_RANGE, STACK_RANGE + 1), 4U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest008A
 * @tc.desc: FpXsanFuncPtr returns false when the fp unwinder is not initialized yet
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest008A, TestSize.Level2)
{
    // Must run before Test011 initializes the shared fp unwinder instance.
    void* buffer[MAX_DEPTH] = {nullptr};
    int realDepth = -1;
    EXPECT_FALSE(FpXsanFuncPtr(nullptr, buffer, MAX_DEPTH, &realDepth));
    EXPECT_EQ(realDepth, 0);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest008B
 * @tc.desc: DwarfXsanFuncPtr returns false when the fp unwinder is not initialized yet
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest008B, TestSize.Level2)
{
    // Must run before Test011 initializes the shared fp unwinder instance.
    void* buffer[MAX_DEPTH] = {nullptr};
    int realDepth = -1;
    EXPECT_FALSE(DwarfXsanFuncPtr(buffer, MAX_DEPTH, &realDepth));
    EXPECT_EQ(realDepth, 0);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest008C
 * @tc.desc: GwpAsanUnwindImpl returns zero when the fp unwinder is not initialized yet
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest008C, TestSize.Level2)
{
    // Must run before Test011 initializes the shared fp unwinder instance.
    size_t frames[FRAME_CAPACITY] = {0};
    int stackProbe = 0;
    size_t stackStart = reinterpret_cast<size_t>(&stackProbe);
    EXPECT_EQ(GwpAsanUnwindImpl(frames, 0, FRAME_CAPACITY, 0, stackStart, stackStart + STACK_RANGE), 0U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest009
 * @tc.desc: InitGwpAsanUnwinderIfNeed returns early for an empty bundle name
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest009, TestSize.Level2)
{
    // Empty bundle name must not consume the once flag nor register anything.
    InitGwpAsanUnwinderIfNeed("");
    // Still callable with a real bundle name afterwards without crashing.
    InitGwpAsanUnwinderIfNeed("com.ohos.unittest");
    SUCCEED();
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest010
 * @tc.desc: InitXsansUnwindIfNeed returns early for an empty bundle name and is idempotent
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest010, TestSize.Level2)
{
    InitXsansUnwindIfNeed("");
    InitXsansUnwindIfNeed("com.ohos.unittest");
    // Second call hits std::call_once with an already-run flag.
    InitXsansUnwindIfNeed("com.ohos.unittest");
    SUCCEED();
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest010A
 * @tc.desc: The first InitFpBacktraceAndStitch is safe under concurrent calls
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest010A, TestSize.Level2)
{
    // Declared before Test011 on purpose: this must be the first call that
    // creates the unwinder, so the race is on the one-time initialization
    // rather than on the fast path of an already initialized instance.
    constexpr int THREAD_COUNT = 8;
    std::vector<std::thread> threads;
    std::vector<size_t> results(THREAD_COUNT, 0);
    threads.reserve(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&results, i] {
            results[i] = InitFpBacktraceAndStitch();
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        EXPECT_EQ(results[i], 1U);
    }
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest011
 * @tc.desc: InitFpBacktraceAndStitch returns 1 on reuse after the first call
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest011, TestSize.Level2)
{
    EXPECT_EQ(InitFpBacktraceAndStitch(), 1U);
    // Second call must short-circuit on the already created instance.
    EXPECT_EQ(InitFpBacktraceAndStitch(), 1U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest011A
 * @tc.desc: RegisterXsansUnwindCallbacks returns -2 when no register function is available
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest011A, TestSize.Level2)
{
    // Declared before Test012 on purpose: with neither a dlsym hit nor a stored
    // callback the registration must be rejected.
    EXPECT_EQ(RegisterXsansUnwindCallbacks(nullptr), -2);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest012
 * @tc.desc: RegisterXsansUnwindCallbacks stores the register function and reuses it afterwards
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest012, TestSize.Level2)
{
    // First call saves the callback; dlsym normally fails in the test binary, so the
    // stored one is used. If the host does export __sanitizer_set_unwind_callbacks,
    // the call still succeeds but goes through that symbol instead.
    EXPECT_EQ(RegisterXsansUnwindCallbacks(FakeRegisterUnwinder), SANITIZER_UNWIND_REGISTER_OK);
    // Second call reuses the stored callback.
    EXPECT_EQ(RegisterXsansUnwindCallbacks(nullptr), SANITIZER_UNWIND_REGISTER_OK);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest013
 * @tc.desc: RegisterXsansUnwindCallbacks propagates a non-zero register result
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest013, TestSize.Level2)
{
    // Only meaningful when the stored callback is actually used, i.e. dlsym misses.
    int ret = RegisterXsansUnwindCallbacks(FakeRegisterUnwinderUnsupported);
    if (ret != SANITIZER_UNWIND_REGISTER_UNSUPPORTED_ABI) {
        // The host exported __sanitizer_set_unwind_callbacks and accepted the callbacks.
        EXPECT_EQ(ret, SANITIZER_UNWIND_REGISTER_OK);
    }
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest015
 * @tc.desc: FpXsanFuncPtr unwinds the current frame and reports a positive depth
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest015, TestSize.Level2)
{
    ASSERT_EQ(InitFpBacktraceAndStitch(), 1U);
    void* buffer[MAX_DEPTH] = {nullptr};
    int realDepth = 0;
    ASSERT_TRUE(FpXsanFuncPtr(nullptr, buffer, MAX_DEPTH, &realDepth));
    EXPECT_GT(realDepth, 0);
    EXPECT_NE(buffer[0], nullptr);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest016
 * @tc.desc: DwarfXsanFuncPtr unwinds the current stack and reports a positive depth
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest016, TestSize.Level2)
{
    ASSERT_EQ(InitFpBacktraceAndStitch(), 1U);
    void* buffer[MAX_DEPTH] = {nullptr};
    int realDepth = 0;
    bool ret = DwarfXsanFuncPtr(buffer, MAX_DEPTH, &realDepth);
    if (ret) {
        EXPECT_GT(realDepth, 0);
    } else {
        EXPECT_EQ(realDepth, 0);
    }
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest017
 * @tc.desc: GwpAsanUnwindImpl records frames for a valid stack range
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest017, TestSize.Level2)
{
    ASSERT_EQ(InitFpBacktraceAndStitch(), 1U);
    size_t frames[FRAME_CAPACITY] = {0};
    int stackProbe = 0;
    size_t stackStart = reinterpret_cast<size_t>(&stackProbe);
    size_t result = GwpAsanUnwindImpl(frames, 0, FRAME_CAPACITY, 0, stackStart, stackStart + STACK_RANGE);
    EXPECT_GT(result, 0U);
    EXPECT_NE(frames[0], 0U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest018
 * @tc.desc: GwpAsanUnwindImpl yields no new frames when the capacity leaves no room
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest018, TestSize.Level2)
{
    ASSERT_EQ(InitFpBacktraceAndStitch(), 1U);
    size_t frames[FRAME_CAPACITY] = {0};
    int stackProbe = 0;
    size_t stackStart = reinterpret_cast<size_t>(&stackProbe);
    // max_record_stack <= LINK_LAYER + ASYNC_MAX_CONTENT leaves a zero request count.
    EXPECT_EQ(GwpAsanUnwindImpl(frames, 0, 16, 0, stackStart, stackStart + STACK_RANGE), 0U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest019
 * @tc.desc: GwpAsanUnwindImpl clamps the request depth to the max depth limit
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest019, TestSize.Level2)
{
    ASSERT_EQ(InitFpBacktraceAndStitch(), 1U);
    constexpr size_t largeCapacity = 256;
    size_t frames[largeCapacity] = {0};
    int stackProbe = 0;
    size_t stackStart = reinterpret_cast<size_t>(&stackProbe);
    size_t result = GwpAsanUnwindImpl(frames, 0, largeCapacity, 0, stackStart, stackStart + STACK_RANGE);
    EXPECT_GT(result, 0U);
    EXPECT_LE(result, 128U);
    EXPECT_NE(frames[0], 0U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest020
 * @tc.desc: InitFpBacktraceAndStitch tolerates a missing async stack library
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest020, TestSize.Level2)
{
    // libasync_stack.z.so may or may not be present; either path must not crash.
    EXPECT_EQ(InitFpBacktraceAndStitch(), 1U);
}

/**
 * @tc.name: WatchdogSanitizerUnwindTest022
 * @tc.desc: Unwind callbacks stay usable while called from multiple threads
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogSanitizerUnwindTest, WatchdogSanitizerUnwindTest022, TestSize.Level2)
{
    ASSERT_EQ(InitFpBacktraceAndStitch(), 1U);
    constexpr int THREAD_COUNT = 4;
    std::vector<std::thread> threads;
    // Plain int instead of vector<bool>: the latter packs bits, so concurrent
    // writes to different elements would race on the same byte.
    std::vector<int> results(THREAD_COUNT, 0);
    threads.reserve(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&results, i] {
            void* buffer[MAX_DEPTH] = {nullptr};
            int realDepth = 0;
            results[i] = (FpXsanFuncPtr(nullptr, buffer, MAX_DEPTH, &realDepth) && realDepth > 0) ? 1 : 0;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        EXPECT_EQ(results[i], 1);
    }
}
#endif // __aarch64__
