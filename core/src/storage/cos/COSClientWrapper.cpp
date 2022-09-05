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

#include "storage/cos/COSClientWrapper.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

#include "config/Config.h"
#include "utils/Error.h"
#include "utils/Log.h"

namespace milvus {
namespace storage {

Status
COSClientWrapper::StartService() {
    server::Config& config = server::Config::GetInstance();

    config.GetStorageConfigCOSRegion(cos_region_);
    config.GetStorageConfigCOSSecretId(cos_access_key_);
    config.GetStorageConfigCOSSecretKey(cos_secret_key_);
    config.GetStorageConfigCOSBucket(cos_bucket_);
    config.GetStorageConfigCOSDestDomain(cos_dest_domain_);

    client_config_.SetAccessKey(cos_access_key_);
    client_config_.SetSecretKey(cos_secret_key_);
    client_config_.SetRegion(cos_region_);
    if (!cos_dest_domain_.empty()) {
        client_config_.SetDestDomain(cos_dest_domain_);
    }

    client_ptr_ = std::make_shared<qcloud_cos::CosAPI>(client_config_);

    auto status = CreateBucket();
    if (!status.ok()) {
        return status;
    }
    return Status::OK();
}

void
COSClientWrapper::StopService() {
    client_ptr_ = nullptr;
}

Status
COSClientWrapper::CreateBucket() {
    qcloud_cos::PutBucketReq request{cos_bucket_};
    qcloud_cos::PutBucketResp response;

    const auto result = client_ptr_->PutBucket(request, &response);
    if (!result.IsSucc()) {
        if (result.GetErrorCode() != "BucketAlreadyExists") {
            LOG_STORAGE_WARNING_ << "ERROR: CreateBucket: " << result.GetErrorCode() << ": " << result.GetErrorMsg();
            return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
        }
    }

    LOG_STORAGE_DEBUG_ << "CreateBucket '" << cos_bucket_ << "' successfully!";
    return Status::OK();
}

Status
COSClientWrapper::DeleteBucket() {
    qcloud_cos::DeleteBucketReq request{cos_bucket_};
    qcloud_cos::DeleteBucketResp response;

    const auto result = client_ptr_->DeleteBucket(request, &response);
    if (!result.IsSucc()) {
        LOG_STORAGE_WARNING_ << "ERROR: DeleteBucket: " << result.GetErrorCode() << ": " << result.GetErrorMsg();
        return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
    }

    LOG_STORAGE_DEBUG_ << "DeleteBucket '" << cos_bucket_ << "' successfully!";
    return Status::OK();
}

Status
COSClientWrapper::PutObjectFile(const std::string& object_name, const std::string& file_path) {
    struct stat buffer;
    if (stat(file_path.c_str(), &buffer) != 0) {
        std::string str = "File '" + file_path + "' not exist!";
        LOG_STORAGE_WARNING_ << "ERROR: " << str;
        return Status(SERVER_UNEXPECTED_ERROR, str);
    }

    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(file_path, std::ios::in | std::ios::binary);
    qcloud_cos::PutObjectByStreamReq request(cos_bucket_, normalize_object_name(object_name), *content);
    qcloud_cos::PutObjectByStreamResp response;

    const auto result = client_ptr_->PutObject(request, &response);
    if (!result.IsSucc()) {
        LOG_STORAGE_WARNING_ << "ERROR: PutObjectFile: " << object_name << ", " << result.GetErrorCode() << ": "
                             << result.GetErrorMsg();
        return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
    }

    LOG_STORAGE_DEBUG_ << "PutObjectFile '" << object_name << "' successfully!";
    return Status::OK();
}

Status
COSClientWrapper::PutObjectStr(const std::string& object_name, const std::string& content) {
    std::shared_ptr<std::iostream> content_stream = std::make_shared<std::stringstream>();
    *content_stream << content;
    qcloud_cos::PutObjectByStreamReq request(cos_bucket_, normalize_object_name(object_name), *content_stream);
    qcloud_cos::PutObjectByStreamResp response;

    const auto result = client_ptr_->PutObject(request, &response);
    if (!result.IsSucc()) {
        LOG_STORAGE_WARNING_ << "ERROR: PutObject: " << object_name << ", " << result.GetErrorCode() << ": "
                             << result.GetErrorMsg();
        return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
    }

    LOG_STORAGE_DEBUG_ << "PutObjectFile '" << object_name << "' successfully!";
    return Status::OK();
}

Status
COSClientWrapper::GetObjectFile(const std::string& object_name, const std::string& file_path) {
    auto stream = std::make_shared<std::fstream>(
        file_path, std::ios_base::out | std::ios_base::in | std::ios_base::trunc | std::ios_base::binary);
    qcloud_cos::GetObjectByStreamReq request(cos_bucket_, normalize_object_name(object_name), *stream);
    qcloud_cos::GetObjectByStreamResp response;

    const auto result = client_ptr_->GetObject(request, &response);
    if (!result.IsSucc()) {
        LOG_STORAGE_WARNING_ << "ERROR: GetObjectFile: " << object_name << ", " << result.GetErrorCode() << ": "
                             << result.GetErrorMsg();
        return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
    }

    LOG_STORAGE_DEBUG_ << "GetObjectFile '" << object_name << "' successfully!";
    return Status::OK();
}

Status
COSClientWrapper::GetObjectStr(const std::string& object_name, std::string& content) {
    auto stream = std::make_shared<std::ostringstream>();
    qcloud_cos::GetObjectByStreamReq request(cos_bucket_, normalize_object_name(object_name), *stream);
    qcloud_cos::GetObjectByStreamResp response;

    const auto result = client_ptr_->GetObject(request, &response);
    if (!result.IsSucc()) {
        LOG_STORAGE_WARNING_ << "ERROR: GetObject: " << object_name << ", " << result.GetErrorCode() << ": "
                             << result.GetErrorMsg();
        return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
    }

    content = stream->str();
    LOG_STORAGE_DEBUG_ << "GetObject '" << object_name << "' successfully!";
    return Status::OK();
}

Status
COSClientWrapper::ListObjects(std::vector<std::string>& object_list, const std::string& prefix) {
    qcloud_cos::GetBucketReq request(cos_bucket_);
    request.SetPrefix(normalize_object_name(prefix) + '/');
    qcloud_cos::GetBucketResp response;

    auto result = client_ptr_->GetBucket(request, &response);
    if (!result.IsSucc()) {
        LOG_STORAGE_WARNING_ << "ERROR: GetBucket: " << result.GetErrorCode() << ": " << result.GetErrorMsg();
        return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
    }

    for (const auto& content : response.GetContents()) {
        object_list.emplace_back(content.m_key);
    }

    if (prefix.empty()) {
        LOG_STORAGE_DEBUG_ << "ListObjects '" << cos_bucket_ << "' successfully!";
    } else {
        LOG_STORAGE_DEBUG_ << "ListObjects '" << cos_bucket_ << ":" << prefix << "' successfully!";
    }

    return Status::OK();
}

Status
COSClientWrapper::DeleteObject(const std::string& object_name) {
    qcloud_cos::DeleteObjectReq request(cos_bucket_, normalize_object_name(object_name));
    qcloud_cos::DeleteObjectResp response;
    const auto result = client_ptr_->DeleteObject(request, &response);

    if (!result.IsSucc()) {
        LOG_STORAGE_WARNING_ << "ERROR: DeleteObject: " << object_name << ", " << result.GetErrorCode() << ": "
                             << result.GetErrorMsg();
        return Status(SERVER_UNEXPECTED_ERROR, result.GetErrorMsg());
    }

    LOG_STORAGE_DEBUG_ << "DeleteObject '" << object_name << "' successfully!";
    return Status::OK();
}

Status
COSClientWrapper::DeleteObjects(const std::string& prefix) {
    std::vector<std::string> object_list;
    ListObjects(object_list, prefix);
    for (const auto& object : object_list) {
        auto status = DeleteObject(object);
        if (!status.ok()) {
            return status;
        }
    }
    return Status::OK();
}

std::string
COSClientWrapper::normalize_object_name(const std::string& object_key) {
    std::string object_name = object_key;
    if (object_name.empty()) {
        return object_name;
    }
    if (object_name[0] == '/') {
        object_name = object_name.substr(1);
    }
    if (object_name[object_name.size() - 1] == '/') {
        object_name = object_name.substr(0, object_name.size() - 1);
    }
    return object_name;
}

}  // namespace storage
}  // namespace milvus
