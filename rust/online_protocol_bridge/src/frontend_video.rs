use crate::ffi::{
    SettingsPanel, SettingsPanelRow, SettingsText, SettingsTextSource, VideoModeSize,
    VideoPanelFacts,
};
use crate::frontend_settings::{authored_choices, choices, literal, panel, referenced};
use SettingsTextSource as Source;

// Use the existing source screens as the layout/style references. Their list
// skins supply fonts, spacing, scroll limits, and hit geometry to the painter.
#[derive(Clone, Copy)]
enum Screen {
    Video = 256,
    Resolution = 269,
    DisplayMode = 271,
    Brightness = 272,
    Gamma = 273,
    Upscaling = 274,
    RestoreDefaults = 276,
    SafeArea = 285,
    Detail = 290,
    ConfirmMode = 293,
}

impl Screen {
    const fn id(self) -> u32 {
        self as u32
    }
}

// Contexts reuse the source screens below. Match display_settings.hpp and the
// native accept handlers, which own the settings and desktop transactions.
const INTERNAL_RESOLUTION: u32 = 1;
const GRAPHICS_MODE: u32 = 1;
const ON_FOOT_FOV: u32 = 2;
const VEHICLE_FOV: u32 = 3;
const UPSCALING_METHOD: u32 = 1;
const UPSCALING_QUALITY: u32 = 2;
const PATH_TRACING: u32 = 3;
const RAY_SAMPLES: u32 = 4;
const BOUNCE_DEPTH: u32 = 5;
const PATH_TRACING_HELP: &str =
    "More ray samples and bounces increase GPU cost. Both default to 1.";

