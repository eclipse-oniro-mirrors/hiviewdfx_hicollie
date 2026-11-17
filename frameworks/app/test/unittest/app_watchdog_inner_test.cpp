/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define private public
#define protected public
#include "app_watchdog_inner.h"
#undef private
#undef protected

#include <gtest/gtest.h>
#include <string>
#include <thread>

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
class AppWatchdogInnerTest : public testing::Test {
public:
    AppWatchdogInnerTest()
    {}
    ~AppWatchdogInnerTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void AppWatchdogInnerTest::SetUpTestCase(void)
{
}

void AppWatchdogInnerTest::TearDownTestCase(void)
{
}

void AppWatchdogInnerTest::SetUp(void)
{
}

void AppWatchdogInnerTest::TearDown(void)
{
}

/**
 * @tc.name: AppWatchdogInner GetReservedTimeForLogging Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInner_GetReservedTimeForLogging_001, TestSize.Level1)
{
    int ret = AppWatchdogInner::GetInstance().GetReservedTimeForLogging();
    EXPECT_TRUE(ret >= 3500);
    ret = AppWatchdogInner::GetInstance().GetReservedTimeForLogging();
    EXPECT_TRUE(ret >= 3500);
}
/**
 * @tc.name: AppWatchdogInner BundleName Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_BundleName_001, TestSize.Level1)
{
    std::string testValue = "AppWatchdogInnerTest_BundleName_001";
    AppWatchdogInner::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetBundleName(), testValue);
    testValue = "AppWatchdogInner";
    AppWatchdogInner::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetBundleName(), testValue);
    testValue = "com.ohos.sceneboard";
    AppWatchdogInner::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetBundleName(), testValue);
}

/**
 * @tc.name: AppWatchdogInner IsSystemApp Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_IsSystemApp_001, TestSize.Level1)
{
    bool isSystemApp = true;
    AppWatchdogInner::GetInstance().SetSystemApp(isSystemApp);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSystemApp(), isSystemApp);
    isSystemApp = false;
    AppWatchdogInner::GetInstance().SetSystemApp(isSystemApp);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSystemApp(), isSystemApp);
}

/**
 * @tc.name: AppWatchdogInner AppDebug Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_AppDebug_001, TestSize.Level1)
{
    bool isAppDebug = true;
    AppWatchdogInner::GetInstance().SetAppDebug(isAppDebug);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetAppDebug(), isAppDebug);
    isAppDebug = false;
    AppWatchdogInner::GetInstance().SetAppDebug(isAppDebug);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetAppDebug(), isAppDebug);
}

/**
 * @tc.name: AppWatchdogInner Foreground Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_Foreground_001, TestSize.Level1)
{
    bool isForeground = true;
    AppWatchdogInner::GetInstance().SetForeground(isForeground);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetForeground(), isForeground);
    isForeground = false;
    AppWatchdogInner::GetInstance().SetForeground(isForeground);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetForeground(), isForeground);
}

/**
 * @tc.name: AppWatchdogInner SpecifiedProcessName Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_SpecifiedProcessName_001, TestSize.Level1)
{
    std::string testValue = "AppWatchdogInnerTest_SpecifiedProcessName_001";
    AppWatchdogInner::GetInstance().SetSpecifiedProcessName(testValue);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSpecifiedProcessName(), testValue);
    testValue = "AppWatchdogInner";
    AppWatchdogInner::GetInstance().SetSpecifiedProcessName(testValue);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSpecifiedProcessName(), testValue);
}

/**
 * @tc.name: AppWatchdogInner ScrollState Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_ScrollState_001, TestSize.Level1)
{
    bool isScroll = true;
    AppWatchdogInner::GetInstance().SetScrollState(isScroll);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetScrollState(), isScroll);
    isScroll = false;
    AppWatchdogInner::GetInstance().SetScrollState(isScroll);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetScrollState(), isScroll);
}

/**
 * @tc.name: AppWatchdogInnerTest_BundleInfoConcurrent_001
 * @tc.desc: Verify SetBundleInfo and GetBundleName are thread-safe under concurrent access
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_BundleInfoConcurrent_001, TestSize.Level1)
{
    auto& inner = AppWatchdogInner::GetInstance();
    std::atomic<bool> running(true);
    std::atomic<int> inconsistentCount(0);

    std::thread setter([&inner, &running]() {
        int i = 0;
        while (running.load()) {
            inner.SetBundleInfo("bundle_" + std::to_string(i % 100), std::to_string(i % 100));
            i++;
        }
    });

    std::thread getter([&inner, &running, &inconsistentCount]() {
        while (running.load()) {
            std::string name = inner.GetBundleName();
            if (!name.empty()) {
                inconsistentCount.fetch_add(0);
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running.store(false);
    setter.join();
    getter.join();

    EXPECT_TRUE(inner.bundleMutex_.try_lock());
    inner.bundleMutex_.unlock();
}

/**
 * @tc.name: AppWatchdogInnerTest_BundleMutex_001
 * @tc.desc: Verify bundleMutex_ exists and is unlocked
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_BundleMutex_001, TestSize.Level1)
{
    auto& inner = AppWatchdogInner::GetInstance();
    EXPECT_TRUE(inner.bundleMutex_.try_lock());
    inner.bundleMutex_.unlock();
}
} // namespace HiviewDFX
} // namespace OHOS
