// SPDX-License-Identifier: GPL-3.0-or-later
// Isolated loopback HTTP client for the locally launched OpenStroid helper.
// Deliberately does not change net::perform() or Xbox network behavior.
#include "app/boosteroid_control.h"
#include "net/socket.h"
#include "platform/platform.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace xc::app::boosteroid_control {
std::string newKey() {
    std::array<unsigned char, 32> random{};
    if (!platform::randomBytes(random.data(), random.size())) return {};
    constexpr char hex[] = "0123456789abcdef";
    std::string key;
    key.reserve(random.size() * 2);
    for (unsigned char byte : random) {
        key += hex[byte >> 4];
        key += hex[byte & 15];
    }
    return key;
}

Result request(const std::string& key, const char* method, const char* path,
               const std::string& jsonBody, int timeoutMs) {
    Result result;
    if (key.size() != 64 || !method || !path || path[0] != '/' ||
        std::string(path).find('\n') != std::string::npos ||
        std::string(path).find('\r') != std::string::npos) {
        result.error = "Invalid local request";
        return result;
    }
    std::string why;
    net::Socket socket = net::connectTcpSocket("127.0.0.1", 3037,
                                              std::max(100, timeoutMs), why);
    if (!socket) {
        result.error = "Boosteroid helper unavailable";
        return result;
    }
    struct Guard { net::Socket& socket; ~Guard() { net::closeSocket(socket); } } guard{socket};
    net::setRecvTimeout(socket, timeoutMs);
    net::setSendTimeout(socket, timeoutMs);
    std::string head = std::string(method) + " " + path + " HTTP/1.1\r\n";
    head += "Host: 127.0.0.1:3037\r\nConnection: close\r\n";
    head += "X-NimboPlay-Bridge: " + key + "\r\nAccept: application/json\r\n";
    if (std::string(method) == "POST") {
        head += "Content-Type: application/json\r\nContent-Length: ";
        head += std::to_string(jsonBody.size()) + "\r\n";
    }
    head += "\r\n";
    const std::string payload = head + jsonBody;
    for (size_t offset = 0; offset < payload.size();) {
        ssize_t n = net::sendNoSignal(socket, payload.data() + offset, payload.size() - offset);
        if (n <= 0) { result.error = "Local request send failed"; return result; }
        offset += static_cast<size_t>(n);
    }

    constexpr size_t kLimit = 8 * 1024 * 1024;
    std::string bytes;
    size_t separator = std::string::npos;
    size_t expected = std::string::npos;
    while (bytes.size() < kLimit) {
        std::array<char, 8192> buf{};
        ssize_t n = net::recvSocket(socket, buf.data(), buf.size());
        if (n <= 0) break;
        bytes.append(buf.data(), static_cast<size_t>(n));
        if (separator == std::string::npos) {
            separator = bytes.find("\r\n\r\n");
            if (separator != std::string::npos) {
                const auto firstEnd = bytes.find("\r\n");
                if (firstEnd == std::string::npos) break;
                int code = 0;
                if (std::sscanf(bytes.substr(0, firstEnd).c_str(), "HTTP/%*s %d", &code) != 1) {
                    result.error = "Invalid local HTTP reply";
                    return result;
                }
                result.status = code;
                const std::string header = bytes.substr(firstEnd + 2, separator - firstEnd - 2);
                size_t p = 0;
                while (p < header.size()) {
                    const size_t e = header.find("\r\n", p);
                    std::string line = header.substr(p, e == std::string::npos ? e : e - p);
                    std::string low = line;
                    std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (low.rfind("content-length:", 0) == 0) {
                        const std::string number = low.substr(15);
                        const unsigned long long len = std::strtoull(number.c_str(), nullptr, 10);
                        if (len > kLimit) { result.error = "Local reply too large"; return result; }
                        expected = static_cast<size_t>(len);
                    }
                    if (e == std::string::npos) break;
                    p = e + 2;
                }
            }
        }
        if (separator != std::string::npos && expected != std::string::npos &&
            bytes.size() >= separator + 4 + expected) break;
    }
    if (separator == std::string::npos) {
        result.error = "No HTTP reply from Boosteroid";
        return result;
    }
    const std::string body = bytes.substr(separator + 4,
        expected == std::string::npos ? std::string::npos : expected);
    std::string jsonError;
    auto value = json::parse(body, &jsonError);
    if (!value || !value->isObject()) {
        result.error = "Malformed Boosteroid response";
        result.status = 0;
        return result;
    }
    result.data = std::move(*value);
    return result;
}
} // namespace xc::app::boosteroid_control
