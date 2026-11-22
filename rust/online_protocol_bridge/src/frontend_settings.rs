use crate::ffi::{
    PcSettingsFacts, SettingsPanel, SettingsPanelRow, SettingsText, SettingsTextSource as Source,
};

fn apply(label: SettingsText, value: i32) -> SettingsPanelRow {
    SettingsPanelRow {
        label,
        vista_route: false,
        destination: -2,
        value,
    }
}

pub(crate) fn literal(text: impl Into<String>) -> SettingsText {
    SettingsText {
        source: Source::Literal,
        key: 0,
        screen: 0,
        text: text.into(),
        suffix: String::new(),
    }
}

pub(crate) fn referenced(source: Source, key: u32) -> SettingsText {
    SettingsText {
        source,
        key,
        ..literal("")
    }
}

pub(crate) fn panel(
    title: &str,
    description: SettingsText,
    selected: u32,
    rows: impl IntoIterator<Item = SettingsPanelRow>,
) -> SettingsPanel {
    SettingsPanel {
        title: title.into(),
        description,
        selected,
        rows: rows.into_iter().collect(),
    }
}

pub(crate) fn choices(
    title: &str,
    description: SettingsText,
    selected: u32,
    labels: impl IntoIterator<Item = SettingsText>,
) -> SettingsPanel {
    panel(
        title,
        description,
        selected,
        labels
            .into_iter()
            .enumerate()
            .map(|(index, label)| apply(label, index as i32)),
    )
}

pub(crate) fn authored_choices(ids: impl IntoIterator<Item = u32>, selected: u32) -> SettingsPanel {
    choices(
        "",
        literal(""),
        selected,
        ids.into_iter()
            .map(|id| referenced(Source::LocalizedLowId, id)),
    )
}

#[derive(Clone, Copy)]
enum Route {
    Vista(u32, i32),
    Retail(u32, i32),
    Overlay {
        shell: u32,
        pause: u32,
        context: i32,
    },
    Set(i32),
}

#[derive(Clone, Copy)]
enum Readout {
    Blank,
    Sticks,
    Buttons,
    ControllerSensitivity,
    ControllerInvert,
    ControllerAutoCenter,
    Vibration,
    KeyboardPreset,
    KeyboardSensitivity,
    KeyboardInvert,
    KeyboardAutoCenter,
    DualWieldInvert,
    Volume,
    Speakers,
    Subtitles,
}

use Readout::*;
use Route::*;

// Keep each source string, destination/context and adjacent readout together.
// A pause alias uses its own screen and skin even when it shares this layout.
struct OptionRow(u32, Route, Readout);

const SETTINGS: &[OptionRow] = &[
    OptionRow(0x197, Retail(193, 0), Blank),
    OptionRow(0x198, Retail(60, 1), Blank),
    OptionRow(0x820, Vista(256, 0), Blank),
    OptionRow(0x81f, Vista(257, 0), Blank),
    OptionRow(0x976, Vista(296, 0), Blank),
    OptionRow(0x821, Vista(297, 0), Blank),
];
const PAUSE_SETTINGS: &[OptionRow] = &[
    OptionRow(0x2ef, Vista(200, 0), Blank),
    OptionRow(0x7c2, Vista(260, 0), Blank),
    OptionRow(0x81f, Vista(262, 0), Blank),
    OptionRow(0x820, Vista(261, 0), Blank),
];
const CONTROLLER: &[OptionRow] = &[
    OptionRow(0x3dd, Vista(41, 1), Sticks),
    OptionRow(0x3de, Vista(40, 1), Buttons),
    OptionRow(0x3df, Vista(42, 1), ControllerSensitivity),
    OptionRow(0x3e0, Vista(43, 1), ControllerInvert),
    OptionRow(0x3e1, Vista(44, 1), ControllerAutoCenter),
    OptionRow(0x3e2, Vista(183, 1), Vibration),
];
// PC 65975E/6598F5. Custom is source preset 4 in visible slot 3.
const KEYBOARD: &[OptionRow] = &[
    OptionRow(
        0x7c2,
        Overlay {
            shell: 243,
            pause: 244,
            context: 0,
        },
        KeyboardPreset,
    ),
    OptionRow(0x3df, Vista(42, 0), KeyboardSensitivity),
    OptionRow(0x3e0, Vista(294, 0), KeyboardInvert),
    OptionRow(0x3e1, Vista(44, 0), KeyboardAutoCenter),
    OptionRow(0x7c7, Vista(245, 0), DualWieldInvert),
    OptionRow(0x848, Vista(295, 0), Blank),
];
const PRESETS: &[OptionRow] = &[
    OptionRow(0x83f, Set(0), Blank),
    OptionRow(0x840, Set(1), Blank),
    OptionRow(0x841, Set(2), Blank),
    OptionRow(
        0x843,
        Overlay {
            shell: 253,
            pause: 254,
            context: 4,
        },
        Blank,
    ),
];
const AUDIO: &[OptionRow] = &[
    OptionRow(0x876, Vista(263, 0), Volume),
    OptionRow(0x87a, Vista(267, 0), Speakers),
    OptionRow(0x2f1, Vista(242, 0), Subtitles),
    OptionRow(0x88a, Vista(268, 0), Blank),
];

