#include "bluetooth.hpp"
#include "chat.hpp"
#include "demo.hpp"
#include <conio.h>
#include <filesystem>
#include <iostream>
#include <sstream>

class ChatApp {
    BluetoothManager bluetooth_;
    DeviceRecord devices_[DEVICE_LIMIT]{};
    size_t count_ = 0;
    static void printRecord(const ChatRecord &record) {
        std::time_t time = static_cast<std::time_t>(record.timestamp);
        std::tm *local = std::localtime(&time);
        if (local) std::cout << '[' << std::put_time(local, "%d-%m %H:%M:%S") << "] ";
        std::cout << (record.outgoing ? "You (sent): " : "Peer: ") << record.text << '\n';
    }
    static void showHistory(const ChatHistory &history) {
        if (!history.first()) std::cout << "No saved messages.\n";
        for (const HistoryNode *node = history.first(); node; node = node->next)
            printRecord(node->record);
    }
    Device chooseDevice() {
        if (!count_) throw std::runtime_error("Scan for devices first.");
        std::cout << "Device number: ";
        std::string line; std::getline(std::cin, line);
        std::istringstream input(line);
        size_t index; char extra;
        if (!(input >> index) || (input >> extra) || index < 1 || index > count_)
            throw std::runtime_error("Invalid device number.");
        return Device{devices_[index - 1]};
    }
    static std::string historyPath(const Device &peer) {
        return "history/" + peer.addressText() + ".txt";
    }
    void chat(ConnectionHandler &connection, const Device &peer, bool demo = false) {
        ChatHistory history(demo ? "history/demo.txt" : historyPath(peer));
        MessagePump pump(connection);
        std::string input;
        bool extendedKey = false;
        if (demo) std::cout << "\nPRACTICE MODE - simulated replies on this laptop, NO Bluetooth.\n";
        else std::cout << "\nConnected to " << peer.data.name << " (" << peer.addressText() << ")\n";
        std::cout
                  << "Enter sends. /history shows saved messages. /quit leaves chat.\n"
                  << "Use printable English text (maximum 1023 characters).\n"
                  << (demo ? "History saves to history/demo.txt. An example is sent automatically.\n"
                           : "Sent means handed to Bluetooth; it is not a read/delivery receipt.\n");
        showHistory(history);
        if (demo) pump.enqueue("Hello Sanchar! This is a practice message.");
        std::cout << "> " << std::flush;
        auto onMessage = [&](const ChatRecord &record) {
            // Erase the input line, display a message, then redraw unfinished input.
            std::cout << '\r' << std::string(input.size() + 2, ' ') << '\r';
            if (!history.append(record)) std::cout << "Warning: could not save this message.\n";
            printRecord(record);
            std::cout << "> " << input << std::flush;
        };
        for (;;) {
            if (!pump.tick(onMessage)) {
                std::cout << "\nConnection ended or invalid data received. " << pump.pending()
                          << " outgoing message(s) unconfirmed.\n";
                break;
            }
            if (_kbhit()) {
                int c = _getch();
                if (extendedKey) { extendedKey = false; continue; }
                if (c == 0 || c == 224) { extendedKey = true; continue; }
                if (c == 13) {
                    std::cout << '\n';
                    if (input == "/quit") {
                        std::cout << "Leaving chat. " << pump.pending()
                                  << " pending message(s) discarded.\n";
                        break;
                    }
                    if (input == "/history") showHistory(history);
                    else if (!input.empty() && !pump.enqueue(input))
                        std::cout << "Outgoing queue full. Please retry your message.\n";
                    input.clear(); std::cout << "> " << std::flush;
                } else if (c == 8 && !input.empty()) {
                    input.pop_back(); std::cout << "\b \b" << std::flush;
                } else if (c >= 32 && c <= 126 && input.size() < TEXT_SIZE - 1) {
                    input += static_cast<char>(c); std::cout << static_cast<char>(c) << std::flush;
                }
            }
            Sleep(5);
        }
    }
public:
    void runDemo() {
        std::filesystem::create_directories("history");
        DemoConnection connection;
        Device peer{};
        chat(connection, peer, true);
    }
    void run() {
        std::filesystem::create_directories("history");
        std::cout << "SANCHAR - Offline Bluetooth Chat\n";
        for (;;) {
            std::cout << "\n1. Scan/list devices\n2. Pair a device\n3. Host a chat\n"
                      << "4. Connect to a device\n5. View device history\n0. Exit\nChoice: ";
            std::string choice;
            if (!std::getline(std::cin, choice) || choice == "0") break;
            try {
                if (choice == "1") {
                    std::cout << "Scanning (several seconds)...\n";
                    count_ = bluetooth_.scan(devices_);
                    for (size_t i = 0; i < count_; ++i) {
                        Device device{devices_[i]};
                        std::cout << i + 1 << ". " << device.data.name << " ["
                                  << device.addressText() << "] "
                                  << (device.data.paired ? "paired" : "not paired") << '\n';
                    }
                    std::cout << count_ << " device(s). Remembered devices may be offline.\n";
                } else if (choice == "2") {
                    Device peer = chooseDevice(); bluetooth_.pair(peer);
                    std::cout << "Pairing completed. Scan again to refresh the status.\n";
                } else if (choice == "3") {
                    Device peer;
                    BluetoothConnection connection(bluetooth_.waitForPeer(peer));
                    chat(connection, peer);
                } else if (choice == "4") {
                    Device peer = chooseDevice();
                    std::cout << "Connecting (may take several seconds)...\n";
                    BluetoothConnection connection(bluetooth_.connectTo(peer));
                    chat(connection, peer);
                } else if (choice == "5") {
                    Device peer = chooseDevice(); ChatHistory history(historyPath(peer));
                    showHistory(history);
                } else std::cout << "Choose an option from the menu.\n";
            } catch (const std::exception &error) { std::cout << "Error: " << error.what() << '\n'; }
        }
    }
};

int main(int argc, char **argv) {
    if (argc == 2 && std::string(argv[1]) == "--demo") {
        try { ChatApp app; app.runDemo(); return 0; }
        catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    }
    WSADATA data{};
    int result = WSAStartup(MAKEWORD(2,2), &data);
    if (result != 0) { std::cerr << "Winsock startup failed: " << result << '\n'; return 1; }
    int exitCode = 0;
    try { ChatApp app; app.run(); }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; exitCode = 1; }
    WSACleanup();
    return exitCode;
}