#[derive(Clone, Copy)]
enum Caption {
    Text(&'static str),
    Authored(u32),
    WithValue(&'static str),
}
use Caption::{Authored, Text, WithValue};

#[derive(Clone, Copy)]
enum Action {
    Open(Screen, u32),
    Apply(i32),
}
use Action::{Apply, Open};

#[derive(Clone, Copy)]
enum Readout {
    None,
    GraphicsMode,
    Fullscreen,
    Resolution,
    InternalResolution,
    Brightness,
    Gamma,
    Upscaler,
    Quality,
    Detail,
    SafeArea,
    OnFootFov,
    VehicleFov,
    Crt,
    Split,
    ObjectMotionBlur,
    PathTracing,
    RaySamples,
    BounceDepth,
}

struct LayoutRow {
    caption: Caption,
    action: Action,
    readout: Readout,
}

const fn row(caption: Caption, action: Action, readout: Readout) -> LayoutRow {
    LayoutRow {
        caption,
        action,
        readout,
    }
}

const VIDEO: &[LayoutRow] = &[
    row(
        Text("Graphics Mode"),
        Open(Screen::DisplayMode, GRAPHICS_MODE),
        Readout::GraphicsMode,
    ),
    row(
        Text("Fullscreen Mode"),
        Open(Screen::DisplayMode, 0),
        Readout::Fullscreen,
    ),
    row(
        Authored(0x897),
        Open(Screen::Resolution, 0),
        Readout::Resolution,
    ),
    row(
        Text("Internal Resolution"),
        Open(Screen::Resolution, INTERNAL_RESOLUTION),
        Readout::InternalResolution,
    ),
    row(
        Authored(0x89a),
        Open(Screen::Brightness, 0),
        Readout::Brightness,
    ),
    row(Authored(0x89c), Open(Screen::Gamma, 0), Readout::Gamma),
    row(
        Text("Upscaling"),
        Open(Screen::Upscaling, 0),
        Readout::Upscaler,
    ),
    row(Authored(0x95e), Open(Screen::Detail, 0), Readout::Detail),
    row(
        Authored(0x907),
        Open(Screen::SafeArea, 0),
        Readout::SafeArea,
    ),
    row(
        Text("Field of View"),
        Open(Screen::Brightness, ON_FOOT_FOV),
        Readout::OnFootFov,
    ),
    row(
        Text("Vehicle Field of View"),
        Open(Screen::Brightness, VEHICLE_FOV),
        Readout::VehicleFov,
    ),
    row(Text("CRT Filter"), Apply(0), Readout::Crt),
    row(Text("2-Player Split Screen"), Apply(1), Readout::Split),
    row(
        Text("Per-Object Motion Blur"),
        Apply(2),
        Readout::ObjectMotionBlur,
    ),
    row(
        Authored(0x89f),
        Open(Screen::RestoreDefaults, 0),
        Readout::None,
    ),
];
const PATH_TRACING_ROW: LayoutRow = row(
    Text("Path Tracing Quality"),
    Open(Screen::Upscaling, PATH_TRACING),
    Readout::PathTracing,
);
const UPSCALING: &[LayoutRow] = &[
    row(
        WithValue("Method: "),
        Open(Screen::Upscaling, UPSCALING_METHOD),
        Readout::Upscaler,
    ),
    row(
        WithValue("Quality: "),
        Open(Screen::Upscaling, UPSCALING_QUALITY),
        Readout::Quality,
    ),
];
const PATH_QUALITY: &[LayoutRow] = &[
    row(
        WithValue("Ray Samples Per Pixel: "),
        Open(Screen::Upscaling, RAY_SAMPLES),
        Readout::RaySamples,
    ),
    row(
        WithValue("Bounce Depth: "),
        Open(Screen::Upscaling, BOUNCE_DEPTH),
        Readout::BounceDepth,
    ),
];

const BRIGHTNESS_LABELS: u32 = 0x8ac;
const GAMMA_LABELS: u32 = 0x8b6;
const DETAIL_LABELS: [u32; 3] = [0x95f, 0x960, 0x961];
const SAFE_AREA_LABELS: [u32; 2] = [0x183, 0x182];

fn size_label(size: &VideoModeSize) -> SettingsText {
    literal(format!("{} x {}", size.width, size.height))
}

fn readout(value: Readout, facts: &VideoPanelFacts) -> SettingsText {
    match value {
        Readout::None => literal(""),
        Readout::GraphicsMode => referenced(Source::GraphicsMode, facts.graphics_mode),
        Readout::Fullscreen => referenced(Source::FullscreenMode, facts.fullscreen),
        Readout::Resolution => size_label(&facts.window),
        Readout::InternalResolution if facts.upscaling_enabled => literal("Automatic"),
        Readout::InternalResolution if facts.internal.width == 0 => literal("Match Screen"),
        Readout::InternalResolution => size_label(&facts.internal),
        Readout::Brightness => referenced(
            Source::LocalizedLowId,
            BRIGHTNESS_LABELS.wrapping_add(facts.brightness),
        ),
        Readout::Gamma => referenced(
            Source::LocalizedLowId,
            GAMMA_LABELS.wrapping_add(facts.gamma),
        ),
        Readout::Upscaler => referenced(Source::Upscaler, facts.upscaler),
        Readout::Quality => referenced(Source::UpscalingQuality, facts.upscaling_quality),
        Readout::Detail => referenced(
            Source::LocalizedLowId,
            DETAIL_LABELS[0].wrapping_add(facts.detail),
        ),
        Readout::SafeArea => SettingsText {
            screen: Screen::SafeArea.id(),
            ..referenced(
                Source::LocalizedLowId,
                SAFE_AREA_LABELS[usize::from(facts.safe_area)],
            )
        },
        Readout::OnFootFov => literal(facts.field_of_view.to_string()),
        Readout::VehicleFov => literal(facts.vehicle_field_of_view.to_string()),
        Readout::Crt => literal(if facts.crt_filter { "On" } else { "Off" }),
        Readout::Split => literal(if facts.vertical_split {
            "Vertical"
        } else {
            "Horizontal"
        }),
        Readout::ObjectMotionBlur => literal(if facts.object_motion_blur {
            "On"
        } else {
            "Off"
        }),
        Readout::RaySamples => literal(facts.ray_samples.to_string()),
        Readout::BounceDepth => literal(facts.bounce_depth.to_string()),
        Readout::PathTracing => literal(format!(
            "{} sample{}, {} bounce{}",
            facts.ray_samples,
            if facts.ray_samples == 1 { "" } else { "s" },
            facts.bounce_depth,
            if facts.bounce_depth == 1 { "" } else { "s" },
        )),
    }
}

fn entry(label: SettingsText, action: Action) -> SettingsPanelRow {
    let (destination, value) = match action {
        Open(screen, context) => (screen.id() as i32, context as i32),
        Apply(value) => (-2, value),
    };
    SettingsPanelRow {
        label,
        vista_route: matches!(action, Open(_, _)),
        destination,
        value,
    }
}

fn layout_entry(row: &LayoutRow, facts: &VideoPanelFacts) -> SettingsPanelRow {
    let label = match row.caption {
        Text(text) => literal(text),
        Authored(id) => referenced(Source::LocalizedLowId, id),
        WithValue(prefix) => {
            let mut label = readout(row.readout, facts);
            label.text.insert_str(0, prefix);
            label
        }
    };
    entry(label, row.action)
}

fn numbered_choices(
    title: &str,
    help: &str,
    selected: u32,
    numbers: std::ops::RangeInclusive<u32>,
    unit: &str,
) -> SettingsPanel {
    panel(
        title,
        literal(help),
        selected.wrapping_sub(*numbers.start()),
        numbers.map(|number| entry(literal(format!("{number}{unit}")), Apply(number as i32))),
    )
}

fn size_choices(internal: bool, facts: &VideoPanelFacts, modes: &[VideoModeSize]) -> SettingsPanel {
    let mut result = panel("", literal(""), 0, []);
    let active = if internal {
        result.title = "INTERNAL RESOLUTION".into();
        result
            .rows
            .push(entry(literal("Match Screen Resolution"), Apply(-1)));
        &facts.internal
    } else {
        &facts.window
    };
    for (index, mode) in modes.iter().enumerate() {
        if mode.width == active.width && mode.height == active.height {
            result.selected = result.rows.len() as u32;
        }
        result
            .rows
            .push(entry(size_label(mode), Apply(index as i32)));
    }
    result
}

fn scaling_panel(context: u32, facts: &VideoPanelFacts) -> SettingsPanel {
    match context {
        0 => panel(
            "UPSCALING",
            literal(if facts.upscaling_enabled {
                "Upscale the 3D world. Menus and HUD stay at native resolution."
            } else {
                "Upscaling is off. Choose a method to upscale the world. Menus and HUD stay at native resolution."
            }),
            0,
            UPSCALING.iter().map(|row| layout_entry(row, facts)),
        ),
        UPSCALING_METHOD => choices(
            "UPSCALING METHOD",
            literal(
                "FSR supports a wide range of graphics cards. DLSS requires an NVIDIA RTX graphics card.",
            ),
            facts.upscaler,
            (0..3).map(|index| referenced(Source::Upscaler, index)),
        ),
        PATH_TRACING => panel(
            "PATH TRACING QUALITY",
            literal(PATH_TRACING_HELP),
            0,
            PATH_QUALITY.iter().map(|row| layout_entry(row, facts)),
        ),
        RAY_SAMPLES => numbered_choices(
            "RAY SAMPLES PER PIXEL",
            PATH_TRACING_HELP,
            facts.ray_samples,
            1..=2,
            "",
        ),
        context if context >= BOUNCE_DEPTH => numbered_choices(
            "BOUNCE DEPTH",
            PATH_TRACING_HELP,
            facts.bounce_depth,
            1..=2,
            "",
        ),
        _ => choices(
            "QUALITY MODE",
            literal(if facts.upscaling_enabled {
                "DLAA and Native AA render the world at full resolution. Other modes trade detail for performance."
            } else {
                "Choose a quality mode, then select an upscaling method to enable it."
            }),
            facts.upscaling_quality,
            (0..6).map(|index| referenced(Source::UpscalingQuality, index)),
        ),
    }
}

pub(crate) fn frontend_video_panel(
    screen: u32,
    context: u32,
    facts: &VideoPanelFacts,
    modes: &[VideoModeSize],
    fov_min: u32,
    fov_max: u32,
) -> SettingsPanel {
    match screen {
        id if id == Screen::Video.id() => panel(
            "",
            literal(""),
            0,
            VIDEO
                .iter()
                .chain(facts.path_tracing.then_some(&PATH_TRACING_ROW))
                .map(|row| layout_entry(row, facts)),
        ),
        id if id == Screen::Resolution.id() => {
            size_choices(context == INTERNAL_RESOLUTION, facts, modes)
        }
        id if id == Screen::DisplayMode.id() && context == GRAPHICS_MODE => choices(
            "GRAPHICS MODE",
            SettingsText {
                text: "Current mode: ".into(),
                suffix: ".".into(),
                ..readout(Readout::GraphicsMode, facts)
            },
            facts.graphics_mode,
            (0..3).map(|index| referenced(Source::GraphicsMode, index)),
        ),
        id if id == Screen::DisplayMode.id() => choices(
            "FULLSCREEN MODE",
            literal(""),
            facts.fullscreen,
            (0..3).map(|index| referenced(Source::FullscreenMode, index)),
        ),
        id if id == Screen::Brightness.id() => match context {
            ON_FOOT_FOV => numbered_choices(
                "FIELD OF VIEW",
                "Widen the view while on foot.",
                facts.field_of_view,
                fov_min..=fov_max,
                " degrees",
            ),
            VEHICLE_FOV => numbered_choices(
                "VEHICLE FIELD OF VIEW",
                "70 degrees preserves the Xbox TU1.5 vehicle camera. Higher values widen its view.",
                facts.vehicle_field_of_view,
                fov_min..=fov_max,
                " degrees",
            ),
            _ => authored_choices(BRIGHTNESS_LABELS..BRIGHTNESS_LABELS + 10, facts.brightness),
        },
        id if id == Screen::Gamma.id() => {
            authored_choices(GAMMA_LABELS..GAMMA_LABELS + 11, facts.gamma)
        }
        id if id == Screen::Upscaling.id() => scaling_panel(context, facts),
        id if id == Screen::RestoreDefaults.id() => authored_choices([0x8a0, 0x8a1], 1),
        id if id == Screen::SafeArea.id() => {
            authored_choices(SAFE_AREA_LABELS, u32::from(facts.safe_area))
        }
        id if id == Screen::Detail.id() => authored_choices(DETAIL_LABELS, facts.detail),
        id if id == Screen::ConfirmMode.id() => choices(
            "",
            literal(""),
            0,
            [referenced(Source::LocalizedId, 0x02000f02)],
        ),
        _ => unreachable!("unsupported source Video screen {screen}"),
    }
}

pub(crate) fn frontend_video_value(
    screen: u32,
    context: u32,
    index: u32,
    facts: &VideoPanelFacts,
) -> SettingsText {
    let index = index as usize;
    let row = if screen == Screen::Video.id() {
        VIDEO
            .get(index)
            .or_else(|| (index == VIDEO.len()).then_some(&PATH_TRACING_ROW))
    } else if context == 0 {
        UPSCALING.get(index)
    } else if context == PATH_TRACING {
        PATH_QUALITY.get(index)
    } else {
        None
    };
    readout(row.map_or(Readout::None, |row| row.readout), facts)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn facts() -> VideoPanelFacts {
        VideoPanelFacts {
            window: VideoModeSize {
                width: 1280,
                height: 720,
            },
            internal: VideoModeSize {
                width: 800,
                height: 600,
            },
            graphics_mode: 0,
            path_tracing: false,
            fullscreen: 0,
            upscaler: 0,
            upscaling_quality: 2,
            upscaling_enabled: false,
            brightness: 3,
            gamma: 0,
            detail: 0,
            safe_area: false,
            field_of_view: 78,
            vehicle_field_of_view: 70,
            crt_filter: false,
            vertical_split: false,
            object_motion_blur: false,
            ray_samples: 1,
            bounce_depth: 2,
        }
    }

    fn view(screen: Screen, context: u32, facts: &VideoPanelFacts) -> SettingsPanel {
        frontend_video_panel(screen.id(), context, facts, &[], 70, 120)
    }

    #[test]
    fn overview_keeps_labels_readouts_and_actions_in_the_same_order() {
        let mut facts = facts();
        let panel = view(Screen::Video, 0, &facts);
        assert_eq!(panel.rows.len(), 15);
        let actions: Vec<_> = panel
            .rows
            .iter()
            .map(|row| (row.destination, row.value))
            .collect();
        assert_eq!(
            actions,
            [
                (271, 1),
                (271, 0),
                (269, 0),
                (269, 1),
                (272, 0),
                (273, 0),
                (274, 0),
                (290, 0),
                (285, 0),
                (272, 2),
                (272, 3),
                (-2, 0),
                (-2, 1),
                (-2, 2),
                (276, 0)
            ]
        );
        assert_eq!(frontend_video_value(256, 0, 3, &facts).text, "800 x 600");
        assert_eq!(frontend_video_value(256, 0, 4, &facts).key, 0x8af);
        let safe_area = frontend_video_value(256, 0, 8, &facts);
        assert_eq!((safe_area.screen, safe_area.key), (285, 0x183));
        assert_eq!(frontend_video_value(256, 0, 13, &facts).text, "Off");
        facts.object_motion_blur = true;
        assert_eq!(frontend_video_value(256, 0, 13, &facts).text, "On");
        facts.upscaling_enabled = true;
        facts.path_tracing = true;
        assert_eq!(frontend_video_value(256, 0, 3, &facts).text, "Automatic");
        let panel = view(Screen::Video, 0, &facts);
        assert_eq!(panel.rows.len(), 16);
        assert_eq!((panel.rows[15].destination, panel.rows[15].value), (274, 3));
        assert_eq!(
            frontend_video_value(256, 0, 15, &facts).text,
            "1 sample, 2 bounces"
        );
    }

    #[test]
    fn resolutions_and_slider_lists_keep_native_values_and_focus() {
        let facts = facts();
        let sizes = [
            VideoModeSize {
                width: 800,
                height: 600,
            },
            VideoModeSize {
                width: 1280,
                height: 720,
            },
        ];
        let screen = Screen::Resolution.id();
        let panel = frontend_video_panel(screen, 0, &facts, &sizes, 70, 120);
        assert_eq!(panel.selected, 1);
        assert_eq!(panel.rows[1].label.text, "1280 x 720");
        assert_eq!(panel.rows[1].value, 1);
        let panel = frontend_video_panel(screen, INTERNAL_RESOLUTION, &facts, &sizes, 70, 120);
        assert_eq!(panel.rows[0].value, -1);
        assert_eq!(panel.selected, 1);
        assert_eq!(panel.rows[1].value, 0);
        let panel = view(Screen::Brightness, ON_FOOT_FOV, &facts);
        assert_eq!(panel.selected, 8);
        assert_eq!(panel.rows.len(), 51);
        assert_eq!(panel.rows[0].value, 70);
        assert_eq!(panel.rows[50].label.text, "120 degrees");
        assert_eq!(view(Screen::Brightness, VEHICLE_FOV, &facts).selected, 0);
        assert_eq!(view(Screen::RestoreDefaults, 0, &facts).selected, 1);
    }

    #[test]
    fn scaling_and_path_tracing_keep_separate_choice_contexts() {
        let mut facts = facts();
        facts.upscaler = 2;
        facts.upscaling_enabled = true;
        facts.upscaling_quality = 0;
        let overview = view(Screen::Upscaling, 0, &facts);
        let quality = &overview.rows[1];
        assert_eq!(quality.label.text, "Quality: ");
        assert!(matches!(quality.label.source, Source::UpscalingQuality));
        assert_eq!(
            (quality.label.key, quality.destination, quality.value),
            (0, 274, 2)
        );
        let choices = view(Screen::Upscaling, UPSCALING_QUALITY, &facts);
        assert_eq!(choices.selected, 0);
        assert_eq!(choices.rows.len(), 6);
        assert_eq!(choices.rows[5].value, 5);
        let path = view(Screen::Upscaling, PATH_TRACING, &facts);
        assert_eq!(path.rows[1].label.text, "Bounce Depth: 2");
        assert_eq!((path.rows[1].destination, path.rows[1].value), (274, 5));
        let samples = view(Screen::Upscaling, RAY_SAMPLES, &facts);
        assert_eq!(samples.description.text, path.description.text);
        assert_eq!(samples.selected, 0);
        assert_eq!(samples.rows[1].value, 2);
        let bounces = view(Screen::Upscaling, BOUNCE_DEPTH, &facts);
        assert_eq!(bounces.description.text, path.description.text);
        assert_eq!(bounces.selected, 1);
        assert_eq!(bounces.rows[0].value, 1);
    }
}
