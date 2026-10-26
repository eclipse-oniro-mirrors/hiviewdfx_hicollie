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


#include "process_kill_reason.h"
#include <limits>
#include <string>

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {

class ProcessKillReasonTest : public testing::Test {
public:
    ProcessKillReasonTest()
    {}
    ~ProcessKillReasonTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void ProcessKillReasonTest::SetUpTestCase(void)
{
}

void ProcessKillReasonTest::TearDownTestCase(void)
{
}

void ProcessKillReasonTest::SetUp(void)
{
}

void ProcessKillReasonTest::TearDown(void)
{
}

/**
 * @tc.name: GetKillReason test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(ProcessKillReasonTest, ProcessKillReasonTest_GetKillReason_001, TestSize.Level1)
{
    int killId = ProcessKillReason::REASON_KILL_APPLICATION_BY_BUNDLE_NAME;
    std::string ret = ProcessKillReason::GetKillReason(killId);
    EXPECT_EQ(ret, "KillApplicationByBundleName");
    ret = ProcessKillReason::GetAppExitReason(killId);
    EXPECT_EQ(ret, "KillApplication");
}

/**
 * @tc.name: GetKillReason test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(ProcessKillReasonTest, ProcessKillReasonTest_GetKillReason_002, TestSize.Level1)
{
    int killId = -1;
    std::string ret = ProcessKillReason::GetKillReason(killId);
    EXPECT_EQ(ret, "InvalidKillId");
    ret = ProcessKillReason::GetAppExitReason(killId);
    EXPECT_EQ(ret, "InvalidKillId");
}

/**
 * @tc.name: GetKillReason test
 * @tc.desc: verify the id -> reason index covers the boundaries of every id range
 * @tc.type: FUNC
 */
HWTEST_F(ProcessKillReasonTest, ProcessKillReasonTest_GetKillReason_003, TestSize.Level1)
{
    struct Boundary {
        int32_t id;
        const char* faultReason;
    };
    const Boundary valid[] = {
        {1, "KillApplicationByBundleName"},
        {9, "KillProcess"},
        {1000, "User Request"},
        {1009, "signal exit"},
        {2000, "THREAD_BLOCK_6S"},
        {2024, "RS_TRANSACTION_DATA_OVERLIMIT"},
        {3000, "ContinuouslyWakeupAbnormal"},
        {3054, "InactiveProcKill"},
        {4000, "OOM_KILLER"},
        {4011, "VolumeUmount"},
    };
    for (const auto& item : valid) {
        EXPECT_EQ(ProcessKillReason::GetKillReason(item.id), item.faultReason);
    }
    const int32_t invalid[] = {0, 10, 999, 1010, 1999, 2025, 2999, 3055, 3999, 4012, 5000, 6000,
                               std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min()};
    for (int32_t id : invalid) {
        EXPECT_EQ(ProcessKillReason::GetKillReason(id), "InvalidKillId");
        EXPECT_EQ(ProcessKillReason::GetAppExitReason(id), "InvalidKillId");
    }
}

/**
 * @tc.name: GetKillReason test
 * @tc.desc: sweep the whole id space, both APIs must agree on validity
 * @tc.type: FUNC
 */
HWTEST_F(ProcessKillReasonTest, ProcessKillReasonTest_GetKillReason_004, TestSize.Level1)
{
    for (int32_t id = -1024; id <= 8192; ++id) {
        bool faultValid = ProcessKillReason::GetKillReason(id) != "InvalidKillId";
        bool appValid = ProcessKillReason::GetAppExitReason(id) != "InvalidKillId";
        EXPECT_EQ(faultValid, appValid) << "id=" << id;
    }
}
} // namespace HiviewDFX
} // namespace OHOS
