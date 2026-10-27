/*
 * This file falls under the GNU General Public License v3.0 license: See the LICENSE.txt in the root of this project for more info.
 * Summary:
 * Permissions of this strong copyleft license are conditioned on making available complete source code of licensed works and modifications, which include larger works using a licensed work, under the same license.
 * Copyright and license notices must be preserved. Contributors provide an express grant of patent rights. Modifications to the source code must be disclosed publicly.
 */

#pragma once

#include "igraphicsapi.h"
#include "displayxrGraphics.h"
#include "displayxrOverlayWindow.h"

#include <openxr/openxr.h>
#include <string>

// Weaving backend that hands the side-by-side frame to a DisplayXR OpenXR runtime instead of weaving it in-process.
// The add-on becomes an ordinary OpenXR application on the game's own graphics device: it submits the left and
// right halves of the back buffer as the two projection views every frame, and the runtime weaves and presents
// them through its display plug-in. Selected with `backend=displayxr` in the [3DGameBridge] section of ReShade.ini;
// when no DisplayXR runtime can be used, try_create returns nullptr and the add-on keeps using the SR weaver.
class DisplayXRBackend : public IGraphicsApi {
    std::string runtime_name;
    std::string status_message;
    std::string format_name;

    HMODULE runtime_module = nullptr;
    PFN_xrGetInstanceProcAddr get_proc_addr = nullptr;

    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId system_id = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace space = XR_NULL_HANDLE;
    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrSessionState session_state = XR_SESSION_STATE_UNKNOWN;
    bool session_running = false;
    bool session_lost = false;

    uint32_t swapchain_width = 0, swapchain_height = 0;
    reshade::api::format swapchain_source_format = reshade::api::format::unknown;
    bool swapchain_failed = false;

    bool weaving_enabled = true;
    bool reshade_overlay_open = false;
    uint64_t frames_submitted = 0;

    DisplayXRGraphics* graphics = nullptr;
    DisplayXROverlayWindow* overlay = nullptr;

    // OpenXR entry points, resolved through the runtime's xrGetInstanceProcAddr
    PFN_xrDestroyInstance xrDestroyInstance = nullptr;
    PFN_xrPollEvent xrPollEvent = nullptr;
    PFN_xrBeginSession xrBeginSession = nullptr;
    PFN_xrEndSession xrEndSession = nullptr;
    PFN_xrDestroySession xrDestroySession = nullptr;
    PFN_xrDestroySpace xrDestroySpace = nullptr;
    PFN_xrWaitFrame xrWaitFrame = nullptr;
    PFN_xrBeginFrame xrBeginFrame = nullptr;
    PFN_xrEndFrame xrEndFrame = nullptr;
    PFN_xrLocateViews xrLocateViews = nullptr;
    PFN_xrEnumerateSwapchainFormats xrEnumerateSwapchainFormats = nullptr;
    PFN_xrCreateSwapchain xrCreateSwapchain = nullptr;
    PFN_xrDestroySwapchain xrDestroySwapchain = nullptr;
    PFN_xrAcquireSwapchainImage xrAcquireSwapchainImage = nullptr;
    PFN_xrWaitSwapchainImage xrWaitSwapchainImage = nullptr;
    PFN_xrReleaseSwapchainImage xrReleaseSwapchainImage = nullptr;

    DisplayXRBackend() = default;

    /// \brief Loads the active OpenXR runtime and creates instance, system, overlay window and session.
    /// \return false with status_message set when DisplayXR cannot be used
    bool initialize(reshade::api::effect_runtime* runtime);
    bool load_runtime();
    void resolve_functions();
    void poll_events();
    void destroy_swapchain();
    bool ensure_swapchain(const reshade::api::resource_desc& desc);

public:
    /// \brief Whether ReShade.ini selects the DisplayXR backend
    static bool is_requested();

    /// \brief Creates the backend for the runtime's device, or returns nullptr when DisplayXR is not usable
    /// (no OpenXR runtime, the active runtime is not DisplayXR, unsupported graphics API, 32-bit process, ...).
    static DisplayXRBackend* try_create(reshade::api::effect_runtime* runtime);

    /// \brief Whether an effect runtime belongs to the window the DisplayXR runtime presents into.
    /// ReShade hooks every swapchain in the process, including the runtime's own; that one must never be woven again.
    static bool is_output_runtime(reshade::api::effect_runtime* runtime);

    /// \brief Makes the destructor skip OpenXR and window teardown. Call before cleanup that runs under the loader
    /// lock (DLL_PROCESS_DETACH): the runtime joins its own threads on teardown, which deadlocks there.
    static void abandon_teardown();

    ~DisplayXRBackend() override;

    /// \brief Hides the woven image while the ReShade overlay is open, so the menu (drawn into the game's own
    /// back buffer after effects) stays readable.
    void on_reshade_overlay(bool open);

    // Inherited via IGraphicsApi
    GbResult on_reshade_finish_effects(reshade::api::effect_runtime* runtime, reshade::api::command_list* cmd_list, reshade::api::resource_view rtv, reshade::api::resource_view rtv_srgb) override;
    void on_init_effect_runtime(reshade::api::effect_runtime* runtime) override;
    void do_weave(bool do_weave) override;
    LatencyModes get_latency_mode() override;
    bool set_latency_in_frames(int32_t number_of_frames) override;
    bool set_latency_frametime_adaptive(uint32_t frametime_in_microseconds) override;
    void draw_status_overlay(reshade::api::effect_runtime* runtime, const std::string& error_message = "") override;
};
