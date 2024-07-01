/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "thread_sampler.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>
#include <set>
#include <string>

#include <sys/mman.h>
#include <sys/prctl.h>
#include <syscall.h>
#include <csignal>

#include "unwinder.h"
#include "dfx_regs.h"
#include "dfx_elf.h"
#include "dfx_frame_formatter.h"
#include "sample_stack_printer.h"
#include "thread_sampler_utils.h"
#include "file_ex.h"

namespace OHOS {
namespace HiviewDFX {

void ThreadSampler::ThreadSamplerSignalHandler(int sig, siginfo_t* si, void* context)
{
#if defined(__aarch64__)
    ThreadSampler::GetInstance().WriteContext(context);
#endif
}

ThreadSampler::ThreadSampler()
{
}

ThreadSampler::~ThreadSampler()
{
}

int ThreadSampler::FindUnwindTable(uintptr_t pc, UnwindTableInfo& outTableInfo, void *arg)
{
    UnwindInfo* unwindInfo = static_cast<UnwindInfo *>(arg);
    if (unwindInfo == nullptr) {
        XCOLLIE_LOGE("invalid FindUnwindTable param\n");
        return -1;
    }

    std::shared_ptr<DfxMap> map;
    if (unwindInfo->maps->FindMapByAddr(pc, map)) {
        if (map == nullptr) {
            XCOLLIE_LOGE("FindUnwindTable: map is nullptr\n");
            return -1;
        }
        auto elf = map->GetElf(getpid());
        if (elf != nullptr) {
            return elf->FindUnwindTableInfo(pc, map, outTableInfo);
        }
    }
    return -1;
}

int ThreadSampler::AccessMem(uintptr_t addr, uintptr_t *val, void *arg)
{
    UnwindInfo* unwindInfo = static_cast<UnwindInfo *>(arg);
    if (unwindInfo == nullptr || addr + sizeof(uintptr_t) < addr) {
        XCOLLIE_LOGE("invalid AccessMem param\n");
        return -1;
    }

    *val = 0;
    if (addr < unwindInfo->context->sp ||
        addr + sizeof(uintptr_t) >= unwindInfo->context->sp + STACK_BUFFER_SIZE) {
        return ThreadSampler::GetInstance().AccessElfMem(addr, val);
    } else {
        size_t stackOffset = addr - unwindInfo->context->sp;
        if (stackOffset >= STACK_BUFFER_SIZE) {
            XCOLLIE_LOGE("limit stack\n");
            return -1;
        }
        *val = *(reinterpret_cast<uintptr_t *>(&unwindInfo->context->buffer[stackOffset]));
    }
    return 0;
}

int ThreadSampler::GetMapByPc(uintptr_t pc, std::shared_ptr<DfxMap>& map, void *arg)
{
    UnwindInfo* unwindInfo = static_cast<UnwindInfo *>(arg);
    if (unwindInfo == nullptr) {
        XCOLLIE_LOGE("invalid GetMapByPc param\n");
        return -1;
    }

    return unwindInfo->maps->FindMapByAddr(pc, map) ? 0 : -1;
}

bool ThreadSampler::Init()
{
    if (init_) {
        return true;
    }

    if (!InitRecordBuffer()) {
        XCOLLIE_LOGE("Failed to InitRecordBuffer\n");
        Deinit();
        return false;
    }

    if (!InitUnwinder()) {
        XCOLLIE_LOGE("Failed to InitUnwinder\n");
        Deinit();
        return false;
    }

    pid_ = getprocpid();
    if (!InitUniqueStackTable()) {
        XCOLLIE_LOGE("Failed to InitUniqueStackTable\n");
        Deinit();
        return false;
    }

    if (!InstallSignalHandler()) {
        XCOLLIE_LOGE("Failed to InstallSignalHandler\n");
        Deinit();
        return false;
    }
    init_ = true;
    return true;
}

bool ThreadSampler::InitRecordBuffer()
{
    if (mmapStart_ != MAP_FAILED) {
        return true;
    }
    // create buffer
    bufferSize_ = SAMPLER_MAX_BUFFER_SZ * sizeof(struct ThreadUnwindContext);
    mmapStart_ = mmap(nullptr, bufferSize_,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmapStart_ == MAP_FAILED) {
        XCOLLIE_LOGE("Failed to create buffer for thread sampler!(%{public}d)\n", errno);
        return false;
    }

    prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, mmapStart_, bufferSize_, "sampler_buf");
    return true;
}

void ThreadSampler::ReleaseRecordBuffer()
{
    if (mmapStart_ == MAP_FAILED) {
        return;
    }
    // release buffer
    if (munmap(mmapStart_, bufferSize_) != 0) {
        XCOLLIE_LOGE("Failed to release buffer!(%{public}d)\n", errno);
        return;
    }
    mmapStart_ = MAP_FAILED;
}

bool ThreadSampler::InitUnwinder()
{
    accessors_ = std::make_shared<OHOS::HiviewDFX::UnwindAccessors>();
    accessors_->AccessReg = nullptr;
    accessors_->AccessMem = &ThreadSampler::AccessMem;
    accessors_->GetMapByPc = &ThreadSampler::GetMapByPc;
    accessors_->FindUnwindTable = &ThreadSampler::FindUnwindTable;
    unwinder_ = std::make_shared<Unwinder>(accessors_, true);

    maps_ = DfxMaps::Create();
    if (maps_ == nullptr) {
        XCOLLIE_LOGE("maps is nullptr\n");
        return false;
    }
    if (!maps_->GetStackRange(stackBegin_, stackEnd_)) {
        XCOLLIE_LOGE("Failed to get stack range\n");
        return false;
    }
    return true;
}

bool ThreadSampler::InitUniqueStackTable()
{
    uniqueStackTable_ = std::make_unique<UniqueStackTable>(pid_, uniqueStackTableSize_);
    if (!uniqueStackTable_->Init()) {
        XCOLLIE_LOGE("Failed to init unique_table\n");
        return false;
    }
    void* uniqueTableBufMMap = reinterpret_cast<void*>(uniqueStackTable_->GetHeadNode());
    prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, uniqueTableBufMMap, uniqueStackTableSize_, uniTableMMapName_.c_str());
    return true;
}

