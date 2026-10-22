#include "bluetooth.hpp"
#include <conio.h>
#include <cstdio>
#include <iostream>
#include <stdexcept>

namespace {
GUID serviceId = {0x789daf61, 0x733a, 0x486b, {0x95,0xd3,0x36,0x89,0xf1,0x61,0xba,0x20}};
void socketError(const char *operation) {
    throw std::runtime_error(std::string(operation) + " failed. Winsock error " +
                             std::to_string(WSAGetLastError()));
}
SOCKET createSocket() {
    SOCKET s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (s == INVALID_SOCKET) socketError("Bluetooth socket");
    BOOL enabled = TRUE;
    if (setsockopt(s, SOL_RFCOMM, SO_BTH_AUTHENTICATE,
                   reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == SOCKET_ERROR) {
        int error = WSAGetLastError(); closesocket(s); WSASetLastError(error);
        socketError("Bluetooth authentication option");
    }
    return s;
}
}

std::string Device::addressText() const {
    char text[20];
    std::snprintf(text, sizeof(text), "%012llX", (unsigned long long)data.address);
    return text;
}
BluetoothConnection::BluetoothConnection(SOCKET s) : socket_(s) {
    u_long enabled = 1;
    if (ioctlsocket(socket_, FIONBIO, &enabled) == SOCKET_ERROR) {
        int error = WSAGetLastError(); closesocket(socket_); WSASetLastError(error);
        socketError("Nonblocking mode");
    }
}
BluetoothConnection::~BluetoothConnection() {
    shutdown(socket_, SD_BOTH);
    closesocket(socket_);
}
int BluetoothConnection::sendBytes(const char *data, int length) {
    int n = send(socket_, data, length, 0);
    if (n == SOCKET_ERROR) return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
    return n == 0 ? -1 : n;
}
int BluetoothConnection::receiveBytes(char *data, int capacity) {
    int n = recv(socket_, data, capacity, 0);
    if (n == SOCKET_ERROR) return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
    return n == 0 ? -1 : n;
}
size_t BluetoothManager::scan(DeviceRecord *devices) {
    BLUETOOTH_DEVICE_SEARCH_PARAMS params{};
    params.dwSize = sizeof(params);
    params.fReturnAuthenticated = params.fReturnRemembered = TRUE;
    params.fReturnUnknown = params.fReturnConnected = params.fIssueInquiry = TRUE;
    params.cTimeoutMultiplier = 4;
    BLUETOOTH_DEVICE_INFO info{};
    info.dwSize = sizeof(info);
    HBLUETOOTH_DEVICE_FIND find = BluetoothFindFirstDevice(&params, &info);
    size_t count = 0;
    if (!find) {
        DWORD error = GetLastError();
        if (error != ERROR_NO_MORE_ITEMS)
            throw std::runtime_error("Scan failed. Windows error " + std::to_string(error));
        return 0;
    }
    do {
        if (devices_find(devices, count, info.Address.ullLong) >= 0) continue;
        DeviceRecord record{};
        record.address = info.Address.ullLong;
        record.paired = info.fAuthenticated;
        if (!WideCharToMultiByte(CP_UTF8, 0, info.szName, -1, record.name,
                                 sizeof(record.name), nullptr, nullptr))
            std::snprintf(record.name, sizeof(record.name), "Unnamed device");
        devices[count++] = record;
    } while (count < DEVICE_LIMIT && BluetoothFindNextDevice(find, &info));
    BluetoothFindDeviceClose(find);
    devices_sort(devices, count);
    return count;
}
void BluetoothManager::pair(const Device &device) {
    BLUETOOTH_DEVICE_INFO info{};
    info.dwSize = sizeof(info);
    info.Address.ullLong = device.data.address;
    DWORD result = BluetoothAuthenticateDeviceEx(nullptr, nullptr, &info, nullptr,
                       MITMProtectionRequiredBonding);
    if (result != ERROR_SUCCESS && result != ERROR_NO_MORE_ITEMS)
        throw std::runtime_error("Pairing failed/cancelled. Windows error " +
             std::to_string(result) + ". Try pairing in Windows Bluetooth Settings.");
}
SOCKET BluetoothManager::connectTo(const Device &device) {
    SOCKET s = createSocket();
    SOCKADDR_BTH address{};
    address.addressFamily = AF_BTH;
    address.btAddr = device.data.address;
    address.serviceClassId = serviceId;
    // port=0 asks Windows to discover the advertised service by UUID.
    if (connect(s, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        int error = WSAGetLastError(); closesocket(s); WSASetLastError(error);
        socketError("Connect (is the other laptop hosting Sanchar?)");
    }
    return s;
}
SOCKET BluetoothManager::waitForPeer(Device &peer) {
    SOCKET listener = createSocket();
    SOCKADDR_BTH local{};
    local.addressFamily = AF_BTH;
    local.port = BT_PORT_ANY;
    WSAQUERYSETW service{};
    CSADDR_INFO socketAddress{};
    wchar_t name[] = L"Sanchar Chat";
    bool registered = false;
    try {
        if (bind(listener, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR)
            socketError("Bind");
        int length = sizeof(local);
        if (getsockname(listener, reinterpret_cast<sockaddr*>(&local), &length) == SOCKET_ERROR)
            socketError("Local address");
        if (listen(listener, 1) == SOCKET_ERROR) socketError("Listen");
        socketAddress.LocalAddr.lpSockaddr = reinterpret_cast<sockaddr*>(&local);
        socketAddress.LocalAddr.iSockaddrLength = sizeof(local);
        socketAddress.iSocketType = SOCK_STREAM;
        socketAddress.iProtocol = BTHPROTO_RFCOMM;
        service.dwSize = sizeof(service);
        service.lpszServiceInstanceName = name;
        service.lpServiceClassId = &serviceId;
        service.dwNameSpace = NS_BTH;
        service.dwNumberOfCsAddrs = 1;
        service.lpcsaBuffer = &socketAddress;
        if (WSASetServiceW(&service, RNRSERVICE_REGISTER, 0) == SOCKET_ERROR)
            socketError("Advertise service");
        registered = true;
        std::cout << "Waiting for a paired laptop. Press Escape to cancel.\n";
        for (;;) {
            if (_kbhit() && _getch() == 27) throw std::runtime_error("Hosting cancelled.");
            fd_set ready; FD_ZERO(&ready); FD_SET(listener, &ready);
            timeval timeout{0, 100000};
            int result = select(0, &ready, nullptr, nullptr, &timeout);
            if (result == SOCKET_ERROR) socketError("Wait");
            if (!result) continue;
            SOCKADDR_BTH remote{};
            length = sizeof(remote);
            SOCKET client = accept(listener, reinterpret_cast<sockaddr*>(&remote), &length);
            if (client == INVALID_SOCKET) socketError("Accept");
            peer.data.address = remote.btAddr;
            std::snprintf(peer.data.name, sizeof(peer.data.name), "Peer %012llX",
                          (unsigned long long)remote.btAddr);
            WSASetServiceW(&service, RNRSERVICE_DELETE, 0);
            closesocket(listener);
            return client;
        }
    } catch (...) {
        if (registered) WSASetServiceW(&service, RNRSERVICE_DELETE, 0);
        closesocket(listener);
        throw;
    }
}
