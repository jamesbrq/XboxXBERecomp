#include "audio/stream_stop.h"
#include <cassert>

int main() {
    audio::StreamStop s;
    assert(!s.StopDue(0) && !s.RearmOnPause(0));
    s.RequestStop(100, 200);
    assert(!s.StopDue(299) && s.StopDue(300));
    s.RequestStop(150, 200);  // repeated stop polls must not postpone completion
    assert(s.release_at == 100 && s.StopDue(300));
    s.FinishStop();
    assert(s.released && s.StopDue(400));  // late old-cue packets stay blocked
    assert(!s.RearmOnPause(1) && s.released);  // pausing is not a new start
    assert(s.RearmOnPause(0) && !s.released && !s.flush_pending);
    assert(!s.StopDue(10000));  // a resumed/new cue cannot be stopped by an old deadline
    s.RequestStop(500, 100);
    assert(s.RearmOnPause(2) && !s.StopDue(10000));  // synchronized reuse
    s.RequestStop(700, 0);
    assert(s.StopDue(700));
    s.FinishStop();
    s.ResetStop();  // synchronous Flush/SetFormat
    assert(!s.released && !s.flush_pending && !s.StopDue(10000));
    bool active = false, paused = false, sync = false;
    s.RequestStop(1000, 0); s.FinishStop();
    // New cue queued before resume: make it active instead of discarding it.
    assert(s.Resume(0, true, active, paused, sync));
    assert(active && !paused && !sync && !s.StopDue(10000));
    s.RequestStop(2000, 0); s.FinishStop(); active = false;
    assert(!s.Resume(1, true, active, paused, sync));
    assert(s.released && !active && paused); // Pause does not rearm late data.
    assert(s.Resume(2, true, active, paused, sync));
    assert(active && !paused && sync && !s.released);
    s.RequestStop(3000, 100); active = true;
    assert(!s.Resume(0, true, active, paused, sync));
    assert(active && !s.flush_pending && !s.released); // Cancel release without losing playback.
    s.FinishStop(); active = false;
    assert(!s.Resume(0, false, active, paused, sync));
    assert(!active && !s.released); // Resume-before-Process remains valid too.
    // Mode 3 does not activate an idle voice or leave submitted packets held.
    assert(!s.Resume(3, false, active, paused, sync));
    assert(!paused && !active && !sync);
    active = true; // Process can activate when data is submitted.
    assert(!paused);
    assert(!s.Resume(0, true, active, paused, sync));
    assert(active && !paused && !sync);
    s.FinishStop(); active = false;
    assert(!s.Resume(3, true, active, paused, sync));
    assert(s.released && !paused && !active); // Mode 3 never rearms a stopped cue.
    s.ResetStop(); active = true; paused = true; sync = true;
    assert(!s.Resume(3, true, active, paused, sync));
    assert(active && !paused && !sync); // SDK active-voice branch clears both holds.
}
