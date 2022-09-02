// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <cos_api.h>
#include <memory>
#include <string>
#include <vector>

#include "utils/Status.h"

namespace milvus {
namespace storage {

class COSClientWrapper {
 public:
    static COSClientWrapper&
    GetInstance() {
        static COSClientWrapper wrapper;
        return wrapper;
    }

    COSClientWrapper() {
        StartService();
    }

    ~COSClientWrapper() {
        StopService();
    }

    Status
    StartService();
    void
    StopService();

    Status
    CreateBucket();
    Status
    DeleteBucket();
    Status
    PutObjectFile(const std::string& object_key, const std::string& file_path);
    Status
    PutObjectStr(const std::string& object_key, const std::string& content);
    Status
    GetObjectFile(const std::string& object_key, const std::string& file_path);
    Status
    GetObjectStr(const std::string& object_key, std::string& content);
    Status
    ListObjects(std::vector<std::string>& object_list, const std::string& prefix = "");
    Status
    DeleteObject(const std::string& object_key);
    Status
    DeleteObjects(const std::string& prefix);

 private:
    std::string
    normalize_object_name(const std::string& object_key);
    qcloud_cos::CosConfig client_config_;

    std::shared_ptr<qcloud_cos::CosAPI> client_ptr_;

    std::string cos_region_;
    std::string cos_access_key_;
    std::string cos_secret_key_;
    std::string cos_bucket_;
    std::string cos_dest_domain_;
};

}  // namespace storage
}  // namespace milvus