void ThreadSampler::DeinitUniqueStackTable()
{
    uniqueStackTable_.reset();
}

void ThreadSampler::DestroyUnwinder()
{
    maps_.reset();
    unwinder_.reset();
    accessors_.reset();
}

bool ThreadSampler::InstallSignalHandler()
{
    struct sigaction action {};
    sigfillset(&action.sa_mask);
    action.sa_sigaction = ThreadSampler::ThreadSamplerSignalHandler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(MUSL_SIGNAL_SAMPLE_STACK, &action, nullptr) != 0) {
        XCOLLIE_LOGE("Failed to register signal(%{public}d:%{public}d)", MUSL_SIGNAL_SAMPLE_STACK, errno);
        return false;
    }
    return true;
}

void ThreadSampler::UninstallSignalHandler()
{
    if (signal(MUSL_SIGNAL_SAMPLE_STACK, SIG_IGN) == SIG_ERR) {
        XCOLLIE_LOGE("Failed to unregister signal(%{public}d)", MUSL_SIGNAL_SAMPLE_STACK);
    }
}

int ThreadSampler::AccessElfMem(uintptr_t addr, uintptr_t *val)
{
    std::shared_ptr<DfxMap> map;
    if (maps_->FindMapByAddr(addr, map)) {
        if (map == nullptr) {
            XCOLLIE_LOGE("AccessElfMem: map is nullptr\n");
            return -1;
        }
        auto elf = map->GetElf(getpid());
        if (elf != nullptr) {
            uint64_t foff = addr - map->begin + map->offset - elf->GetBaseOffset();
            if (elf->Read(foff, val, sizeof(uintptr_t))) {
                return 0;
            }
        }
    }
    return -1;
}

ThreadUnwindContext* ThreadSampler::GetReadContext()
{
    ThreadUnwindContext* contextArray = static_cast<ThreadUnwindContext*>(mmapStart_);
    int32_t index = readIndex_;
    if (contextArray[index].requestTime == 0 || contextArray[index].snapshotTime == 0) {
        return nullptr;
    }

    ThreadUnwindContext* ret = &contextArray[index];
    readIndex_ = (index + 1) % SAMPLER_MAX_BUFFER_SZ;
    return ret;
}

