// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid Android TV native UDP transport (encrypted H264 + Opus RTP).
// Protocol reference: ZERODROID, Apache-2.0; ported to PSBox net/socket.
#pragma once

#include "boosteroid/native_contract.h"
#include "net/socket.h"
#include "platform/platform.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace xc::boosteroid {

// Decrypts a complete, lossless set of 1410-byte Android TV video shards.
// Does not recover Reed-Solomon parity shards yet; missing data is dropped.
bool decodeNativeVideoGroup(const std::vector<std::vector<uint8_t>>& shards,
                            const std::vector<uint8_t>& aesKey,
                            uint16_t groupId, std::vector<uint8_t>& output);

class NativeUdp {
public:
    explicit NativeUdp(EncodedMediaSink sink) : sink_(std::move(sink)) {}
    NativeUdp(const NativeUdp&) = delete;
    NativeUdp& operator=(const NativeUdp&) = delete;
    ~NativeUdp() { stop(); }

    bool start(const std::string& host, int videoPort, int audioPort,
               std::string& error);
    bool setKeyHex(const std::string& hex);
    void stop();
    bool running() const { return running_; }
    uint64_t videoPackets() const { return videoPackets_; }

private:
    struct Group {
        uint16_t id = 0, dataCount = 0, totalCount = 0;
        size_t receivedCount = 0;
        std::vector<std::vector<uint8_t>> shards;
        std::vector<bool> received;
    };
    void handleVideoPacket(const uint8_t* packet, size_t size);
    void videoLoop();
    void audioLoop();

    EncodedMediaSink sink_;
    net::Socket videoSocket_, audioSocket_;
    platform::Thread videoThread_, audioThread_;
    std::atomic<bool> stop_{false}, running_{false};
    std::atomic<uint64_t> videoPackets_{0};
    std::mutex groupMutex_, keyMutex_;
    std::map<uint16_t, Group> groups_;
    std::vector<uint8_t> key_;
};

} // namespace xc::boosteroid
