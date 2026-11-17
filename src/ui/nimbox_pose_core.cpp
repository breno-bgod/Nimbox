// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/nimbox_pose_core.h"

extern "C" float nimbox_pose_value(int offset, int component) {
    const auto p = xc::ui::nimbox::previewPose(offset);
    switch (component) {
        case 0: return p.x;
        case 1: return p.y;
        case 2: return p.z;
        case 3: return p.rotationY;
        case 4: return p.scale;
        case 5: return p.opacity;
        case 6: return static_cast<float>(p.zIndex);
        default: return 0;
    }
}

extern "C" int nimbox_relative_offset(int item, int selected, int count) {
    return xc::ui::nimbox::previewRelativeOffset(item, selected, count);
}
