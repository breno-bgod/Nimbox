// SPDX-License-Identifier: GPL-3.0-or-later
#include "boosteroid/control_input.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace xc::boosteroid {
namespace {
int axis(float value) {
    if (!std::isfinite(value)) return 0;
    return static_cast<int>(std::lround(std::clamp(value, -1.0f, 1.0f) * 32767.0f));
}

int trigger(float value) {
    if (!std::isfinite(value)) return -32767;
    return static_cast<int>(std::lround(std::clamp(value, 0.0f, 1.0f) * 65534.0f - 32767.0f));
}

int hat(const input::ControllerState& s) {
    const bool u = s.dpadUp, d = s.dpadDown, l = s.dpadLeft, r = s.dpadRight;
    if (u && l) return 9;
    if (u && r) return 3;
    if (d && l) return 12;
    if (d && r) return 6;
    if (u) return 1;
    if (r) return 2;
    if (d) return 4;
    if (l) return 8;
    return 0;
}

json::Value event(int controllerId, const char* action) {
    auto value = json::Value::object();
    value.set("type", "controller");
    value.set("action", action);
    value.set("id", controllerId);
    return value;
}
} // namespace

std::vector<json::Value> ControllerMapper::encode(const input::ControllerState& s,
                                                  int controllerId) {
    std::vector<json::Value> out;
    if (controllerId <= 0 || !s.connected) {
        ready_ = false;
        return out;
    }

    const std::array<bool, 13> buttons = {
        s.btnA, s.btnB, s.btnX, s.btnY, s.btnL1, s.btnR1, s.btnTouchpad,
        s.btnOptions, s.btnL3, s.btnR3, s.triggerL2 >= 0.5f,
        s.triggerR2 >= 0.5f, s.btnNexus
    };
    const std::array<bool, 13> previous = {
        previous_.btnA, previous_.btnB, previous_.btnX, previous_.btnY,
        previous_.btnL1, previous_.btnR1, previous_.btnTouchpad,
        previous_.btnOptions, previous_.btnL3, previous_.btnR3,
        previous_.triggerL2 >= 0.5f, previous_.triggerR2 >= 0.5f,
        previous_.btnNexus
    };
    for (size_t i = 0; i < buttons.size(); ++i) {
        if (ready_ && buttons[i] == previous[i]) continue;
        auto msg = event(controllerId, "button");
        msg.set("button", i == 12 ? 16 : static_cast<int>(i));
        msg.set("value", buttons[i] ? 1 : 0);
        out.push_back(std::move(msg));
    }

    const std::array<int, 6> currentAxes = {
        axis(s.leftStickX), axis(s.leftStickY), trigger(s.triggerL2),
        axis(s.rightStickX), axis(s.rightStickY), trigger(s.triggerR2)
    };
    const std::array<int, 6> oldAxes = {
        axis(previous_.leftStickX), axis(previous_.leftStickY), trigger(previous_.triggerL2),
        axis(previous_.rightStickX), axis(previous_.rightStickY), trigger(previous_.triggerR2)
    };
    for (size_t i = 0; i < currentAxes.size(); ++i) {
        if (ready_ && std::abs(currentAxes[i] - oldAxes[i]) < 480) continue;
        auto msg = event(controllerId, "axes");
        msg.set("axes", static_cast<int>(i));
        msg.set("value", currentAxes[i]);
        out.push_back(std::move(msg));
    }

    if (!ready_ || hat(s) != hat(previous_)) {
        auto msg = event(controllerId, "pad");
        msg.set("hat", hat(s));
        out.push_back(std::move(msg));
    }
    previous_ = s;
    ready_ = true;
    return out;
}

} // namespace xc::boosteroid
