```cpp
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

#include <string>
#include "dsa.h"
#include "connection.hpp"

struct Device {
    DeviceRecord data{};

    std::string addressText() const;
};

class BluetoothConnection : public ConnectionHandler {
private:
    SOCKET socket_;

public:
    explicit BluetoothConnection(SOCKET socket);

    ~BluetoothConnection() override;

    BluetoothConnection(const BluetoothConnection&) = delete;
    BluetoothConnection& operator=(const BluetoothConnection&) = delete;

    int sendBytes(const char* data, int length) override;
    int receiveBytes(char* data, int capacity) override;
};

class BluetoothManager {
public:
    size_t scan(DeviceRecord* devices);

    void pair(const Device& device);

    SOCKET connectTo(const Device& device);

    SOCKET waitForPeer(Device& peer);
};
```
