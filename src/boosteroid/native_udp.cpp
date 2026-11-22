// SPDX-License-Identifier: Apache-2.0
// Native Android TV UDP media implementation based on transport behavior in
// ZERODROID by elzerdodata: https://github.com/elzerdodata/ZERODROID
// Adapted to NimboPlay's PSBox socket, crypto and decoding boundaries.
#include "boosteroid/native_udp.h"

#include "net/socket.h"
#include "platform/platform.h"

#include <mbedtls/aes.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <utility>

namespace xc::boosteroid {
namespace {
uint16_t readLe16(const uint8_t* b) {
    return static_cast<uint16_t>(b[0]) |
           (static_cast<uint16_t>(b[1]) << 8);
}

uint32_t readBe32(const uint8_t* b) {
    return (static_cast<uint32_t>(b[0]) << 24) |
           (static_cast<uint32_t>(b[1]) << 16) |
           (static_cast<uint32_t>(b[2]) << 8) | b[3];
}

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool decodeChunk(const std::vector<uint8_t>& shard,
                 const std::vector<uint8_t>& key, uint16_t groupId,
                 uint16_t index, std::vector<uint8_t>& plain) {
    if (shard.size() != 1410 ||
        (key.size() != 16 && key.size() != 24 && key.size() != 32))
        return false;
    std::array<uint8_t, 16> iv{};
    iv[6] = static_cast<uint8_t>(groupId);
    iv[7] = static_cast<uint8_t>(groupId >> 8);
    iv[14] = static_cast<uint8_t>(index);
    iv[15] = static_cast<uint8_t>(index >> 8);
    plain.resize(1408);
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    const int init = mbedtls_aes_setkey_dec(
        &aes, key.data(), static_cast<unsigned int>(key.size() * 8));
    const int rc = init == 0
        ? mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 1408,
                                iv.data(), shard.data() + 2, plain.data())
        : init;
    mbedtls_aes_free(&aes);
    return rc == 0;
}

bool openUdpChannel(const std::string& host, int port, net::Socket& out,
                    std::string& error) {
    if (host.empty() || host.size() > 253 || port <= 0 || port > 65535) {
        error = "Endereço de UDP Boosteroid inválido.";
        return false;
    }
    uint32_t ip = 0;
    if (!platform::resolveIPv4(host, ip, error)) {
        error = "Não foi possível resolver o gateway UDP Boosteroid.";
        return false;
    }
    // platform::resolveIPv4 supplies the address in network byte order.
    const uint8_t* b = reinterpret_cast<const uint8_t*>(&ip);
    char dotted[16]{};
    std::snprintf(dotted, sizeof dotted, "%u.%u.%u.%u",
                  b[0], b[1], b[2], b[3]);
    out = net::createUdpSocket();
    if (!out.valid() || !net::connectSocket(out, std::string(dotted), port)) {
        error = "Não foi possível conectar ao canal UDP Boosteroid.";
        net::closeSocket(out);
        return false;
    }
    net::setRecvTimeout(out, 200);
    net::setRecvBufferSize(out, 512000);
    net::setNoSigpipe(out);
    static constexpr char hello[] = "ping";
    for (int i = 0; i < 10; ++i)
        net::sendNoSignal(out, hello, sizeof(hello) - 1);
    return true;
}

bool unpackOpusRtp(const uint8_t* data, size_t size,
                   const uint8_t*& payload, size_t& length,
                   uint32_t& timestamp) {
    if (!data || size < 12 || (data[0] & 0xc0) != 0x80 ||
        (data[1] & 0x7f) != 97) return false;
    const size_t cc = data[0] & 0x0f;
    size_t off = 12 + cc * 4;
    if (off > size) return false;
    if (data[0] & 0x10) {
        if (off + 4 > size) return false;
        const size_t words = (static_cast<size_t>(data[off + 2]) << 8) |
                              data[off + 3];
        if (words > (size - off - 4) / 4) return false;
        off += 4 + words * 4;
    }
    size_t end = size;
    if (data[0] & 0x20) {
        if (end <= off) return false;
        const uint8_t padding = data[end - 1];
        if (padding == 0 || padding > end - off) return false;
        end -= padding;
    }
    if (off >= end) return false;
    timestamp = readBe32(data + 4);
    payload = data + off;
    length = end - off;
    return true;
}
} // namespace

