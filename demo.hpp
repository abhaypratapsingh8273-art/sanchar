#pragma once
#include "connection.hpp"
#include <algorithm>
#include <cstring>
#include <string>

// Learning aid only: no Bluetooth and no remote person.
// The same MessagePump and C queues are used by demo and real chat.
class DemoConnection : public ConnectionHandler {
    std::string input_, replies_;
public:
    int sendBytes(const char *data, int length) override {
        int n = std::min(length, 16); // Deliberately demonstrate partial writes.
        input_.append(data, n);
        size_t end;
        while ((end = input_.find('\n')) != std::string::npos) {
            replies_ += "[SIMULATED] Received: " + input_.substr(0, std::min(end, size_t(990))) + "\n";
            input_.erase(0, end + 1);
        }
        return n;
    }
    int receiveBytes(char *data, int capacity) override {
        int n = std::min(capacity, static_cast<int>(replies_.size()));
        std::memcpy(data, replies_.data(), n);
        replies_.erase(0, n);
        return n;
    }
};
