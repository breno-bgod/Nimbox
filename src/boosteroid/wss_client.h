// SPDX-License-Identifier: Apache-2.0
// Based on ZERODROID by elzerdodata, Android TV Boosteroid native streaming.
// Adapted for NimboPlay PS5/Android POSIX networking and device entropy.
// Original: https://github.com/elzerdodata/ZERODROID
#pragma once

#include "net/socket.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace xc::boosteroid {

// RFC 6455 WebSocket over verified TLS, without libdatachannel's disabled
// WebSocket feature. Use from a dedicated worker thread, not the Horizon loop.
class WssClient {
public:
    WssClient();
    ~WssClient();
    WssClient(const WssClient&) = delete;
    WssClient& operator=(const WssClient&) = delete;
    bool connect(const std::string& hostAndPort, const std::string& path,
                 const std::string& caBundle, std::string& error);
    void close();
    bool connected() const { return connected_; }
    bool send_text(const std::string& text);
    bool send_ping();
    int last_ping_ms() const { return lastPingMs_.load(); }
    // false with empty error denotes a read timeout, not a fatal error.
    bool read_text(std::string& text, int timeoutMs, std::string& error);
private:
    bool send_frame_locked(uint8_t opcode, const void* data, std::size_t size);
    bool write_all_locked(const unsigned char* data, std::size_t size);
    bool read_exact_locked(unsigned char* data, std::size_t size, int timeoutMs,
                           bool& timedOut, std::string& error);
    void reset_locked();
    mutable std::mutex ioMutex_;
    net::Socket socket_;
    mbedtls_ssl_context ssl_;
    mbedtls_ssl_config config_;
    mbedtls_x509_crt ca_;
    mbedtls_ctr_drbg_context random_;
    mbedtls_entropy_context entropy_;
    bool initialized_ = false;
    bool connected_ = false;
    std::atomic<int> lastPingMs_{-1};
    std::vector<unsigned char> fragment_;
    uint8_t fragmentOpcode_ = 0;
};

} // namespace xc::boosteroid
