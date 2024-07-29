/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "hicollie.h"

#include <unistd.h>
#include <string>
#include "watchdog.h"
#include "report_data.h"
#include "xcollie_utils.h"
#include "iservice_registry.h"
#include "iremote_object.h"

DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.AppMgr");

#ifdef SUPPORT_ASAN
constexpr uint32_t CHECK_INTERVAL_TIME = 45000;
#else
constexpr uint32_t CHECK_INTERVAL_TIME = 3000;
#endif
constexpr uint32_t INI_TIMER_FIRST_SECOND = 10000;
constexpr uint32_t NOTIFY_APP_FAULT = 38;
constexpr uint32_t APP_MGR_SERVICE_ID = 501;
constexpr int64_t MIN_APP_UID = 20000;

bool IsAppMainThread()
{
    static int pid = -1;
    static int uid = -1;

    if (pid == -1) {
        pid = getpid();
    }
    if (uid == -1) {
        uid = getuid();
    }
    if (pid == gettid() && uid >= MIN_APP_UID) {
        return true;
    }
    return false;
}

HiCollie_ErrorCode OH_HiCollie_Init_StuckDetection(OH_HiCollie_Task task)
{
    if (IsAppMainThread()) {
        return HICOLLIE_WRONG_THREAD_CONTEXT;
    }
    if (!task) {
        OHOS::HiviewDFX::Watchdog::GetInstance().RemovePeriodicalTask("BussinessWatchdog");
    } else {
        OHOS::HiviewDFX::Watchdog::GetInstance().RunPeriodicalTask("BussinessWatchdog", task,
            CHECK_INTERVAL_TIME, INI_TIMER_FIRST_SECOND);
        OHOS::HiviewDFX::Watchdog::GetInstance().RemovePeriodicalTask("AppkitWatchdog");
    }
    return HICOLLIE_SUCCESS;
}

HiCollie_ErrorCode OH_HiCollie_Init_JankDetection(OH_HiCollie_BeginFunc* beginFunc,
    OH_HiCollie_EndFunc* endFunc, HiCollie_DetectionParam param)
{
    if (IsAppMainThread()) {
        return HICOLLIE_WRONG_THREAD_CONTEXT;
    }
    if ((!beginFunc && endFunc) || (beginFunc && !endFunc)) {
        return HICOLLIE_INVALID_ARGUMENT;
    }
    OHOS::HiviewDFX::Watchdog::GetInstance().InitMainLooperWatcher(beginFunc,
        endFunc);
    return HICOLLIE_SUCCESS;
}

int32_t NotifyAppFault(const OHOS::HiviewDFX::ReportData &reportData)
{
    XCOLLIE_LOGD("called.");
    auto systemAbilityMgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        XCOLLIE_LOGE("ReportData failed to get system ability manager.");
        return -1;
    }
    auto appMgrService = systemAbilityMgr->GetSystemAbility(APP_MGR_SERVICE_ID);
    if (appMgrService == nullptr) {
        XCOLLIE_LOGE("ReportData failed to find APP_MGR_SERVICE_ID.");
        return -1;
    }
    OHOS::MessageParcel data;
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        XCOLLIE_LOGE("ReportData failed to WriteInterfaceToken.");
        return -1;
    }
    if (!data.WriteParcelable(&reportData)) {
        XCOLLIE_LOGE("ReportData write reportData failed.");
        return -1;
    }
    auto ret = appMgrService->SendRequest(NOTIFY_APP_FAULT, data, reply, option);
    if (ret != OHOS::NO_ERROR) {
        XCOLLIE_LOGE("ReportData Send request failed with error code: %{public}d", ret);
        return -1;
    }
    return reply.ReadInt32();
}

int Report(bool* isSixSecond)
{
    OHOS::HiviewDFX::ReportData reportData;
    reportData.errorObject.message = "Bussiness thread is not response!";
    reportData.faultType = OHOS::HiviewDFX::FaultDataType::APP_FREEZE;

    if (*isSixSecond) {
        reportData.errorObject.name = "BUSSINESS_THREAD_BLOCK_6S";
        reportData.forceExit = true;
        *isSixSecond = false;
    } else {
        reportData.errorObject.name = "BUSSINESS_THREAD_BLOCK_3S";
        reportData.forceExit = false;
        *isSixSecond = true;
    }
    reportData.timeoutMarkers = "";
    reportData.errorObject.stack = "";
    reportData.notifyApp = false;
    reportData.waitSaveState = false;
    auto result = NotifyAppFault(reportData);
    XCOLLIE_LOGI("OH_HiCollie_Report result: %{public}d", result);
    return result;
}

HiCollie_ErrorCode OH_HiCollie_Report(bool* isSixSecond)
{
    if (IsAppMainThread()) {
        return HICOLLIE_WRONG_THREAD_CONTEXT;
    }
    if (isSixSecond == nullptr) {
        return HICOLLIE_INVALID_ARGUMENT;
    }
    if (Report(isSixSecond) != 0) {
        return HICOLLIE_REMOTE_FAILED;
    }
    return HICOLLIE_SUCCESS;
}