// SPDX-License-Identifier: GPL-3.0-or-later
// Android-only compositor for Nimbox's EXISTING transparent game menu/touch HUD.
// The original SDL/Vulkan renderers remain unchanged. This view mirrors the
// already-rendered UI overlay above the MediaCodec SurfaceView.
package com.rafaelngp.psbox.surface;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;
import java.nio.ByteBuffer;

public final class AndroidSurfaceOverlayView extends View {
    private static final float LOGICAL_WIDTH = 1920f;
    private static final float LOGICAL_HEIGHT = 1080f;
    private final Object frameLock = new Object();
    private final Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG);
    private Bitmap bitmap;
    private int posX, posY;
    private int pixelWidth, pixelHeight;
    private int opacity;

    public AndroidSurfaceOverlayView(Context context) {
        super(context);
        setWillNotDraw(false);
        setBackgroundColor(Color.TRANSPARENT);
        setClickable(false); // SDL beneath remains responsible for touch and controller input.
        setFocusable(false);
    }

    // Called on native SDL thread. Copy bytes before JNI's temporary source expires.
    public void setFrame(ByteBuffer rgba, int x, int y, int width, int height, int alpha) {
        if (width <= 0 || height <= 0 || rgba == null ||
                width > 4096 || height > 2160 ||
                rgba.capacity() < ((long) width * height * 4)) {
            clearFrame();
            return;
        }
        synchronized (frameLock) {
            if (bitmap == null || bitmap.getWidth() != width || bitmap.getHeight() != height) {
                // The Canvas uint32 format 0xAABBGGRR is RGBA_8888 in little-endian memory.
                bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            }
            ByteBuffer copy = rgba.duplicate();
            copy.position(0);
            bitmap.copyPixelsFromBuffer(copy);
            posX = x;
            posY = y;
            pixelWidth = width;
            pixelHeight = height;
            opacity = Math.max(0, Math.min(255, alpha));
        }
        postInvalidate();
    }

    public void clearFrame() {
        synchronized (frameLock) {
            bitmap = null;
            pixelWidth = pixelHeight = 0;
        }
        postInvalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        synchronized (frameLock) {
            if (bitmap == null || pixelWidth <= 0 || pixelHeight <= 0) return;
            float scale = Math.min(getWidth() / LOGICAL_WIDTH, getHeight() / LOGICAL_HEIGHT);
            float x0 = (getWidth() - LOGICAL_WIDTH * scale) / 2f;
            float y0 = (getHeight() - LOGICAL_HEIGHT * scale) / 2f;
            RectF dst = new RectF(x0 + posX * scale, y0 + posY * scale,
                    x0 + (posX + pixelWidth) * scale, y0 + (posY + pixelHeight) * scale);
            paint.setAlpha(opacity);
            canvas.drawBitmap(bitmap, null, dst, paint);
        }
    }
}