fn layout(screen: u32) -> Option<&'static [OptionRow]> {
    match screen {
        19 => Some(SETTINGS),
        259 => Some(PAUSE_SETTINGS),
        200 => Some(CONTROLLER),
        258 => Some(KEYBOARD),
        243 => Some(PRESETS),
        257 => Some(AUDIO),
        _ => None,
    }
}

fn localized(key: u32) -> SettingsText {
    referenced(Source::LocalizedLowId, key)
}

fn from_screen(screen: u32, key: u32) -> SettingsText {
    SettingsText {
        screen,
        ..localized(key)
    }
}

fn enabled(on: bool) -> SettingsText {
    localized(if on { 0x308 } else { 0x309 })
}

impl OptionRow {
    fn entry(&self, pause: bool) -> SettingsPanelRow {
        let (vista_route, destination, value) = match self.1 {
            Vista(id, context) => (true, id as i32, context),
            Retail(id, context) => (false, id as i32, context),
            Overlay {
                shell,
                pause: alias,
                context,
            } => (true, if pause { alias } else { shell } as i32, context),
            Set(value) => (false, -2, value),
        };
        SettingsPanelRow {
            label: localized(self.0),
            vista_route,
            destination,
            value,
        }
    }
    fn value(&self, facts: &PcSettingsFacts) -> SettingsText {
        match self.2 {
            Blank => literal(""),
            Sticks => from_screen(
                41,
                if facts.sticks == 0 {
                    1
                } else {
                    0x2f4 + facts.sticks
                },
            ),
            Buttons => from_screen(
                40,
                if facts.buttons == 0 {
                    1
                } else {
                    0x3e3 + facts.buttons
                },
            ),
            ControllerSensitivity => literal(facts.controller_sensitivity.to_string()),
            ControllerInvert => enabled(facts.controller_invert),
            ControllerAutoCenter => enabled(facts.controller_auto_center),
            Vibration => from_screen(183, if facts.vibration { 0x182 } else { 0x183 }),
            KeyboardPreset if facts.default_bindings => literal("MCC Default"),
            KeyboardPreset => from_screen(if facts.gameplay_overlay { 244 } else { 0 }, 0x844),
            KeyboardSensitivity => literal(facts.keyboard_sensitivity.to_string()),
            KeyboardInvert => enabled(facts.keyboard_invert),
            KeyboardAutoCenter => enabled(facts.keyboard_auto_center),
            DualWieldInvert => enabled(facts.dual_wield_invert),
            Volume if facts.volume == 0 => localized(0x957),
            Volume => literal(facts.volume.to_string()),
            Speakers => localized(0x87b_u32.wrapping_add(facts.speakers)),
            Subtitles => localized(match facts.subtitles {
                0 => 1,
                1 => 0x182,
                _ => 0x183,
            }),
        }
    }
}

