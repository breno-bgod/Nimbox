package com.rafaelngp.psbox;

import android.Manifest;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.pm.ApplicationInfo;
import android.graphics.Color;
import android.os.Handler;
import android.view.Gravity;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;
import android.graphics.PixelFormat;
import android.os.Looper;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.view.ViewGroup;
import java.nio.ByteBuffer;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import com.rafaelngp.psbox.surface.AndroidSurfaceEngine;
import com.rafaelngp.psbox.surface.AndroidSurfaceOverlayView;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.util.Log;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends SDLActivity {
    private static final String TAG = "Nimbox";

    /**
     * Recommend an AVC decoder to the C++ MediaCodec pipeline.
     * Prefer actual hardware codecs with advertised low-latency support.
     * The native backend still validates configuration and falls back to
     * AMediaCodec_createDecoderByType or FFmpeg when unavailable.
     *
     * This runs once per decoder initialization, never in the frame loop.
     */
    public static String preferredAvcDecoder() {
        final String mime = "video/avc";
        String selected = "";
        int bestScore = 0;

        try {
            for (MediaCodecInfo info :
                    new MediaCodecList(MediaCodecList.ALL_CODECS).getCodecInfos()) {
                if (info.isEncoder()) continue;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && info.isAlias()) continue;

                boolean supportsAvc = false;
                for (String type : info.getSupportedTypes()) {
                    if (mime.equalsIgnoreCase(type)) {
                        supportsAvc = true;
                        break;
                    }
                }
                if (!supportsAvc) continue;

                // Before API 29 there is no public hardware-acceleration flag.
                // Exclude the known platform software families as a fallback.
                boolean hardware;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    hardware = info.isHardwareAccelerated();
                } else {
                    String name = info.getName().toLowerCase(java.util.Locale.ROOT);
                    hardware = !(name.startsWith("omx.google.")
                            || name.startsWith("omx.ffmpeg.")
                            || name.startsWith("c2.android.")
                            || name.startsWith("avcdecoder"));
                }
                if (!hardware) continue;

                boolean lowLatency = false;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    try {
                        MediaCodecInfo.CodecCapabilities caps = info.getCapabilitiesForType(mime);
                        lowLatency = caps != null && caps.isFeatureSupported(
                                MediaCodecInfo.CodecCapabilities.FEATURE_LowLatency);
                    } catch (RuntimeException ignored) {
                        // A few vendor implementations fail capabilities queries.
                    }
                }
                int score = lowLatency ? 2 : 1;
                if (score > bestScore) {
                    bestScore = score;
                    selected = info.getName();
                    if (lowLatency) break;
                }
            }
        } catch (RuntimeException e) {
            Log.w(TAG, "AVC decoder enumeration unavailable, using platform default", e);
            return "";
        }

        if (selected.isEmpty()) {
            Log.i(TAG, "AVC decoder selector: no explicitly identified hardware codec; default fallback");
        } else {
            Log.i(TAG, "AVC decoder selector: " + selected
                    + " (lowLatencyAdvertised=" + (bestScore == 2) + ")");
        }
        return selected;
    }

    private static final int PERMISSION_REQUEST_BT = 1001;
    private static final int PERMISSION_REQUEST_AUDIO = 1002;

    // Only the debuggable NimboPlay Alpha APK exposes this floating diagnostic.
    // It never changes the SDL interface, media pipeline or console UI.
    private static native boolean nativeRequestDeviceDiagnosis(String title, boolean probeSession);
    private static native String nativeDeviceDiagnosisReport();
    private Button mDeviceDiagnosticButton;
    private AlertDialog mDeviceDiagnosticDialog;
    private final Handler mDeviceDiagnosticHandler = new Handler(Looper.getMainLooper());
    private Runnable mDeviceDiagnosticPoll;

    private int dp(int value) {
        return Math.round(getResources().getDisplayMetrics().density * value);
    }

    private void installExperimentalDiagnostics() {
        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) == 0 || mLayout == null) return;
        mDeviceDiagnosticButton = new Button(this);
        mDeviceDiagnosticButton.setText("NX Teste");
        mDeviceDiagnosticButton.setTextSize(11);
        mDeviceDiagnosticButton.setAllCaps(false);
        mDeviceDiagnosticButton.setTextColor(Color.WHITE);
        mDeviceDiagnosticButton.setBackgroundTintList(
                android.content.res.ColorStateList.valueOf(Color.rgb(24, 91, 162)));
        mDeviceDiagnosticButton.setMinHeight(0);
        mDeviceDiagnosticButton.setMinimumHeight(0);
        mDeviceDiagnosticButton.setMinWidth(0);
        mDeviceDiagnosticButton.setMinimumWidth(0);
        mDeviceDiagnosticButton.setPadding(dp(12), dp(3), dp(12), dp(3));
        mDeviceDiagnosticButton.setAlpha(0.93f);
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, dp(42));
        params.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        params.addRule(RelativeLayout.ALIGN_PARENT_END);
        params.setMargins(0, dp(48), dp(20), 0);
        mLayout.addView(mDeviceDiagnosticButton, params);
        mDeviceDiagnosticButton.setOnClickListener(v -> chooseDiagnosticTitle());
        updateDeviceDiagnosticButton();
    }

    private void updateDeviceDiagnosticButton() {
        runOnUiThread(() -> {
            if (mDeviceDiagnosticButton != null) {
                mDeviceDiagnosticButton.setVisibility(
                        (!sStreamingActive && sIsForeground) ? View.VISIBLE : View.GONE);
            }
        });
    }

    private void chooseDiagnosticTitle() {
        if (sStreamingActive) return;
        final String[] titles = {
                "Killer Instinct 2 Classic",
                "Grand Theft Auto VI",
                "Fortnite"
        };
        new AlertDialog.Builder(this)
                .setTitle("NX Diagnóstico xCloud - Experimental")
                .setMessage("Usa sua conta Xbox já conectada ao NimboPlay. "
                        + "Nenhuma alteração de identidade ou desbloqueio é realizada. "
                        + "Selecione o jogo:")
                .setItems(titles, (dialog, which) -> chooseDiagnosticMode(titles[which]))
                .setNegativeButton("Cancelar", null)
                .show();
    }

    private void chooseDiagnosticMode(String title) {
        new AlertDialog.Builder(this)
                .setTitle(title)
                .setItems(new String[] {
                        "Consultar catálogo e dispositivos (sem iniciar sessão)",
                        "Testar solicitação de sessão (com confirmação)"
                }, (dialog, which) -> {
                    if (which == 0) {
                        beginDeviceDiagnosis(title, false);
                    } else {
                        new AlertDialog.Builder(this)
                                .setTitle("Confirmar teste de sessão")
                                .setMessage("O serviço Xbox receberá uma solicitação real para "
                                        + title + ". A sessão será encerrada em seguida, "
                                        + "sem vídeo, áudio ou controles. "
                                        + "Somente se sua conta tiver direito de acesso.")
                                .setPositiveButton("Confirmar", (d, w) ->
                                        beginDeviceDiagnosis(title, true))
                                .setNegativeButton("Cancelar", null)
                                .show();
                    }
                })
                .setNegativeButton("Voltar", null)
                .show();
    }

    private void beginDeviceDiagnosis(String title, boolean probeSession) {
        if (sStreamingActive) return;
        boolean queued;
        try {
            queued = nativeRequestDeviceDiagnosis(title, probeSession);
        } catch (UnsatisfiedLinkError e) {
            new AlertDialog.Builder(this)
                    .setMessage("O motor nativo ainda está inicializando. "
                            + "Abra o diagnóstico novamente após o login.")
                    .setPositiveButton("OK", null).show();
            return;
        }
        if (!queued) {
            new AlertDialog.Builder(this)
                    .setTitle("Diagnóstico ocupado")
                    .setMessage("O NimboPlay ainda está entrando na conta, "
                            + "iniciando um jogo ou processando outro comando. "
                            + "Tente novamente no menu inicial.")
                    .setPositiveButton("OK", null).show();
            return;
        }
        final TextView report = new TextView(this);
        report.setTextSize(14);
        report.setTextColor(Color.WHITE);
        report.setPadding(dp(16), dp(16), dp(16), dp(16));
        report.setText("Consultando o serviço xCloud... \n"
                + "Esta operação usa o login já existente e não inicia o streaming.");
        report.setTextIsSelectable(true);
        ScrollView scroll = new ScrollView(this);
        scroll.addView(report);
        mDeviceDiagnosticDialog = new AlertDialog.Builder(this)
                .setTitle("NX Diagnóstico: " + title)
                .setView(scroll)
                .setPositiveButton("Fechar", null)
                .setNeutralButton("Copiar resultado", (dialog, which) -> {
                    ClipboardManager clipboard =
                            (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
                    if (clipboard != null) clipboard.setPrimaryClip(
                            ClipData.newPlainText("NX Diagnóstico xCloud", report.getText()));
                })
                .create();
        mDeviceDiagnosticDialog.setOnDismissListener(d -> {
            if (mDeviceDiagnosticPoll != null)
                mDeviceDiagnosticHandler.removeCallbacks(mDeviceDiagnosticPoll);
            mDeviceDiagnosticPoll = null;
            mDeviceDiagnosticDialog = null;
        });
        mDeviceDiagnosticDialog.show();
        mDeviceDiagnosticPoll = new Runnable() {
            @Override public void run() {
                if (mDeviceDiagnosticDialog == null ||
                        !mDeviceDiagnosticDialog.isShowing()) return;
                String result;
                try {
                    result = nativeDeviceDiagnosisReport();
                } catch (UnsatisfiedLinkError e) {
                    report.setText("O diagnóstico C++ não está disponível neste APK.");
                    return;
                }
                if (result != null && !"NX_DIAG_RUNNING".equals(result)) {
                    report.setText(result);
                    return;
                }
                mDeviceDiagnosticHandler.postDelayed(this, 500);
            }
        };
        mDeviceDiagnosticHandler.post(mDeviceDiagnosticPoll);
    }

    private static volatile boolean sStreamingActive = false;
    private static volatile boolean sIsForeground = false;
    private static WifiManager.WifiLock sWifiLock = null;

    // Experimental Android-only third playback engine. No change to CPU/GPU paths.
    private static volatile MainActivity sSurfaceActivity;
    private static SurfaceView sSurfaceView;
    private static AndroidSurfaceOverlayView sSurfaceOverlay;
    private static volatile AndroidSurfaceEngine sSurfaceEngine;

    // Match SDL_LOGICAL_PRESENTATION_LETTERBOX (1920x1080) on wide Android displays.
    private static void updateSurfaceBounds(MainActivity activity) {
        if (activity.mLayout == null || sSurfaceView == null) return;
        int parentW = activity.mLayout.getWidth();
        int parentH = activity.mLayout.getHeight();
        if (parentW <= 0 || parentH <= 0) return;
        float scale = Math.min(parentW / 1920f, parentH / 1080f);
        int videoW = Math.max(1, Math.round(1920f * scale));
        int videoH = Math.max(1, Math.round(1080f * scale));
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(videoW, videoH);
        params.addRule(RelativeLayout.CENTER_IN_PARENT);
        sSurfaceView.setLayoutParams(params);
    }

    private static void hideSurfaceVideoLayer() {
        MainActivity activity = sSurfaceActivity;
        if (activity != null) activity.runOnUiThread(() -> {
            if (sSurfaceView != null) sSurfaceView.setVisibility(View.GONE);
            if (sSurfaceOverlay != null) sSurfaceOverlay.setVisibility(View.GONE);
        });
    }

    public static boolean surfaceVideoStart(int width, int height) {
        MainActivity activity = sSurfaceActivity;
        if (activity == null || activity.mLayout == null) return false;
        final CountDownLatch created = new CountDownLatch(1);
        Runnable create = () -> {
            if (sSurfaceView == null) {
                sSurfaceView = new SurfaceView(activity);
                sSurfaceView.setZOrderMediaOverlay(true);
                sSurfaceView.getHolder().setFormat(PixelFormat.OPAQUE);
                sSurfaceView.setClickable(false);
                sSurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
                    @Override public void surfaceCreated(SurfaceHolder holder) { }
                    @Override public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) { }
                    @Override public void surfaceDestroyed(SurfaceHolder holder) {
                        // The next compressed AU tells native playback to fall back safely.
                        // An intentionally hidden layer is not a device surface failure.
                        if (sSurfaceView != null && sSurfaceView.getVisibility() == View.VISIBLE) {
                            surfaceVideoStop();
                        }
                    }
                });
                activity.mLayout.addView(sSurfaceView, new RelativeLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                activity.mLayout.addOnLayoutChangeListener((view, left, top, right, bottom,
                                                            oldLeft, oldTop, oldRight, oldBottom) -> {
                    if ((right - left) != (oldRight - oldLeft) ||
                            (bottom - top) != (oldBottom - oldTop)) {
                        updateSurfaceBounds(activity);
                    }
                });
            }
            updateSurfaceBounds(activity);
            if (sSurfaceOverlay == null) {
                sSurfaceOverlay = new AndroidSurfaceOverlayView(activity);
                activity.mLayout.addView(sSurfaceOverlay, new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            }
            sSurfaceOverlay.setVisibility(View.VISIBLE);
            sSurfaceView.setVisibility(View.VISIBLE);
            created.countDown();
        };
        if (Looper.myLooper() == Looper.getMainLooper()) create.run();
        else activity.runOnUiThread(create);
        try {
            if (!created.await(2, TimeUnit.SECONDS)) {
                hideSurfaceVideoLayer();
                return false;
            }
            Surface surface = null;
            for (int attempt = 0; attempt < 100; attempt++) {
                if (sSurfaceView != null) {
                    surface = sSurfaceView.getHolder().getSurface();
                    if (surface != null && surface.isValid()) break;
                }
                Thread.sleep(20);
            }
            if (surface == null || !surface.isValid()) {
                hideSurfaceVideoLayer();
                return false;
            }
            AndroidSurfaceEngine engine = new AndroidSurfaceEngine();
            if (!engine.start(surface, width, height)) {
                hideSurfaceVideoLayer();
                return false;
            }
            sSurfaceEngine = engine;
            return true;
        } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
            hideSurfaceVideoLayer();
            return false;
        }
    }

    public static int surfaceVideoQueue(ByteBuffer frame, long ptsUs, boolean keyframe) {
        AndroidSurfaceEngine engine = sSurfaceEngine;
        return engine != null ? engine.queueVideoFrame(frame, ptsUs, keyframe)
                : AndroidSurfaceEngine.ERROR;
    }

    public static long[] surfaceVideoStats() {
        AndroidSurfaceEngine engine = sSurfaceEngine;
        return engine != null ? engine.stats() : new long[] {0, 0, 0, 0, 0};
    }

    public static void surfaceVideoOverlay(ByteBuffer pixels, int x, int y,
                                           int width, int height, int opacity) {
        if (sSurfaceEngine == null) return;
        AndroidSurfaceOverlayView overlay = sSurfaceOverlay;
        if (overlay == null) return;
        if (pixels == null || width <= 0 || height <= 0) overlay.clearFrame();
        else overlay.setFrame(pixels, x, y, width, height, opacity);
    }

    public static void surfaceVideoStop() {
        AndroidSurfaceEngine engine = sSurfaceEngine;
        sSurfaceEngine = null;
        if (engine != null) engine.stop();
        MainActivity activity = sSurfaceActivity;
        if (activity != null) activity.runOnUiThread(() -> {
            if (sSurfaceOverlay != null) {
                sSurfaceOverlay.clearFrame();
                sSurfaceOverlay.setVisibility(View.GONE);
            }
            if (sSurfaceView != null) {
                sSurfaceView.setVisibility(View.GONE);
            }
        });
    }


    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }

    @Override
    protected String[] getArguments() {
        String selftest = getIntent().getStringExtra("selftest");
        if (selftest != null && !selftest.isEmpty()) {
            return new String[] { "--selftest", selftest };
        }
        return new String[0];
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sSurfaceActivity = this;
        if (getWindow() != null) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
        configureEdgeToEdgeAndCutout();
        setImmersiveFullscreen();
        disableDefaultFocusHighlight();
        setDisplayRefreshRate(60.0f);
        checkAndRequestStartupPermissions();
        installExperimentalDiagnostics();
    }

    @Override
    protected void onResume() {
        super.onResume();
        sIsForeground = true;
        updateWifiLock();
        updateDeviceDiagnosticButton();
        setImmersiveFullscreen();
        disableDefaultFocusHighlight();
        setDisplayRefreshRate(60.0f);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            setImmersiveFullscreen();
            disableDefaultFocusHighlight();
        }
    }

    @Override
    protected void onPause() {
        sIsForeground = false;
        updateDeviceDiagnosticButton();
        surfaceVideoStop(); // Release additional decoder when the Activity pauses.
        updateWifiLock();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if (mDeviceDiagnosticPoll != null)
            mDeviceDiagnosticHandler.removeCallbacks(mDeviceDiagnosticPoll);
        if (mDeviceDiagnosticDialog != null) mDeviceDiagnosticDialog.dismiss();
        if (mLayout != null && mDeviceDiagnosticButton != null) mLayout.removeView(mDeviceDiagnosticButton);
        mDeviceDiagnosticButton = null;
        surfaceVideoStop();
        if (mLayout != null) {
            if (sSurfaceOverlay != null) mLayout.removeView(sSurfaceOverlay);
            if (sSurfaceView != null) mLayout.removeView(sSurfaceView);
        }
        sSurfaceOverlay = null;
        sSurfaceView = null;
        sSurfaceActivity = null;
        sStreamingActive = false;
        sIsForeground = false;
        updateWifiLock();
        super.onDestroy();
    }

    // --- 1. Edge-to-Edge & Cutout Inset Configuration ---
    private void configureEdgeToEdgeAndCutout() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            Window window = getWindow();
            if (window != null) {
                WindowManager.LayoutParams lp = window.getAttributes();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
                } else {
                    lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
                }
                window.setAttributes(lp);
            }
        }
    }

    private void setImmersiveFullscreen() {
        Window window = getWindow();
        if (window == null) return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            View decorView = window.getDecorView();
            if (decorView != null) {
                int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                          | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                          | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                          | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                          | View.SYSTEM_UI_FLAG_FULLSCREEN
                          | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
                decorView.setSystemUiVisibility(flags);
            }
        }
    }

    private void disableDefaultFocusHighlight() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            try {
                Window window = getWindow();
                if (window != null && window.getDecorView() != null) {
                    window.getDecorView().setDefaultFocusHighlightEnabled(false);
                }
                if (mSurface != null) {
                    mSurface.setDefaultFocusHighlightEnabled(false);
                }
                if (mLayout != null) {
                    mLayout.setDefaultFocusHighlightEnabled(false);
                }
            } catch (Exception e) {
                Log.w(TAG, "disableDefaultFocusHighlight error: " + e.getMessage());
            }
        }
    }

    public static int[] getDisplayCutoutInsets() {
        try {
            Activity activity = (Activity) getContext();
            if (activity == null) return new int[]{0, 0, 0, 0};
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                Window window = activity.getWindow();
                if (window != null && window.getDecorView() != null) {
                    WindowInsets insets = window.getDecorView().getRootWindowInsets();
                    if (insets != null) {
                        DisplayCutout cutout = insets.getDisplayCutout();
                        if (cutout != null) {
                            return new int[]{
                                cutout.getSafeInsetLeft(),
                                cutout.getSafeInsetTop(),
                                cutout.getSafeInsetRight(),
                                cutout.getSafeInsetBottom()
                            };
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "getDisplayCutoutInsets error: " + e.getMessage());
        }
        return new int[]{0, 0, 0, 0};
    }

    // --- 2. Low-Latency Wi-Fi Lock with Lifecycle Awareness ---
    public static synchronized void setLowLatencyWifi(boolean enable) {
        sStreamingActive = enable;
        updateWifiLock();
        MainActivity activity = sSurfaceActivity;
        if (activity != null) activity.updateDeviceDiagnosticButton();
    }

    private static synchronized void updateWifiLock() {
        try {
            Context ctx = getContext();
            if (ctx == null) return;
            WifiManager wm = (WifiManager) ctx.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (wm == null) return;

            if (sStreamingActive && sIsForeground) {
                if (sWifiLock == null) {
                    int mode = WifiManager.WIFI_MODE_FULL_HIGH_PERF;
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        mode = WifiManager.WIFI_MODE_FULL_LOW_LATENCY;
                    }
                    try {
                        sWifiLock = wm.createWifiLock(mode, "Nimbox:StreamingLock");
                    } catch (Exception ex) {
                        Log.w(TAG, "Failed creating low-latency wifi lock, falling back to high-perf: " + ex.getMessage());
                        sWifiLock = wm.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "Nimbox:StreamingLock");
                    }
                    if (sWifiLock != null) {
                        sWifiLock.setReferenceCounted(false);
                    }
                }
                if (sWifiLock != null && !sWifiLock.isHeld()) {
                    sWifiLock.acquire();
                    Log.i(TAG, "Wi-Fi low-latency lock acquired");
                }
            } else {
                if (sWifiLock != null && sWifiLock.isHeld()) {
                    sWifiLock.release();
                    Log.i(TAG, "Wi-Fi low-latency lock released");
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "updateWifiLock error: " + e.getMessage());
        }
    }

    // --- 3. High Refresh Rate (120Hz / 60Hz Locking) ---
    public static boolean is120HzSupported() {
        try {
            Activity activity = (Activity) getContext();
            if (activity == null) return false;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                Display display = activity.getWindowManager().getDefaultDisplay();
                for (Display.Mode mode : display.getSupportedModes()) {
                    if (mode.getRefreshRate() >= 119.0f) {
                        return true;
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "is120HzSupported error: " + e.getMessage());
        }
        return false;
    }

    public static void setDisplayRefreshRate(float targetRate) {
        Activity activity = (Activity) getContext();
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            try {
                Window window = activity.getWindow();
                if (window == null) return;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    Display display = activity.getWindowManager().getDefaultDisplay();
                    Display.Mode[] modes = display.getSupportedModes();
                    Display.Mode bestMode = null;
                    for (Display.Mode mode : modes) {
                        if (Math.abs(mode.getRefreshRate() - targetRate) < 1.0f) {
                            if (bestMode == null ||
                                (mode.getPhysicalWidth() >= bestMode.getPhysicalWidth() &&
                                 mode.getPhysicalHeight() >= bestMode.getPhysicalHeight())) {
                                bestMode = mode;
                            }
                        }
                    }
                    WindowManager.LayoutParams lp = window.getAttributes();
                    int desiredModeId = (bestMode != null) ? bestMode.getModeId() : (targetRate <= 0.0f ? 0 : lp.preferredDisplayModeId);
                    if (lp.preferredDisplayModeId != desiredModeId) {
                        lp.preferredDisplayModeId = desiredModeId;
                        if (bestMode != null) {
                            Log.i(TAG, "Locking display mode to " + bestMode.getModeId() + " (" + bestMode.getRefreshRate() + " Hz)");
                        } else if (targetRate <= 0.0f) {
                            Log.i(TAG, "Resetting display mode to default");
                        }
                        window.setAttributes(lp);
                    }
                }
            } catch (Exception e) {
                Log.w(TAG, "setDisplayRefreshRate error: " + e.getMessage());
            }
        });
    }

    // --- 4. Runtime Permissions Flow ---
    private void checkAndRequestStartupPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            List<String> perms = new ArrayList<>();
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                perms.add(Manifest.permission.BLUETOOTH_CONNECT);
            }
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                perms.add(Manifest.permission.BLUETOOTH_SCAN);
            }
            if (!perms.isEmpty()) {
                Log.i(TAG, "Requesting Bluetooth permissions: " + perms);
                requestPermissions(perms.toArray(new String[0]), PERMISSION_REQUEST_BT);
            }
        }
    }

    public static boolean hasRecordAudioPermission() {
        Activity activity = (Activity) getContext();
        if (activity == null) return false;
        return activity.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED;
    }

    public static void requestRecordAudioPermission() {
        Activity activity = (Activity) getContext();
        if (activity == null) return;
        if (activity.checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            activity.requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, PERMISSION_REQUEST_AUDIO);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_BT) {
            for (int i = 0; i < permissions.length; i++) {
                Log.i(TAG, "Permission " + permissions[i] + ": " +
                      (grantResults.length > i && grantResults[i] == PackageManager.PERMISSION_GRANTED ? "GRANTED" : "DENIED"));
            }
        }
    }

    // --- 5. Device Vibration / Haptics ---
    private static Vibrator sVibrator = null;
    private static boolean sVibratorInitialized = false;
    private static boolean sHasVibrator = false;
    private static boolean sHasAmplitudeControl = false;

    private static VibrationEffect sEffectClick = null;
    private static VibrationEffect sEffectTick = null;
    private static VibrationEffect sEffectHeavyClick = null;

    private static synchronized Vibrator getDeviceVibrator(Context ctx) {
        if (sVibratorInitialized) return sVibrator;
        if (ctx == null) return null;
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                VibratorManager vm = (VibratorManager) ctx.getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
                if (vm != null) sVibrator = vm.getDefaultVibrator();
            }
            if (sVibrator == null) {
                sVibrator = (Vibrator) ctx.getSystemService(Context.VIBRATOR_SERVICE);
            }
            if (sVibrator != null) {
                sHasVibrator = sVibrator.hasVibrator();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    sHasAmplitudeControl = sVibrator.hasAmplitudeControl();
                }
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                sEffectClick = VibrationEffect.createPredefined(VibrationEffect.EFFECT_CLICK);
                sEffectTick = VibrationEffect.createPredefined(VibrationEffect.EFFECT_TICK);
                sEffectHeavyClick = VibrationEffect.createPredefined(VibrationEffect.EFFECT_HEAVY_CLICK);
            }
            sVibratorInitialized = true;
        } catch (Exception e) {
            Log.w(TAG, "vibrator init error: " + e.getMessage());
        }
        return sVibrator;
    }

    public static void vibrateDevice(int large, int small, int durationMs) {
        try {
            Activity activity = (Activity) getContext();
            Vibrator vib = getDeviceVibrator(activity);
            if (vib == null || !sHasVibrator) return;
            int amp = Math.max(large, (small * 3) / 4);
            if (amp <= 0 || durationMs <= 0) {
                vib.cancel();
                return;
            }
            int clampedAmp = Math.min(255, Math.max(1, amp));
            long dur = Math.min(5000L, Math.max(15L, (long) durationMs));
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                int actualAmp = sHasAmplitudeControl ? clampedAmp : VibrationEffect.DEFAULT_AMPLITUDE;
                vib.vibrate(VibrationEffect.createOneShot(dur, actualAmp));
            } else {
                vib.vibrate(dur);
            }
        } catch (Exception e) {
            // ignore
        }
    }

    public static void performHaptic(int type) {
        try {
            Activity activity = (Activity) getContext();
            if (activity == null) return;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                Vibrator vib = getDeviceVibrator(activity);
                if (vib != null && sHasVibrator) {
                    VibrationEffect effect = (type == 1) ? sEffectTick :
                                             (type == 2) ? sEffectHeavyClick :
                                                           sEffectClick;
                    if (effect != null) {
                        vib.vibrate(effect);
                        return;
                    }
                }
            }
            View decor = activity.getWindow().getDecorView();
            if (decor != null) {
                decor.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
            }
        } catch (Exception ignored) {
        }
    }
}
