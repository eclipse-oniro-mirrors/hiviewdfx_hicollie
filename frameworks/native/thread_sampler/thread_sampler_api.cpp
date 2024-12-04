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
#include "thread_sampler_api.h"

#include <cstring>

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int SUCCESS = 0;
constexpr int FAIL = -1;
}
int ThreadSamplerInit(int collectStackCount)
{
    return ThreadSampler::GetInstance().Init(collectStackCount) ? SUCCESS : FAIL;
}

int32_t ThreadSamplerSample()
{
    return ThreadSampler::GetInstance().Sample();
}

int ThreadSamplerCollect(char* stack, size_t size, int treeFormat)
{
    bool enableTreeFormat = (treeFormat == 1);
    std::string stk;
    int success = (ThreadSampler::GetInstance().CollectStack(stk, enableTreeFormat) ? SUCCESS : FAIL);
    size_t len = stk.size();
    if (strncpy_s(stack, size, stk.c_str(), len + 1) != EOK) {
        return FAIL;
    }
    return success;
}

int ThreadSamplerDeinit()
{
    return ThreadSampler::GetInstance().Deinit() ? SUCCESS : FAIL;
}

void ThreadSamplerSigHandler(int sig, siginfo_t* si, void* context)
{
    ThreadSampler::ThreadSamplerSignalHandler(sig, si, context);
}
}
}