// PC 634CB6..634E0B/641333. Four headings divide the original advanced editor.
const EDITOR_LABELS: &[u32] = &[
    0x2100082e, 0x1e000832, 0x1b000833, 0x1d000830, 0x1e000831, 0x16000834, 0x18000835, 0x19000857,
    0x1b000858, 0x1b000859, 0x1c00085a, 0x1f00082f, 0x16000836, 0x2200085c, 0x1f000837, 0x2000083b,
    0x1d00085e, 0x1f00083d, 0x2100085f, 0x1f00083c, 0x1e000838, 0x1d00083e, 0x18000956, 0x20000855,
    0x1f000861, 0x1e000862, 0x17000864, 0x21000865, 0x1e000866, 0x21000867, 0x2300097f, 0x1e000868,
    0x1d000869, 0x20000856, 0x1e00086b, 0x1c00086c, 0x1600086d, 0x19000962, 0x1a000963, 0x1d00086e,
    0x1b00086f, 0x28000978, 0x1e00097e,
];
fn binding_editor(actions: &[u32]) -> SettingsPanel {
    let rows = EDITOR_LABELS.iter().map(|&id| {
        let action = match id {
            0x2100082e | 0x1f00082f | 0x20000855 | 0x20000856 => -2,
            _ => actions
                .iter()
                .position(|&a| a == id)
                .map_or(-1, |index| index as i32),
        };
        apply(referenced(Source::LocalizedId, id), action)
    });
    panel(
        "",
        literal("Enter: primary binding   X: secondary binding   Backspace: clear   Esc: cancel"),
        1,
        rows,
    )
}

fn indexed_from_one(ids: impl IntoIterator<Item = u32>, selected: u32) -> SettingsPanel {
    panel(
        "",
        literal(""),
        selected,
        ids.into_iter()
            .enumerate()
            .map(|(i, id)| apply(localized(id), i as i32 + 1)),
    )
}

fn yes_first(on_label: u32, off_label: u32, on: bool) -> SettingsPanel {
    panel(
        "",
        literal(""),
        u32::from(!on),
        [
            apply(localized(on_label), 1),
            apply(localized(off_label), 0),
        ],
    )
}

pub(crate) fn frontend_pc_settings_panel(
    screen: u32,
    context: u32,
    facts: &PcSettingsFacts,
    action_labels: &[u32],
) -> SettingsPanel {
    if let Some(rows) = layout(screen) {
        let selected = if screen == 243 {
            facts.keyboard_preset.clamp(0, 3) as u32
        } else {
            0
        };
        return panel(
            "",
            literal(""),
            selected,
            rows.iter().map(|row| row.entry(facts.gameplay_overlay)),
        );
    }
    match screen {
        40 => authored_choices([1, 0x3e4, 0x3e5, 0x3e6], facts.buttons),
        41 => authored_choices([1, 0x2f5, 0x2f6, 0x2f7], facts.sticks),
        42 => indexed_from_one(
            0x3cf..0x3d9,
            (if context == 1 {
                facts.controller_sensitivity
            } else {
                facts.keyboard_sensitivity
            })
            .wrapping_sub(1),
        ),
        43 => yes_first(0x308, 0x309, facts.controller_invert),
        44 => authored_choices(
            [0x309, 0x308],
            u32::from(if context == 1 {
                facts.controller_auto_center
            } else {
                facts.keyboard_auto_center
            }),
        ),
        183 => yes_first(0x182, 0x183, facts.vibration),
        242 => authored_choices([1, 0x182, 0x183], facts.subtitles),
        245 => authored_choices([0x309, 0x308], u32::from(facts.dual_wield_invert)),
        253 => binding_editor(action_labels),
        263 => authored_choices(std::iter::once(0x957).chain(0x880..0x88a), facts.volume),
        267 => authored_choices(0x87b..0x880, facts.speakers),
        268 => authored_choices([0x895, 0x896], 1),
        294 => authored_choices([0x309, 0x308], u32::from(facts.keyboard_invert)),
        295 => authored_choices([0x846, 0x847], 1),
        296 => panel(
            "",
            literal("Native LAN uses the operating system's network routing."),
            0,
            [apply(literal("System Default"), 0)],
        ),
        297 => panel("ABOUT HALO 2 NATIVE", literal(""), 0, []),
        _ => unreachable!("unsupported source PC Settings screen {screen}"),
    }
}