ThreadUnwindContext* ThreadSampler::GetWriteContext()
{
    ThreadUnwindContext* contextArray = static_cast<ThreadUnwindContext*>(mmapStart_);
    int32_t index = writeIndex_;
    if (contextArray[index].requestTime > 0 &&
        (contextArray[index].snapshotTime == 0 || contextArray[index].processTime == 0)) {
        return nullptr;
    }
    return &contextArray[index];
}

__attribute__((no_sanitize("address"), no_sanitize("hwaddress"))) void ThreadSampler::WriteContext(void* context)
{
#if defined(__aarch64__)
    if (!init_) {
        return;
    }
#if defined(CONSUME_STATISTICS)
    uint64_t begin = GetCurrentTimeNanoseconds();
#endif
    ThreadUnwindContext* contextArray = static_cast<ThreadUnwindContext*>(mmapStart_);
    int32_t index = writeIndex_;
#if defined(CONSUME_STATISTICS)
    signalTimeCost_ += begin - contextArray[index].requestTime;
#endif

    // current buffer has not been processed, stop copy
    if (contextArray[index].snapshotTime > 0 && contextArray[index].processTime == 0) {
        return;
    }

    contextArray[index].fp = static_cast<ucontext_t*>(context)->uc_mcontext.regs[RegsEnumArm64::REG_FP];
    contextArray[index].lr = static_cast<ucontext_t*>(context)->uc_mcontext.regs[RegsEnumArm64::REG_LR];
    contextArray[index].sp = static_cast<ucontext_t*>(context)->uc_mcontext.sp;
    contextArray[index].pc = static_cast<ucontext_t*>(context)->uc_mcontext.pc;
    if (contextArray[index].sp < stackBegin_ ||
        contextArray[index].sp >= stackEnd_) {
        return;
    }

    uintptr_t curStackSz = stackEnd_ - contextArray[index].sp;
    uintptr_t cpySz = curStackSz  > STACK_BUFFER_SIZE ? STACK_BUFFER_SIZE : curStackSz;

    for (uintptr_t pos = 0; pos < cpySz; pos++) {
        reinterpret_cast<char*>(contextArray[index].buffer)[pos] =
            reinterpret_cast<const char*>(contextArray[index].sp)[pos];
    }

    uint64_t end = GetCurrentTimeNanoseconds();
    contextArray[index].snapshotTime = end;
    writeIndex_ = (index + 1) % SAMPLER_MAX_BUFFER_SZ;

#if defined(CONSUME_STATISTICS)
    copyStackCount_++;
    copyStackTimeCost_ += end - begin;
#endif
#endif  // #if defined(__aarch64__)
}

void ThreadSampler::SendSampleRequest()
{
    ThreadUnwindContext* ptr = GetWriteContext();
    if (ptr == nullptr) {
        return;
    }

    uint64_t ts = GetCurrentTimeNanoseconds();

    ptr->requestTime = ts;
    siginfo_t si {0};
    si.si_signo = MUSL_SIGNAL_SAMPLE_STACK;
    si.si_errno = 0;
    si.si_code = -1;
    if (syscall(SYS_rt_tgsigqueueinfo, pid_, pid_, si.si_signo, &si) != 0) {
        XCOLLIE_LOGE("Failed to queue signal(%{public}d) to %{public}d, errno(%{public}d).\n",
            si.si_signo, pid_, errno);
        return;
    }
#if defined (CONSUME_STATISTICS)
    requestCount_++;
#endif
}

void ThreadSampler::ProcessStackBuffer()
{
#if defined(__aarch64__)
    if (!init_) {
        XCOLLIE_LOGE("sampler has not initialized.\n");
        return;
    }
    while (true) {
        ThreadUnwindContext* context = GetReadContext();
        if (context == nullptr) {
            break;
        }

        struct TimeAndFrames taf;
        taf.requestTime = context->requestTime;
        taf.snapshotTime = context->snapshotTime;

#if defined(CONSUME_STATISTICS)
        uint64_t unwindStart = GetCurrentTimeNanoseconds();
#endif
        UnwindInfo unwindInfo = {
            .context = context,
            .maps = maps_.get(),
        };

        DoUnwind(context, unwinder_, unwindInfo);
#if defined(CONSUME_STATISTICS)
        uint64_t unwindEnd = GetCurrentTimeNanoseconds();
#endif
        /* for print full stack */
        auto frames = unwinder_->GetFrames();
        taf.frameList = frames;
        timeAndFrameList_.emplace_back(taf);
        /* for print tree format stack */
        auto pcs = unwinder_->GetPcs();
        uint64_t stackId = 0;
        auto stackIdPtr = reinterpret_cast<OHOS::HiviewDFX::StackId*>(&stackId);
        uniqueStackTable_->PutPcsInTable(stackIdPtr, pcs.data(), pcs.size());
        PutTimeInMap(stackIdTimeMap_, stackId, context->snapshotTime);
        
        uint64_t ts = GetCurrentTimeNanoseconds();
        context->processTime = ts;

#if defined(CONSUME_STATISTICS)
        processTimeCost_ += ts - unwindStart;
        processCount_++;
        unwindCount_++;
        unwindTimeCost_ += unwindEnd - unwindStart;
#endif  //#if defined(CONSUME_STATISTICS)
        context->requestTime = 0;
        context->snapshotTime = 0;
    }
#endif  // #if defined(__aarch64__)
}