bool decodeNativeVideoGroup(const std::vector<std::vector<uint8_t>>& shards,
                            const std::vector<uint8_t>& aesKey,
                            uint16_t groupId, std::vector<uint8_t>& output) {
    output.clear();
    if (shards.empty() || shards.size() > 1024 ||
        (aesKey.size() != 16 && aesKey.size() != 24 && aesKey.size() != 32))
        return false;
    // The group may include parity shards; here all supplied shards must be
    // the ORIGINAL data chunks in order. Missing originals are dropped rather
    // than showing corrupted frames or claiming FEC was performed.
    output.reserve(std::min(shards.size() * size_t(1408), size_t(2u << 20)));
    for (size_t i = 0; i < shards.size(); ++i) {
        const auto& chunk = shards[i];
        if (chunk.size() != 1410) { output.clear(); return false; }
        const uint16_t used = readLe16(chunk.data());
        if (used == 0 || used > 1408) { output.clear(); return false; }
        std::vector<uint8_t> clear;
        if (!decodeChunk(chunk, aesKey, groupId, static_cast<uint16_t>(i), clear)) {
            output.clear();
            return false;
        }
        if (i == 0 && (clear.size() < 4 || clear[0] || clear[1] || clear[2])) {
            output.clear();
            return false;
        }
        output.insert(output.end(), clear.begin(), clear.begin() + used);
    }
    return !output.empty();
}

