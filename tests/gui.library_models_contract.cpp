// SPDX-License-Identifier: MIT
#include <compare>
#include <cmath>
import gui.library_models;
int main()
{
    epochengine::gui_demo::LibraryModels model;
    if (!model.ready || !model.step()) return 1;
#if EPOCH_DEMO_PLATFORM
    if (model.platform_key.empty()) return 7;
#endif
#if EPOCH_DEMO_AUDIO
    if (model.mixed.metrics.produced_frames != 4000
        || model.mixed.metrics.mixed_sources != 1 || model.mixed.interleaved_samples.size() != 8000) return 2;

    if (model.mixed.interleaved_samples[0] != 0.0f
        || model.mixed.interleaved_samples[2] != 0.25f
        || model.mixed.interleaved_samples[4] != -0.25f) return 8;
#endif
#if EPOCH_DEMO_MEDIA
    if (!model.timeline.source || model.timeline.playhead_seconds != 0.5) return 3;
#endif
    const auto progress = model.progress({{10.0f, 20.0f}, {200.0f, 10.0f}});
    if (std::abs(progress.fraction - 0.05f) > 0.0001f) return 4;
    for (int i = 0; i < 19; ++i) if (!model.step()) return 5;
#if EPOCH_DEMO_MEDIA
    if (model.timeline.playhead_seconds != 0.0) return 6;
#endif
#if EPOCH_DEMO_AUDIO
    if (model.mixer.metrics().frames_produced != 80000) return 9;
#endif
    return 0;
}