int32_t ThreadSampler::Sample()
{
    if (!init_) {
        XCOLLIE_LOGE("sampler has not initialized.\n");
        return -1;
    }
#if defined(CONSUME_STATISTICS)
    sampleCount_++;
#endif
    SendSampleRequest();
    ProcessStackBuffer();
    return 0;
}

void ThreadSampler::ResetConsumeInfo()
{
#if defined(CONSUME_STATISTICS)
    sampleCount_ = 0;
    requestCount_ = 0;
    copyStackCount_ = 0;
    copyStackTimeCost_ = 0;
    processTimeCost_ = 0;
    processCount_ = 0;
    unwindCount_ = 0;
    unwindTimeCost_ = 0;
    signalTimeCost_ = 0;
#endif // #if defined(CONSUME_STATISTICS)
}

bool ThreadSampler::CollectStack(std::string& stack, bool treeFormat)
{
    ProcessStackBuffer();

    if (!init_) {
        XCOLLIE_LOGE("sampler has not initialized.\n");
    }

    stack.clear();
    if (timeAndFrameList_.empty() && stackIdTimeMap_.empty()) {
        if (!LoadStringFromFile("/proc/self/wchan", stack)) {
            XCOLLIE_LOGE("read file failed.\n");
        }
        if (stack.empty()) {
            stack += "empty";
        }
        stack += "\n";
#if defined(CONSUME_STATISTICS)
        ResetConsumeInfo();
#endif
        return false;
    }

#if defined(CONSUME_STATISTICS)
    uint64_t collectStart = GetCurrentTimeNanoseconds();
#endif
    auto printer = std::make_unique<SampleStackPrinter>(unwinder_, maps_);
    if (!treeFormat) {
        stack = printer->GetFullStack(timeAndFrameList_);
    } else {
        stack = printer->GetTreeStack(stackIdTimeMap_, uniqueStackTable_);
    }
    timeAndFrameList_.clear();
    stackIdTimeMap_.clear();
    
#if defined(CONSUME_STATISTICS)
    uint64_t collectEnd = GetCurrentTimeNanoseconds();
    uint64_t elapse = collectEnd - collectStart;
    XCOLLIE_LOGI("Sample count:%{public}llu\nRequest count:%{public}llu\n\
        Snapshot count:%{public}llu\nAverage copy stack time:%{public}llu ns\n",
        (unsigned long long)sampleCount_, (unsigned long long)requestCount_,
        (unsigned long long)copyStackCount_, (unsigned long long)copyStackTimeCost_ / copyStackCount_);
    XCOLLIE_LOGI("Average process time:%{public}llu ns\n", (unsigned long long)processTimeCost_/processCount_);
    XCOLLIE_LOGI("Average unwind time:%{public}llu ns\n", (unsigned long long)unwindTimeCost_/unwindCount_);
    XCOLLIE_LOGI("FormatStack time:%{public}llu ns\n", (unsigned long long)elapse);
    ResetConsumeInfo();
#endif
    return true;
}

void ThreadSampler::Deinit()
{
    DeinitUniqueStackTable();
    DestroyUnwinder();
    ReleaseRecordBuffer();
    UninstallSignalHandler();
    uniqueStackTableSize_ = DEFAULT_UNIQUE_STACK_TABLE_SIZE;
    uniTableMMapName_ = "hicollie_buf";
    init_ = false;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS