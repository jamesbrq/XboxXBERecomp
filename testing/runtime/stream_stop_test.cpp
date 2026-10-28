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
}
