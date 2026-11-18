// SPDX-License-Identifier: MIT
module;
#include <compare>
#include <cstdint>
#include <optional>
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
        static constexpr media::timeline_preview::AtlasLayout atlas{5, 4};
        static constexpr std::uint32_t frame_width = 16;
        static constexpr std::uint32_t frame_height = 9;
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
                .logical_path = "demo/generated_atlas",
                .content_sha256 = std::string(64, '0'),
                .provider_revision = "generated-atlas-demo-v1",
                .source_width = frame_width, .source_height = frame_height,
                .frame_count = 20, .frame_rate = {2, 1}, .duration_seconds = 10.0};
            if (!media::admit(timeline, source, {1, 1})) return;
            if (!media::set_loop(timeline, true)) return;
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
            namespace media = epochengine::media::timeline_preview;
            const auto current = media::current_frame_index(timeline);
            if (!current) return false;
            auto next = *current + 1;
            if (timeline.loop && next == timeline.source->frame_count) next = 0;
            return media::seek(timeline, static_cast<double>(next) / timeline.source->frame_rate.value());
#else
            return true;
#endif
        }

#if EPOCH_DEMO_MEDIA
        [[nodiscard]] bool playing() const noexcept
        {
            return timeline.transport == media::timeline_preview::TransportMode::playing;
        }

        bool toggle_playback() noexcept
        {
            return playing() ? media::timeline_preview::pause(timeline)
                             : media::timeline_preview::play(timeline);
        }

        bool seek_fraction(double fraction) noexcept
        {
            return timeline.source && media::timeline_preview::seek(timeline,
                fraction * timeline.source->duration_seconds);
        }

        [[nodiscard]] std::optional<media::timeline_preview::AtlasFrame> active_frame() const noexcept
        {
            return media::timeline_preview::current_atlas_frame(timeline, atlas);
        }
#endif

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