pub(crate) fn frontend_pc_settings_value(
    screen: u32,
    index: u32,
    column: u32,
    action: i32,
    capturing_slot: i32,
    facts: &PcSettingsFacts,
) -> SettingsText {
    if screen == 253 {
        return match (action, column) {
            (_, 3) | (-2, _) => literal(""),
            (a, 1) if a < 0 => literal("Unavailable"),
            (a, _) if a < 0 => literal(""),
            _ if capturing_slot == i32::from(column == 2) => literal("Press a key or mouse button"),
            _ => referenced(
                Source::PcBinding,
                if column == 2 {
                    facts.secondary_binding
                } else {
                    facts.primary_binding
                },
            ),
        };
    }
    layout(screen)
        .and_then(|rows| rows.get(index as usize))
        .map_or_else(|| literal(""), |row| row.value(facts))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn settings_links_preserve_source_banks_and_pause_aliases() {
        let mut facts = PcSettingsFacts::default();
        let rows = frontend_pc_settings_panel(19, 0, &facts, &[]).rows;
        assert_eq!(
            rows.iter()
                .map(|r| (r.label.key, r.vista_route, r.destination, r.value))
                .collect::<Vec<_>>(),
            [
                (0x197, false, 193, 0),
                (0x198, false, 60, 1),
                (0x820, true, 256, 0),
                (0x81f, true, 257, 0),
                (0x976, true, 296, 0),
                (0x821, true, 297, 0)
            ]
        );
        for (overlay, preset, editor) in [(false, 243, 253), (true, 244, 254)] {
            facts.gameplay_overlay = overlay;
            facts.keyboard_preset = 4;
            let k = frontend_pc_settings_panel(258, 0, &facts, &[]);
            assert_eq!(k.rows[0].destination, preset);
            let p = frontend_pc_settings_panel(243, 0, &facts, &[]);
            assert_eq!(
                (p.selected, p.rows[3].destination, p.rows[3].value),
                (3, editor, 4)
            );
        }
    }

    #[test]
    fn shared_controls_keep_direction_context_and_serialized_values() {
        let facts = PcSettingsFacts {
            controller_sensitivity: 2,
            keyboard_sensitivity: 9,
            keyboard_auto_center: true,
            vibration: true,
            speakers: 4,
            volume: 0,
            subtitles: 2,
            ..Default::default()
        };
        for (context, expected) in [(1, 1), (0, 8)] {
            let p = frontend_pc_settings_panel(42, context, &facts, &[]);
            assert_eq!(p.selected, expected);
            assert_eq!(p.rows[expected as usize].value, expected as i32 + 1);
        }
        for (screen, context, selected, value) in [
            (43, 1, 1, 0),
            (183, 1, 0, 1),
            (44, 0, 1, 1),
            (44, 1, 0, 0),
            (263, 0, 0, 0),
            (267, 0, 4, 4),
            (242, 0, 2, 2),
            (268, 0, 1, 1),
            (295, 0, 1, 1),
        ] {
            let p = frontend_pc_settings_panel(screen, context, &facts, &[]);
            assert_eq!(
                (p.selected, p.rows[selected].value),
                (selected as u32, value)
            );
        }
        assert_eq!(
            frontend_pc_settings_value(257, 0, 1, 0, -1, &facts).key,
            0x957
        );
    }

    #[test]
    fn binding_editor_preserves_headings_unavailable_rows_and_both_slots() {
        let facts = PcSettingsFacts {
            primary_binding: 4,
            secondary_binding: 5,
            ..Default::default()
        };
        let p = frontend_pc_settings_panel(253, 4, &facts, &[0x1e000832]);
        assert_eq!(p.rows.len(), 43);
        assert_eq!(p.selected, 1);
        assert_eq!(p.rows.iter().filter(|r| r.value == -2).count(), 4);
        assert_eq!(
            (p.rows[0].value, p.rows[1].value, p.rows[2].value),
            (-2, 0, -1)
        );
        for (slot, column, action, expected) in [
            (-1, 1, -2, ""),
            (-1, 1, -1, "Unavailable"),
            (-1, 2, -1, ""),
            (0, 1, 0, "Press a key or mouse button"),
            (1, 2, 0, "Press a key or mouse button"),
            (0, 3, 0, ""),
        ] {
            assert_eq!(
                frontend_pc_settings_value(253, 0, column, action, slot, &facts).text,
                expected
            );
        }
        for (slot, column, key) in [(1, 1, 4), (0, 2, 5)] {
            let text = frontend_pc_settings_value(253, 0, column, 0, slot, &facts);
            assert!(matches!(text.source, Source::PcBinding));
            assert_eq!(text.key, key);
        }
    }
}