bool NativeUdp::setKeyHex(const std::string& hex) {
    if (hex.size() != 32 && hex.size() != 48 && hex.size() != 64) return false;
    std::vector<uint8_t> next;
    next.reserve(hex.size() / 2);
    for (size_t p = 0; p < hex.size(); p += 2) {
        const int hi = hexNibble(hex[p]), lo = hexNibble(hex[p + 1]);
        if (hi < 0 || lo < 0) return false;
        next.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    std::lock_guard<std::mutex> lock(keyMutex_);
    std::fill(key_.begin(), key_.end(), 0);
    key_ = std::move(next);
    return true;
}

bool NativeUdp::start(const std::string& host, int videoPort, int audioPort,
                      std::string& error) {
    error.clear();
    if (running_) return true;
    stop_ = false;
    if (!openUdpChannel(host, videoPort, videoSocket_, error))
        return false;
    if (!openUdpChannel(host, audioPort, audioSocket_, error)) {
        net::closeSocket(videoSocket_);
        return false;
    }
    running_ = true;
    if (!platform::startThread(videoThread_, [this] { videoLoop(); }, 8u << 20)) {
        error = "Não foi possível iniciar a recepção de vídeo UDP.";
        stop();
        return false;
    }
    if (!platform::startThread(audioThread_, [this] { audioLoop(); }, 8u << 20)) {
        error = "Não foi possível iniciar a recepção de áudio UDP.";
        stop();
        return false;
    }
    return true;
}

void NativeUdp::stop() {
    stop_ = true;
    if (videoSocket_.valid()) {
        net::shutdownSocket(videoSocket_);
        net::closeSocket(videoSocket_);
    }
    if (audioSocket_.valid()) {
        net::shutdownSocket(audioSocket_);
        net::closeSocket(audioSocket_);
    }
    if (videoThread_.joinable()) videoThread_.join();
    if (audioThread_.joinable()) audioThread_.join();
    running_ = false;
    {
        std::lock_guard<std::mutex> lock(groupMutex_);
        groups_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(keyMutex_);
        std::fill(key_.begin(), key_.end(), 0);
        key_.clear();
    }
}

void NativeUdp::handleVideoPacket(const uint8_t* data, size_t len) {
    if (len != 1420) return;
    ++videoPackets_;
    const uint16_t groupId = readLe16(data + 2);
    const uint16_t index = readLe16(data + 4);
    const uint16_t dataCount = readLe16(data + 6);
    const uint16_t totalCount = readLe16(data + 8);
    if (!dataCount || totalCount < dataCount || totalCount > 1024 ||
        index >= totalCount) return;

    std::vector<std::vector<uint8_t>> complete;
    {
        std::lock_guard<std::mutex> lock(groupMutex_);
        if (groups_.size() >= 8 && groups_.find(groupId) == groups_.end())
            groups_.erase(groups_.begin());
        auto& group = groups_[groupId];
        if (group.dataCount != dataCount || group.totalCount != totalCount) {
            group = {};
            group.id = groupId;
            group.dataCount = dataCount;
            group.totalCount = totalCount;
            group.shards.resize(totalCount);
            group.received.assign(totalCount, false);
        }
        if (!group.received[index]) {
            group.shards[index].assign(data + 10, data + len);
            group.received[index] = true;
            ++group.receivedCount;
        }
        if (group.receivedCount < dataCount) return;
        // Only the complete original data packets are decoded. If an
        // original is missing, wait for later arrivals or discard this group.
        for (size_t i = 0; i < dataCount; ++i)
            if (!group.received[i]) return;
        complete.assign(group.shards.begin(), group.shards.begin() + dataCount);
        groups_.erase(groupId);
    }
    std::vector<uint8_t> key;
    {
        std::lock_guard<std::mutex> lock(keyMutex_);
        key = key_;
    }
    if (key.empty()) return;
    std::vector<uint8_t> annexB;
    if (decodeNativeVideoGroup(complete, key, groupId, annexB))
        deliverH264(sink_, annexB.data(), annexB.size(), groupId);
    std::fill(key.begin(), key.end(), 0);
}

void NativeUdp::videoLoop() {
    platform::setThreadAffinity(platform::ThreadRole::Network, "boosteroid_udp_video");
    const net::Socket sock = videoSocket_;
    std::array<uint8_t, 2048> packet{};
    uint64_t nextKeepalive = platform::nowMs() + 10000;
    while (!stop_) {
        const ssize_t n = net::recvSocket(sock, packet.data(), packet.size());
        if (n > 0) handleVideoPacket(packet.data(), static_cast<size_t>(n));
        else if (n < 0 && !net::wouldBlock() && !net::wasInterrupted()) break;
        if (platform::nowMs() >= nextKeepalive) {
            static constexpr char ping[] = "{\"type\":\"keepalive\"}";
            net::sendNoSignal(sock, ping, sizeof(ping) - 1);
            nextKeepalive = platform::nowMs() + 10000;
        }
    }
}

void NativeUdp::audioLoop() {
    platform::setThreadAffinity(platform::ThreadRole::Network, "boosteroid_udp_audio");
    const net::Socket sock = audioSocket_;
    std::array<uint8_t, 4096> packet{};
    uint64_t nextKeepalive = platform::nowMs() + 10000;
    while (!stop_) {
        const ssize_t n = net::recvSocket(sock, packet.data(), packet.size());
        if (n > 0) {
            const uint8_t* opus = nullptr;
            size_t len = 0;
            uint32_t timestamp = 0;
            if (unpackOpusRtp(packet.data(), static_cast<size_t>(n),
                              opus, len, timestamp))
                deliverOpus(sink_, opus, len, timestamp);
        } else if (n < 0 && !net::wouldBlock() && !net::wasInterrupted()) break;
        if (platform::nowMs() >= nextKeepalive) {
            static constexpr char ping[] = "{\"type\":\"keepalive\"}";
            net::sendNoSignal(sock, ping, sizeof(ping) - 1);
            nextKeepalive = platform::nowMs() + 10000;
        }
    }
}

} // namespace xc::boosteroid
