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

#include "storage/cos/COSIOReader.h"
#include <cstring>
#include "COSClientWrapper.h"

namespace milvus {
namespace storage {

bool
COSIOReader::open(const std::string& name) {
    name_ = name;
    pos_ = 0;
    return (COSClientWrapper::GetInstance().GetObjectStr(name_, buffer_).ok());
}

void
COSIOReader::read(void* ptr, int64_t size) {
    memcpy(ptr, buffer_.data() + pos_, size);
    pos_ += size;
}

void
COSIOReader::seekg(int64_t pos) {
    pos_ = pos;
}

int64_t
COSIOReader::length() {
    return buffer_.length();
}

void
COSIOReader::close() {
}

}  // namespace storage
}  // namespace milvus
