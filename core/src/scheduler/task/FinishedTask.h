// Copyright (C) 2019-2020 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include <memory>

#include "scheduler/task/Task.h"

namespace milvus::scheduler {

class FinishedTask : public Task {
 public:
    static std::shared_ptr<FinishedTask>
    Create(const TaskPtr& task);

 public:
    explicit FinishedTask(const TaskPtr& task);

    void
    Load(LoadType type, uint8_t device_id) override;

    void
    Execute() override;
};

}  // namespace milvus::scheduler
