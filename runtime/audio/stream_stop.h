#pragma once
#include <cstdint>

namespace audio {

// An asynchronous cue stop must reject late packets until the owner explicitly
// resumes/reuses the stream. Queue completion remains on the audio mixer.
struct StreamStop {
    bool flush_pending = false, released = false;
    int64_t release_at = 0, release_ms = 0;

    void RequestStop(int64_t now, int64_t duration) {
        if (!flush_pending) release_at = now;
        release_ms = duration;
        flush_pending = true;
    }
    bool StopDue(int64_t now) const {
        return released || (flush_pending && now - release_at >= release_ms);
    }
    void FinishStop() { flush_pending = false; released = true; }
    void ResetStop() { flush_pending = released = false; release_at = release_ms = 0; }
    bool RearmOnPause(uint32_t mode) {
        if ((mode != 0 && mode != 2) || (!released && !flush_pending)) return false;
        ResetStop();
        return true;
    }
    // Resume is allowed after the caller queues the next cue's packets. Reset
    // stop state without flushing that queue; Process alone still cannot rearm.
    bool Resume(uint32_t mode, bool queued, bool& active, bool& paused, bool& sync) {
        const bool rearmed = RearmOnPause(mode);
        const bool starting = rearmed && queued && !active;
        if (starting) active = true;
        paused = mode == 1;
        sync = mode == 2;
        return starting;
    }
};

}  // namespace audio
