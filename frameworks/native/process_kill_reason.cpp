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

namespace OHOS {
namespace HiviewDFX {
namespace ProcessKillReason {
namespace {
constexpr const char* INVALID_KILL_ID = "InvalidKillId";

struct KillReasonEntry {
    int32_t id;
    const char* faultReason;
    const char* appReason;
};

constexpr KillReasonEntry KILL_REASON_CONFIG[] = {
    {REASON_KILL_APPLICATION_BY_BUNDLE_NAME, "KillApplicationByBundleName", "KillApplication"},
    {REASON_ON_REMOTE_DIED, "OnRemoteDied", "OnRemoteDied"},
    {REASON_RESTART, "RestartApp", "Restart"},
    {REASON_APP_EXIT, "app exit", "KillApplication"},
    {REASON_KILL_APPLICATION_SELF, "KillApplicationSelf", "KillApplication"},
    {REASON_KILL_PROCESS_WITH_ACCOUNT, "KillProcessWithAccount", "KillApplication"},
    {REASON_KILL_APPLICATION, "KillApplication", "KillApplication"},
    {REASON_KILL_CGROUP, "KillCgroup", "KillApplication"},
    {REASON_KILL_PROCESS, "KillProcess", "KillProcess"},

    {REASON_USER_REQUEST, "User Request", "UserRequest"},
    {REASON_CLEAR_SESSION, "ClearSession", "UserRequest"},
    {REASON_UNINSTALL_APP, "UninstallApp", "Uninstall"},
    {REASON_UPGRADE_APP, "UpgradeApp", "Upgrade"},
    {REASON_USER_LOGOUT, "UserLogout", "Logout"},
    {REASON_USER_STOP, "UserStop", "Logout"},
    {REASON_PERMISSION_UPDATE, "PermissionUpdate", "PermissionUpdate"},
    {REASON_AA_FORCE_STOP, "aa force-stop", "aaForceStop"},
    {REASON_SIGNAL_KILL, "signal kill", "signalKill"},
    {REASON_SIGNAL_EXIT, "signal exit", "signalExit"},

    {REASON_THREAD_BLOCK_6S, "THREAD_BLOCK_6S", "ThreadBlock6S"},
    {REASON_APP_INPUT_BLOCK, "APP_INPUT_BLOCK", "AppInputBlock"},
    {REASON_LIFECYCLE_TIMEOUT, "LIFECYCLE_TIMEOUT", "LifecycleTimeout"},
    {REASON_JS_ERROR, "Js Error", "JsError"},
    {REASON_CPP_CRASH, "Cpp Crash", "CppCrash"},
    {REASON_CES_REGISTER_EXCEED_LIMIT, "CES Register exceed limit", "ResourceLeak(CES)"},
    {REASON_RESOURCE_LEAK_PSS_SOFT_KILL, "ResourceLeak:Pss Soft Kill", "ResourceLeak(PSSSoftLeak)"},
    {REASON_RESOURCE_LEAK_PSS_KILL, "ResourceLeak:Pss Kill", "ResourceLeak(PSSLeak)"},
    {REASON_RESOURCE_LEAK_ION_LEAK, "ResourceLeak:Ion Leak", "ResourceLeak(IonLeak)"},
    {REASON_RESOURCE_LEAK_ASHMEM_LEAK, "ResourceLeak:Ashmem Leak", "ResourceLeak(AshmemLeak)"},
    {REASON_RESOURCE_LEAK_GPU_RS_LEAK, "ResourceLeak:Gpu_rs Leak", "ResourceLeak(GpuRsLeak)"},
    {REASON_RESOURCE_LEAK_GPU_LEAK, "ResourceLeak:Gpu Leak", "ResourceLeak(GpuLeak)"},
    {REASON_RESOURCE_LEAK_VMA_LEAK, "ResourceLeak:Vma Leak", "ResourceLeak(VMALeak)"},
    {REASON_RESOURCE_LEAK_FD_LEAK, "ResourceLeak:Fd Leak", "ResourceLeak(FDLeak)"},
    {REASON_RESOURCE_LEAK_THREAD_LEAK, "ResourceLeak:Thread Leak", "ResourceLeak(ThreadLeak)"},
    {REASON_FD_EXCEEDS_LIMIT, "The number of fd exceeds the limit", "FdRs"},
    {REASON_APP_HIGH_POWER_NOTIFY_KILL, "App High Power Notify Kill", "CPUHighloadNotify"},
    {REASON_HIGH_POWER_CONSUMPTION_KILL, "High Power Consumption Kill", "CPUHighloadUserRequest"},
    {REASON_ILLEGAL_AUDIO_RENDERER_BY_SUSPEND, "ILLEGAL_AUDIO_RENDERER_BY_SUSPEND", "IllegalAudioRendererBySuspend"},
    {REASON_ILLEGAL_AUDIO_CAPTURER_BY_SUSPEND, "ILLEGAL_AUDIO_CAPTURER_BY_SUSPEND", "IllegalAudioCapturerBySuspend"},
    {REASON_IO_MANAGE_CONTROL, "IO Manage Control", "IOHighload"},
    {REASON_BUSINESS_THREAD_BLOCK_6S, "BUSINESS_THREAD_BLOCK_6S", "Appfreeze"},
    {REASON_BUSINESS_INPUT_BLOCK, "BUSINESS_INPUT_BLOCK", "Appfreeze"},
    {REASON_MALICIOUS_CONTINUOUSTASK_ACTIVE, "MALICIOUS_CONTINUOUSTASK_ACTIVE", "MaliciousContinuoustaskActive"},
    {REASON_RS_TRANSACTION_DATA_OVERLIMIT, "RS_TRANSACTION_DATA_OVERLIMIT", "RsDataOverflow"},

    {REASON_CONTINUOUSLY_WAKEUP_ABNORMAL, "ContinuouslyWakeupAbnormal", "PowerSaveClean"},
    {REASON_BACKGROUND_FREEZE_ABNORMAL, "BackgroundFreezeAbnormal", "PowerSaveClean"},
    {REASON_GNSS_WORK_TIME_ABNORMAL, "GnssWorkTimeAbnormal", "PowerSaveClean"},
    {REASON_BLUETOOTH_WORK_TIME_ABNORMAL, "BluetoothWorkTimeAbnormal", "PowerSaveClean"},
    {REASON_RUNNING_LOCK_ABNORMAL, "RunningLockAbnormal", "PowerSaveClean"},
    {REASON_KERNEL_WAKELOCK_ABNORMAL, "KernelWakelockAbnormal", "PowerSaveClean"},
    {REASON_POWER_SAVE_CLEAN_EXTREME_POWER_SAVE_CLEAN, "PowerSaveClean ExtremePowerSaveClean", "PowerSaveClean"},
    {REASON_MODULE_POWER_CONSUMPTION_ABNORMAL, "Module Power Consumption Abnormal", "PowerSaveClean"},
    {REASON_POWER_ABNORMAL, "Power Abnormal", "PowerSaveClean"},
    {REASON_MEMORY_PRESSURE, "Memory Pressure", "LowMemoryKill"},
    {REASON_STABILITY_CHECK_KILL, "StabilityCheckKill", "LowMemoryKill"},
    {REASON_APP_LIMIT_KILL, "APP_LIMIT_KILL", "LowMemoryKill"},
    {REASON_SWAP_FULL, "SWAP_FULL", "SwapFull"},
    {REASON_SYSTEM_CAMERA_QUICK_KILL, "SYSTEM_CAMERA_QUICK_KILL", "LowMemoryKill"},
    {REASON_OTHER_CAMERA_QUICK_KILL, "OTHER_CAMERA_QUICK_KILL", "LowMemoryKill"},
    {REASON_ALIVE_APP_COUNT_KILL, "AliveAppCountKill", "LowMemoryKill"},
    {REASON_LOW_FREE_KILL, "LowFreeKill", "LowMemoryKill"},
    {REASON_ZSWAPD_PRESSURE, "ZSWAPD_PRESSURE", "LowMemoryKill"},
    {REASON_BIG_MEMORY_KILL, "BIG_MEMORY_KILL", "LowMemoryKill"},
    {REASON_GPU_LEAK_KILL, "GPU_LEAK_KILL", "LowMemoryKill"},
    {REASON_MEMORY_LIMIT_KILL, "MemoryLimitKill", "LowMemoryKill"},
    {REASON_PC_BIG_MEMORY_KILL, "PcBigMemoryKill", "LowMemoryKill"},
    {REASON_GPU_TEXTURE_MEM_LEAK_KILL, "GpuTextureMemLeakKill", "LowMemoryKill"},
    {REASON_ANCO_START_QUICK_KILL, "AncoStartQuickKill", "LowMemoryKill"},
    {REASON_BG_DEPDED_MINI_SYS_KILL, "BgDepdedMiniSysKill", "LowMemoryKill"},
    {REASON_FORE_UNDEPD_MINI_SYS_KILL, "ForeUndepdMiniSysKill", "LowMemoryKill"},
    {REASON_TEMPERATURE_CONTROL, "Temperature Control", "HighTemperature"},
    {REASON_CPU_EXT_HIGHLOAD, "CPU_EXT Highload", "PowerSaveClean"},
    {REASON_SCREEN_OFF_CLEAN, "Screen Off Clean", "PowerSaveClean"},
    {REASON_SLEEP_CLEAN, "Sleep Clean", "PowerSaveClean"},
    {REASON_INACTIVE_CLEAN, "Inactive Clean", "PowerSaveClean"},
    {REASON_STANDBY_FAILED_CONTINUOUSLY_WAKEUP_ABNORMAL, "StandbyFailedContinuouslyWakeupAbnormal", "PowerSaveClean"},
    {REASON_SWITCH_TO_PENGLAI_MODE, "Switch To Penglai Mode", "PowerSaveClean"},
    {REASON_HEAVY_LOAD_MUTEX_KILL, "HeavyLoad Mutex Kill", "PowerSaveClean"},
    {REASON_TRANSIENT_TASK_TIMEOUT, "TRANSIENT_TASK_TIMEOUT", "TransientTaskTimeout"},
    {REASON_RESOURCE_OVERLIMIT, "Resource Overlimit", "LowMemoryKill"},
    {REASON_RESOURCE_LEAK_STANDBY_CLEAN, "ResourceLeak:Standby Clean", "PowerSaveClean"},
    {REASON_SYSFREEZE_KILL_PEER_BINDER, "SysfreezeKillPeerBinder", "LowMemoryKill"},
    {REASON_TOO_MANY_READY_THREADS, "TooManyReadyThreads", "TooManyReadyThreads"},
    {REASON_JS_HEAP_SLEEP_CLEAN_KILL, "Js_Heap_Sleep_Clean_Kill", "JsHeapSleepCleanKill"},
    {REASON_HARDWARE_DECODING_RESOURCES_LIMIT, "HARDWARE_DECODING_RESOURCES_LIMIT", "HardwareDecodingResourcesLimit"},
    {REASON_APPRECOVERY_NOTIFYAPP_OVER_LIMIT, "AppRecoveryNotifyAppOverLimit", "AppRecoveryNotifyAppOverLimit"},
    {REASON_EXTREME_POWER_SAVE_CLEAN, "ExtremePowerSaveClean", "PowerSaveClean"},
    {REASON_RS_VULKAN_ERROR, "RS_VULKAN_ERROR", "GpuError"},
    {REASON_NOT_ATTACHED_TO_STATUS_BAR, "NotAttachedToStatusBar", "NotAttachedToStatusBar"},
    {REASON_ACCOUNT_COLD_SWITCH_KILL, "ACCOUNT_COLD_SWITCH_KILL", "LowMemoryKill"},
    {REASON_GPU_MEMORY_READ_ERR, "GPU_MEMORY_READ_ERR", "LowMemoryKill"},
    {REASON_DMA_MEMORY_READ_ERR, "DMA_MEMORY_READ_ERR", "LowMemoryKill"},
    {REASON_MSDP_BOTTOM_N_QUICK_KILL, "MSDP_BOTTOM_N_QUICK_KILL", "LowMemoryKill"},
    {REASON_LOW_MEMORY_ANCO_CACHED_KILL, "LOW_MEMORY_ANCO_CACHED_KILL", "LowMemoryKill"},
    {REASON_MINI_SYS_ANCO_SYS_PROC_KILL, "MINI_SYS_ANCO_SYS_PROC_KILL", "LowMemoryKill"},
    {REASON_POWER_SAVE_CLEAN, "PowerSaveClean", "PowerSaveClean"},
    {REASON_VRS_KILL_PROCESS, "VRS_KILL_PROCESS", "VrsKill"},
    {REASON_APPSPAWN_STOP, "APPSPAWN_STOP", "AppspawnStop"},
    {REASON_INACTIVE_PROC_KILL, "InactiveProcKill", "LowMemoryKill"},

    {REASON_OOM_KILLER, "OOM_KILLER", "OomKiller"},
    {REASON_CPA_KILLER, "CPA_KILLER", "CpaKiller"},
    {REASON_LOW_MEMORY_KILLER, "LowMemoryKill", "LowMemoryKill"},
    {REASON_CPU_HIGHLOAD_KILLER, "CPU Highload", "CPUHighload"},
    {REASON_RSS_THRESHOLD_KILLER, "RSS_THRESHOLD_KILLER", "RssThresholdKiller"},
    {REASON_ASHMEM_THRESHOLD_KILLER, "ASHMEM_THRESHOLD_KILLER", "AshmemKiller"},
    {REASON_GPU_THRESHOLD_KILLER, "GPU_THRESHOLD_KILLER", "GpuKiller"},
    {REASON_DMA_THRESHOLD_KILLER, "DMA_THRESHOLD_KILLER", "DmaKiller"},
    {REASON_THREAD_THRESHOLD_KILLER, "THREAD_THRESHOLD_KILLER", "ThreadKiller"},
    {REASON_KERNEL_ZONE_LOW, "Kernel Zone Low", "ResourceLeak(KernelZoneLeak)"},
    {REASON_APP_SANDBOX_UNINSTALL, "VolumeLogout", "Logout"},
    {REASON_STORAGE_CARD_UNINSTALL, "VolumeUmount", "UninstallStorage"},
};

constexpr size_t REASON_CONFIG_COUNT = sizeof(KILL_REASON_CONFIG) / sizeof(KILL_REASON_CONFIG[0]);

constexpr bool IsConfigSorted()
{
    for (size_t i = 1; i < REASON_CONFIG_COUNT; i++) {
        if (KILL_REASON_CONFIG[i - 1].id > KILL_REASON_CONFIG[i].id) {
            return false;
        }
    }
    return true;
}
static_assert(IsConfigSorted(), "KILL_REASON_CONFIG must be sorted by id");

// ===== FAST LOOKUP v1: compile-time segmented direct index =================
// The id space is documented as 1000-wide ranges (1-9, 1000-1009, 2000-2024,
// 3000-3054, 4000-4011) and ids inside a range are contiguous. That makes
// id -> &KILL_REASON_CONFIG[i] computable with two independent loads and no loop.
constexpr int32_t SEG_WIDTH = 1000;
constexpr int32_t ID_LIMIT = 5000;                       // exclusive upper bound
constexpr int32_t SEG_NUM = ID_LIMIT / SEG_WIDTH;

struct Segment {
    int32_t base;   // first id of the segment, -1 when the segment is empty
    int32_t count;  // number of entries in the segment
    int32_t offset; // index of the first entry inside KILL_REASON_CONFIG
};

constexpr Segment MakeSegment(int32_t seg)
{
    Segment s{-1, 0, 0};
    for (size_t i = 0; i < REASON_CONFIG_COUNT; ++i) {
        if (KILL_REASON_CONFIG[i].id / SEG_WIDTH == seg) {
            if (s.base < 0) {
                s.base = KILL_REASON_CONFIG[i].id;
                s.offset = static_cast<int32_t>(i);
            }
            s.count++;
        }
    }
    return s;
}

constexpr Segment SEGMENTS[SEG_NUM] = {
    MakeSegment(0), MakeSegment(1), MakeSegment(2), MakeSegment(3), MakeSegment(4)
};

// Correctness of the index depends on contiguity inside each segment; if someone
// later adds a non-contiguous id the build fails instead of returning a wrong reason.
constexpr bool IsSegmentContiguous()
{
    for (int32_t s = 0; s < SEG_NUM; ++s) {
        for (int32_t i = 0; i < SEGMENTS[s].count; ++i) {
            if (KILL_REASON_CONFIG[SEGMENTS[s].offset + i].id != SEGMENTS[s].base + i) {
                return false;
            }
        }
    }
    return true;
}
static_assert(IsSegmentContiguous(),
              "ids inside one 1000-range must stay contiguous: please update FindEntry");
static_assert(KILL_REASON_CONFIG[REASON_CONFIG_COUNT - 1].id < ID_LIMIT,
              "max kill id must stay inside the documented id space");

const KillReasonEntry* FindEntry(int32_t killId)
{
    if (static_cast<uint32_t>(killId) >= static_cast<uint32_t>(ID_LIMIT)) {
        return nullptr;
    }
    const Segment& seg = SEGMENTS[killId / SEG_WIDTH];
    const uint32_t off = static_cast<uint32_t>(killId - seg.base);
    if (off >= static_cast<uint32_t>(seg.count)) {
        return nullptr;
    }
    return &KILL_REASON_CONFIG[seg.offset + off];
}
}

std::string GetKillReason(int killId)
{
    const auto* entry = FindEntry(killId);
    return (entry != nullptr) ? entry->faultReason : INVALID_KILL_ID;
}

std::string GetAppExitReason(int killId)
{
    const auto* entry = FindEntry(killId);
    return (entry != nullptr) ? entry->appReason : INVALID_KILL_ID;
}
} // end of namespace ProcessKillReason
} // end of namespace HiviewDFX
} // end of namespace OHOS
