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

#ifndef RELIABILITY_HANDLER_CHECKER_H
#define RELIABILITY_HANDLER_CHECKER_H

#include <atomic>
#include <string>

#include "dumper.h"
#include "event_handler.h"
#include "xcollie_define.h"

namespace OHOS {
namespace HiviewDFX {
enum CheckStatus {
    COMPLETED = 0,
    WAITING = 1,
    WAITED_HALF = 2,
};

class HandlerChecker : public std::enable_shared_from_this<HandlerChecker> {
public:
    HandlerChecker(std::string name, std::shared_ptr<AppExecFwk::EventHandler> handler)
        : name_(name), handler_(handler)
    {
        if (name == IPC_FULL_TASK && handler == nullptr) {
            auto runner = AppExecFwk::EventRunner::Create(name);
            handler_ = std::make_shared<AppExecFwk::EventHandler>(runner);
        }
    };
    ~HandlerChecker() {};

public:
    void ScheduleCheck();
    int GetCheckState();
    std::shared_ptr<AppExecFwk::EventHandler> GetHandler() const;
    std::string GetDumpInfo();

private:
    std::string name_;
    std::shared_ptr<AppExecFwk::EventHandler> handler_;
    std::atomic<bool> isCompleted_ = true;
    bool taskSlow_ = false;
};

class HandlerDumper : public AppExecFwk::Dumper {
public:
    void Dump(const std::string &message) override;
    std::string GetTag() override;
    std::string GetDumpInfo();
private:
    std::string dumpInfo_;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
