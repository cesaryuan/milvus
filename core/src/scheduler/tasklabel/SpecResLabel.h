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
#include <string>

#include "TaskLabel.h"
#include "scheduler/ResourceMgr.h"

// class Resource;
//
// using ResourceWPtr = std::weak_ptr<Resource>;

namespace milvus {
namespace scheduler {

class SpecResLabel : public TaskLabel {
 public:
    explicit SpecResLabel(const ResourceWPtr& resource, bool hybrid = false)
        : TaskLabel(TaskLabelType::SPECIFIED_RESOURCE), resource_(resource), hybrid_(hybrid) {
    }

    ResourceWPtr&
    resource() {
        return resource_;
    }

    std::string
    name() const override {
        return resource_.lock()->name();
    }

    bool
    IsHybrid() {
        return hybrid_;
    }

 private:
    bool hybrid_;
    ResourceWPtr resource_;
};

using SpecResLabelPtr = std::shared_ptr<SpecResLabel>();

}  // namespace scheduler
}  // namespace milvus
