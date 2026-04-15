#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <future>
#include <optional>

namespace deepiri {

struct GPUTask {
    std::string id;
    std::vector<float> input_data;
    std::string kernel_name;
    std::optional<int> preferred_device;
};

struct GPURESult {
    std::string task_id;
    std::vector<float> output_data;
    bool success;
    std::string error_message;
    double execution_time_ms;
};

class ZepGPUClient {
public:
    ZepGPUClient(const std::string& endpoint, bool use_websocket = false);
    ~ZepGPUClient();

    void connect();
    void disconnect();
    bool isConnected() const;

    std::future<GPURESult> submitTaskAsync(const GPUTask& task);
    GPURESult submitTask(const GPUTask& task);

    void setAuthToken(const std::string& token);
    void setTimeout(int timeout_ms);

    std::vector<std::string> listAvailableKernels();
    std::string getDeviceInfo();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}