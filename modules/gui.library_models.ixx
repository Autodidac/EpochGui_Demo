// SPDX-License-Identifier: MIT
module;
#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
export module gui.library_models;
import epoch.gui;
#if EPOCH_DEMO_PLATFORM
import platform.native;
#endif
#if EPOCH_DEMO_AUDIO
import audio.mixer;
#endif
#if EPOCH_DEMO_MEDIA
import media.timeline_preview;
#endif

export namespace epochengine::gui_demo
{
    // Demo composition, not another scheduler, renderer or device backend.
    class LibraryModels
    {
    public:
#if EPOCH_DEMO_MEDIA
        media::timeline_preview::PreviewState timeline{};
#endif
#if EPOCH_DEMO_AUDIO
        audio::AudioManager manager{};
        audio::AudioMixer mixer{};
        audio::MixedAudioFrame mixed{};
#endif
        std::uint64_t frame{};
        bool ready{};
#if EPOCH_DEMO_PLATFORM
        std::string platform_key{platform::to_string(platform::current_platform())};
        platform::RuntimePolicy native_policy{platform::current_runtime_policy()};

#endif
        LibraryModels()
        {
#if EPOCH_DEMO_MEDIA
            namespace media = epochengine::media::timeline_preview;
            media::SourceDescriptor source{
                .logical_path = "demo/generated_timeline",
                .content_sha256 = std::string(64, '0'),
                .provider_revision = "generated-metadata-demo-v1",
                .source_width = 1920, .source_height = 1080,
                .frame_count = 300, .frame_rate = {30, 1}, .duration_seconds = 10.0};
            if (!media::admit(timeline, source, {1, 1})) return;
            timeline.loop = true;
            if (!media::play(timeline)) return;
#endif
#if EPOCH_DEMO_AUDIO
            const audio::ClipId clip{71, 1};
            if (mixer.register_clip({.id = clip,
                .format = {.channel_layout = audio::PcmChannelLayout::mono, .sample_rate = 8000},
                .frame_count = 4, .interleaved_samples = {0.0f, 0.25f, -0.25f, 0.0f}})
                    != audio::AudioMixerCode::success) return;
            const auto bus = manager.bus_snapshot(manager.master_bus());
            if (!bus || mixer.bind_bus({bus->id, manager.master_bus()}) != audio::AudioMixerCode::success) return;
            const auto voice = manager.create_source({
                .clip = {.id = clip, .duration_seconds = 0.0005, .nominal_sample_rate = 8000,
                    .channel_count = 1, .seekable = true},
                .bus = manager.master_bus(), .looping = true});
            if (!voice.succeeded()) return;
            if (!manager.submit({.target_frame = 1,
                .payload = audio::PlayCommand{.source = voice.handle}}).accepted()) return;
#endif
            ready = true;
        }

        bool step()
        {
            if (!ready) return false;
            ++frame;
#if EPOCH_DEMO_AUDIO
            const auto plan = manager.prepare_frame({.frame_index = frame,
                .timeline_seconds = static_cast<double>(frame) * 0.5, .duration_seconds = 0.5});
            mixed = mixer.mix(plan, {.output_format = {.sample_rate = 8000}, .output_frame_count = 4000});
            if (!mixed) { ready = false; return false; }
#endif
#if EPOCH_DEMO_MEDIA
            return media::timeline_preview::advance(timeline, 0.5);
#else
            return true;
#endif
        }

        [[nodiscard]] gui_lib::ProgressBarLayout progress(gui_lib::Rect track) const
        {
            return gui_lib::make_progress_bar_layout({.track = track,
#if EPOCH_DEMO_MEDIA
                .value = static_cast<float>(timeline.playhead_seconds),
#else
                .value = static_cast<float>(frame % 20u) * 0.5f,
#endif
                .maximum = 10.0f});
        }
    };
}
