// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid native gamepad wire messages (Android TV control WSS).
// Mapped from NimboPlay's existing cross-platform DualSense/Android input.
#pragma once

#include "input/controller.h"
#include "util/json.h"

#include <vector>

namespace xc::boosteroid {

// A separate provider adapter, not PSBox/GSSV gamepad frames.
class ControllerMapper {
public:
    void reset() { ready_ = false; }
    std::vector<json::Value> encode(const input::ControllerState& state, int controllerId);
private:
    bool ready_ = false;
    input::ControllerState previous_;
};

} // namespace xc::boosteroid
