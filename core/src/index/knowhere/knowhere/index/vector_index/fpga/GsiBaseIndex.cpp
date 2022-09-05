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

#include "knowhere/index/vector_index/fpga/GsiBaseIndex.h"

#include <scheduler/job/SearchJob.h>
#include <scheduler/task/SearchTask.h>

#include <fstream>

namespace milvus {
namespace knowhere {

GsiBaseIndex::GsiBaseIndex(uint32_t dim) {
    num_bfeatures_ = dim;
}

GsiBaseIndex::~GsiBaseIndex() {
    // freeAllocatedMem();
}

void
GsiBaseIndex::AllocateMemory(const DatasetPtr& dataset, const Config& config) {
    // allocating memory for search queries
    setQueriesInfo(dataset, config);

    // allocating memory for result indices
    setResultIndicesStruct();

    // allocating memory for result distances
    setResultDistancesStruct();
}

void
GsiBaseIndex::setResultDistancesStruct() {
    distances_ = {.row_size = topK_,
                  .row_stride = topK_ * sizeof(float),
                  .num_rows = num_queries_,
                  .rows_f32 = (float*)calloc(topK_ * num_queries_, sizeof(float))};
    if (NULL == distances_.rows_f32) {
        // ERROR("no memory to allocate.");
    }
    // heap_allocations[counter_heap_allocations++] =  (void*)distances_.rows_f32;
}

void
GsiBaseIndex::setResultIndicesStruct() {
    indices_ = {.row_size = topK_,
                .row_stride = topK_ * sizeof(uint32_t),
                .num_rows = num_queries_,
                .rows_u32 = (uint32_t*)calloc(topK_ * num_queries_, sizeof(uint32_t))};
    if (NULL == indices_.rows_u32) {
        // ERROR("no memory to allocate.");
    }
    // heap_allocations[counter_heap_allocations++] = (void*)indices_.rows_u32;
}

void
GsiBaseIndex::setQueriesInfo(const DatasetPtr& dataset, const Config& config) {
    queries_ = {.row_size = num_bfeatures_,
                .row_stride = num_bytes_in_rec_,
                .num_rows = num_queries_,
                .rows_u1 = queries_.rows_u1 = (void*)dataset->Get<const void*>(meta::TENSOR)};
    // if (NULL == queries_.rows_u1){}
}

void
GsiBaseIndex::freeAllocatedMem() {
    std::cout << "cleaning APU allocation" << std::endl;
    // free((void*)indices_.rows_u32);
    // free((void*)distances_.rows_f32);

    for (size_t i = 0; i < counter_heap_allocations; i++) free(heap_allocations[i]);
}

int64_t*
GsiBaseIndex::convertToInt64_t(gsl_matrix_u32* indices, int64_t* ids_int64) {
    uint32_t* indices_buff = NULL;
    int64_t* indices_buff_int64 = NULL;
    uint32_t stride_64 = topK_ * sizeof(int64_t);

    for (unsigned int i = 0; i < num_queries_; ++i) {
        indices_buff = (uint32_t*)((char*)indices->rows_u32 + indices->row_stride * i);
        indices_buff_int64 = (int64_t*)((char*)ids_int64 + stride_64 * i);

        for (unsigned int j = 0; j < topK_; ++j) indices_buff_int64[j] = (int64_t)indices_buff[j];
    }
    return ids_int64;
}

BinarySet
GsiBaseIndex::Serialize(const Config& config) {
    return BinarySet();
}

void
GsiBaseIndex::Load(const BinarySet& set) {
}

void
GsiBaseIndex::Train(const DatasetPtr& dataset, const Config& config) {
}

void
GsiBaseIndex::AddWithoutIds(const DatasetPtr& dataset, const Config& config) {
}

int64_t
GsiBaseIndex::Dim() {
    return num_bfeatures_;
}

int64_t
GsiBaseIndex::Count() {
    return 0;
}

int64_t
GsiBaseIndex::Size() {
    return index_size_;
}

}  //  namespace knowhere
}  //  namespace milvus
