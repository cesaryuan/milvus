// Copyright (C) 2019-2020 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied. See the License for the specific language governing permissions and limitations under the License

#ifndef FPGA_H
#define FPGA_H

#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

#define FPGA_BASE_ADDR_0 0x000000000
#define FPGA_BASE_ADDR_1 0x400000000
#define FPGA_BASE_ADDR_2 0x800000000
#define FPGA_BASE_ADDR_3 0xc00000000

#define REG_BASE_ADDR 0x400000
#define PARA_BASE_ADDR 0x500000
#define DATA_BASE_BASE_ADDR 0x501000

#define Time_REG_ADDR 0xFFFE0
#define TRIGGER_REG_ADDR 0xFFFD0

// device clock frequency :MHz
#define FPGA_RATE 200

namespace Fpga {

using idx_t = int64_t;

class FpgaInterface {
 public:
    idx_t nb;  // number of dataset
    idx_t nq;
    idx_t nlist;
    idx_t topk;
    idx_t d;  // dimension
    idx_t m;  // sub-quantizer
    idx_t nbits;

    bool verbose;      // flag to output debug msg
    bool first_query;  // flag to firt query
    idx_t last_field;  // record to last flush field

    faiss::IndexIVFPQ* index_ptr;

    // data in cal time
    idx_t ci_s;
    idx_t tm2_s;
    idx_t db_s;
    idx_t pq_s;
    idx_t que_s;
    idx_t step1_s;
    idx_t step2_s;
    idx_t step3_s;
    idx_t final_s;
    std::vector<idx_t> cfg_table;
    std::vector<idx_t> all_index_id;  // id map between initial and added, (nb, )
    std::vector<int> all_list_size;   // dataset number in each cluster, (nlist, )
    FpgaInterface() {
        pre_nprobe = 0;
    }
    void
    setIndex(faiss::IndexIVFPQ* index) {
        nb = index->ntotal;
        nlist = index->nlist;
        d = index->d;
        m = index->pq.M;
        nbits = index->pq.nbits;
        first_query = true;
        verbose = false;
        index_ptr = index;

        all_index_id.resize(nb);
        all_list_size.resize(nlist);
        GetDevAddr();
    }

    virtual ~FpgaInterface();

    void
    GetDevAddr();
    void
    GetConfigTable(idx_t nprobe);

    //  h2c0 functions
    void
    TransFile2FPGA(const char* filename, loff_t base, size_t offset);
    char*
    LoadFile2Mem(const char* filename, size_t& file_size, char* buffer);
    void
    TransMem2FPGA(char* buffer, size_t size, loff_t base_addr);

    // c2ch functions
    void
    WriteFileFromFPGA(const char* filename, size_t size, loff_t base, size_t offset);
    char*
    LoadFPGA2Mem(loff_t fpga_addr, char* buffer, size_t size);
    void
    SaveMem2File(char* buffer, size_t size, const char* filename);

    // control functions
    int
    OpenCtrlDev();
    int
    CloseCtrlDev(int fd);
    int
    WritePara(int fd, uint32_t addr, uint32_t val);
    int
    ReadPara(int fd, uint32_t addr);
    int
    WriteReg(int fd, uint32_t addr, uint32_t val);
    int
    ReadReg(int fd, uint32_t addr);

    void
    CopyIndexToFPGA();
    bool
    BatchAnnQuery(idx_t nprobe, idx_t batch_size, float* items, idx_t expect_k, std::vector<idx_t>& id_vec,
                  std::vector<float>& dis_vec);
    void
    TriggerSearch(int fd);
    uint32_t
    GetSearchTime(int fd);
    void
    StartFPGASearch();

    int
    GetDatabaseListAlign(std::vector<int>& data_list, uint32_t align, std::vector<int>& align_list);
    void
    GetDatabaseListSum(std::vector<int>& data_vec, std::vector<int>& sum_vec);
    void
    GetProbeDatabaseAddressList(uint32_t base_addr, std::vector<int>& data_vec, int align,
                                std::vector<uint32_t>& addr_vec, uint32_t m);
    void
    GetProbeTerm2AddressList(uint32_t base_addr, std::vector<int>& data_vec, int align,
                             std::vector<uint32_t>& addr_vec);
    int
    CfgDataBaseNumReal(int fd);
    void
    TransferCfg();
    void
    FlushResultMem();
    void
    ReadResultFile(std::vector<idx_t>& id_vec, std::vector<float>& dis_vec);

 private:
    int pre_nprobe;
};
using FpgaInterfacePtr = std::shared_ptr<FpgaInterface>;
}  // namespace Fpga
#endif
