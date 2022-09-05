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

#include "server/Server.h"

#include <fcntl.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <cstring>
#include <unordered_map>

#include "server/init/InstanceLockCheck.h"

#ifdef MILVUS_GPU_VERSION
#include <vector>

#include "src/cache/GpuCacheMgr.h"
#endif

#include "config/Config.h"
#include "index/archive/KnowhereResource.h"
#include "metrics/Metrics.h"
#include "scheduler/SchedInst.h"
#include "server/DBWrapper.h"
#include "server/grpc_impl/GrpcServer.h"
#include "server/init/CpuChecker.h"
#include "server/init/GpuChecker.h"
#include "server/init/StorageChecker.h"
#include "server/web_impl/WebServer.h"
#include "src/version.h"
//#include "storage/s3/S3ClientWrapper.h"
#include "tracing/TracerUtil.h"
#include "utils/CommonUtil.h"
#include "utils/Log.h"
#include "utils/LogUtil.h"
#include "utils/SignalUtil.h"
#include "utils/TimeRecorder.h"

namespace milvus {
namespace server {

Server&
Server::GetInstance() {
    static Server server;
    return server;
}

void
Server::Init(const std::string& config_filename) {
    config_filename_ = config_filename;
}

Status
Server::Start() {
    try {
        /* Read config file */
        Status s = LoadConfig();
        if (!s.ok()) {
            std::cerr << "ERROR: Milvus server fail to load config file" << std::endl;
            return s;
        }

        Config& config = Config::GetInstance();

        std::string meta_uri;
        STATUS_CHECK(config.GetGeneralConfigMetaURI(meta_uri));
        if (meta_uri.length() > 6 && strcasecmp("sqlite", meta_uri.substr(0, 6).c_str()) == 0) {
            std::cout << "NOTICE: You are using SQLite as the meta data management. "
                         "We recommend change it to MySQL."
                      << std::endl;
        }

        /* Init opentracing tracer from config */
        std::string tracing_config_path;
        s = config.GetTracingConfigJsonConfigPath(tracing_config_path);
        tracing_config_path.empty() ? tracing::TracerUtil::InitGlobal()
                                    : tracing::TracerUtil::InitGlobal(tracing_config_path);

        /* Set timezone */
        std::string time_zone;
        s = config.GetGeneralConfigTimezone(time_zone);
        if (!s.ok()) {
            std::cerr << "Fail to get server config timezone" << std::endl;
            return s;
        }

        STATUS_CHECK(server::CommonUtil::SetTimezoneEnv(time_zone));

        /* Log path is defined in Config file, so InitLog must be called after LoadConfig */
        {
            std::unordered_map<std::string, int64_t> level_to_int{
                {"debug", 5}, {"info", 4}, {"warning", 3}, {"error", 2}, {"fatal", 1},
            };

            std::string level;
            bool trace_enable = false;
            bool debug_enable = false;
            bool info_enable = false;
            bool warning_enable = false;
            bool error_enable = false;
            bool fatal_enable = false;
            std::string logs_path;
            int64_t max_log_file_size = 0;
            int64_t delete_exceeds = 0;
            bool log_to_stdout = false;
            bool log_to_file = true;

            STATUS_CHECK(config.GetLogsLevel(level));
            switch (level_to_int[level]) {
                case 5:
                    debug_enable = true;
                case 4:
                    info_enable = true;
                case 3:
                    warning_enable = true;
                case 2:
                    error_enable = true;
                case 1:
                    fatal_enable = true;
                    break;
                default:
                    return Status(SERVER_UNEXPECTED_ERROR, "invalid log level");
            }

            STATUS_CHECK(config.GetLogsTraceEnable(trace_enable));
            STATUS_CHECK(config.GetLogsPath(logs_path));
            STATUS_CHECK(config.GetLogsMaxLogFileSize(max_log_file_size));
            STATUS_CHECK(config.GetLogsLogRotateNum(delete_exceeds));
            STATUS_CHECK(config.GetLogsLogToStdout(log_to_stdout));
            STATUS_CHECK(config.GetLogsLogToFile(log_to_file));
            InitLog(trace_enable, debug_enable, info_enable, warning_enable, error_enable, fatal_enable, logs_path,
                    max_log_file_size, delete_exceeds, log_to_stdout, log_to_file);
        }

        bool cluster_enable = false;
        std::string cluster_role;
        STATUS_CHECK(config.GetClusterConfigEnable(cluster_enable));
        STATUS_CHECK(config.GetClusterConfigRole(cluster_role));

        // std::string deploy_mode;
        // STATUS_CHECK(config.GetServerConfigDeployMode(deploy_mode));

        // if (deploy_mode == "single" || deploy_mode == "cluster_writable") {
        if ((not cluster_enable) || cluster_role == "rw") {
            std::string db_path;
            STATUS_CHECK(config.GetStorageConfigPath(db_path));

            try {
                // True if a new directory was created, otherwise false.
                boost::filesystem::create_directories(db_path);
            } catch (...) {
                return Status(SERVER_UNEXPECTED_ERROR, "Cannot create db directory");
            }

            s = InstanceLockCheck::Check(db_path);
            if (!s.ok()) {
                if (not cluster_enable) {
                    std::cerr << "single instance lock db path failed." << s.message() << std::endl;
                } else {
                    std::cerr << cluster_role << " instance lock db path failed." << s.message() << std::endl;
                }
                return s;
            }

            bool wal_enable = false;
            STATUS_CHECK(config.GetWalConfigEnable(wal_enable));

            if (wal_enable) {
                std::string wal_path;
                STATUS_CHECK(config.GetWalConfigWalPath(wal_path));

                try {
                    // True if a new directory was created, otherwise false.
                    boost::filesystem::create_directories(wal_path);
                } catch (...) {
                    return Status(SERVER_UNEXPECTED_ERROR, "Cannot create wal directory");
                }
                s = InstanceLockCheck::Check(wal_path);
                if (!s.ok()) {
                    if (not cluster_enable) {
                        std::cerr << "single instance lock wal path failed." << s.message() << std::endl;
                    } else {
                        std::cerr << cluster_role << " instance lock wal path failed." << s.message() << std::endl;
                    }
                    return s;
                }
            }
        }

        // print version information
        LOG_SERVER_INFO_ << "Milvus " << BUILD_TYPE << " version: v" << MILVUS_VERSION << ", built at " << BUILD_TIME;
#ifdef MILVUS_GPU_VERSION
        LOG_SERVER_INFO_ << "GPU edition";
#else
        LOG_SERVER_INFO_ << "CPU edition";
#endif
        STATUS_CHECK(StorageChecker::CheckStoragePermission());
        STATUS_CHECK(CpuChecker::CheckCpuInstructionSet());
#ifdef MILVUS_GPU_VERSION
        STATUS_CHECK(GpuChecker::CheckGpuEnvironment());
#endif
        /* record config and hardware information into log */
        LogConfigInFile(config_filename_);
        LogCpuInfo();
        LogConfigInMem();

        server::Metrics::GetInstance().Init();
        server::SystemInfo::GetInstance().Init();

        return StartService();
    } catch (std::exception& ex) {
        std::string str = "Milvus server encounter exception: " + std::string(ex.what());
        return Status(SERVER_UNEXPECTED_ERROR, str);
    }
}

void
Server::Stop() {
    std::cerr << "Milvus server is going to shutdown ..." << std::endl;

#ifdef MILVUS_GPU_VERSION
    {
        auto& config = server::Config::GetInstance();
        std::vector<int64_t> gpus;
        Status s = config.GetGpuResourceConfigSearchResources(gpus);
        if (s.ok()) {
            for (auto& gpu_id : gpus) {
                cache::GpuCacheMgr::GetInstance(gpu_id)->ClearCache();
            }
        }
    }
#endif

    StopService();

    std::cerr << "Milvus server exit..." << std::endl;
}

Status
Server::LoadConfig() {
    Config& config = Config::GetInstance();
    Status s = config.LoadConfigFile(config_filename_);
    if (!s.ok()) {
        std::cerr << s.message() << std::endl;
        return s;
    }

    s = config.ValidateConfig();
    if (!s.ok()) {
        std::cerr << "Config check fail: " << s.message() << std::endl;
        return s;
    }
    return milvus::Status::OK();
}

Status
Server::StartService() {
    Status stat;
    stat = engine::KnowhereResource::Initialize();
    if (!stat.ok()) {
        LOG_SERVER_ERROR_ << "KnowhereResource initialize fail: " << stat.message();
        goto FAIL;
    }

    scheduler::StartSchedulerService();

    stat = DBWrapper::GetInstance().StartService();
    if (!stat.ok()) {
        LOG_SERVER_ERROR_ << "DBWrapper start service fail: " << stat.message();
        goto FAIL;
    }

    grpc::GrpcServer::GetInstance().Start();
    web::WebServer::GetInstance().Start();

    // stat = storage::S3ClientWrapper::GetInstance().StartService();
    // if (!stat.ok()) {
    //     LOG_SERVER_ERROR_ << "S3Client start service fail: " << stat.message();
    //     goto FAIL;
    // }

    //    search::TaskInst::GetInstance().Start();

    return Status::OK();
FAIL:
    std::cerr << "Milvus initializes fail: " << stat.message() << std::endl;
    return stat;
}

void
Server::StopService() {
    //    search::TaskInst::GetInstance().Stop();
    // storage::S3ClientWrapper::GetInstance().StopService();
    web::WebServer::GetInstance().Stop();
    grpc::GrpcServer::GetInstance().Stop();
    DBWrapper::GetInstance().StopService();
    scheduler::StopSchedulerService();
    engine::KnowhereResource::Finalize();
}

}  // namespace server
}  // namespace milvus
