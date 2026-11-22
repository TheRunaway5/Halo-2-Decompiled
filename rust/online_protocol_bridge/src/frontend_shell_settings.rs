//! Profile and game-variant layouts in the original menu's screen namespace.
//! Dynamic sections bind to the same native profile, variant and tag stores.

use crate::ffi::{ShellSettingsElement as Element, ShellSettingsPart as Part};

const APPLY: i32 = -2;

fn element(part: Part, key: u32, destination: i32, value: i32) -> Element {
    Element {
        part,
        key,
        destination,
        value,
    }
}

fn row(label: u32, destination: i32, value: i32) -> Element {
    element(Part::LocalizedRow, label, destination, value)
}

fn links(labels_and_screens: &[(u32, i32)]) -> Vec<Element> {
    labels_and_screens
        .iter()
        .map(|&(label, screen)| row(label, screen, 0))
        .collect()
}

fn choices(labels: &[u32]) -> Vec<Element> {
    labels
        .iter()
        .enumerate()
        .map(|(value, &label)| row(label, APPLY, value as i32))
        .collect()
}

fn consecutive_choices(first: u32, last: u32) -> Vec<Element> {
    (first..=last)
        .map(|label| row(label, APPLY, (label - first) as i32))
        .collect()
}

// Indexed by the stored native engine enum. Each triple indexes the authored
// match, team and game category tables, respectively.
fn variant_categories(engine: u32) -> Result<[u32; 3], &'static str> {
    [
        [1, 32, 14],
        [2, 33, 15],
        [6, 37, 19],
        [3, 34, 16],
        [0, 31, 13],
        [8, 39, 21],
        [7, 38, 20],
    ]
    .get(engine as usize)
    .copied()
    .ok_or("Invalid engine for game variant settings")
}

fn category_heading(category: u32) -> u32 {
    0x189e
        + match category {
            9 => 1,
            11 => 4,
            12 => 5,
            31.. => 2,
            13.. => 3,
            _ => 0,
        }
}

/// A layout can be used from Settings or from another owner of the same
/// screen (such as the session lobby). An empty layout leaves unrelated
/// screens with their original builder.
pub(crate) fn frontend_shell_settings_layout(
    screen: u32,
    context: u32,
    engine: u32,
    custom_category: u32,
) -> Result<Vec<Element>, &'static str> {
    Ok(match screen {
        19 => vec![
            row(0x196, 193, 0),
            row(0x197, 60, 1),
            element(Part::PcSettingsLink, 0x0e000820, 256, 0),
        ],
        10 | 193 => vec![
            element(Part::GuestProfile, 0, -80, 0),
            row(0x195, 167, -1),
            element(Part::Profiles, 0, -3, 0),
        ],
        194 => {
            let (edit, rename) = if context == 0 { (38, 5) } else { (178, 6) };
            vec![
                row(0x19e, edit, 0),
                row(0x19f, 167, rename),
                row(0x1a0, -12, context as i32),
            ]
        }
        38 => {
            let mut rows = links(&[
                (0x2ef, 39),
                (0x2f0, 47),
                (0x302, 55),
                (0x301, 54),
                (0x2f2, 58),
                (0x2f1, 242),
            ]);
            rows.push(element(Part::PcSettingsLink, 0x16000853, 258, 0));
            rows
        }
        39 => links(&[
            (0x3dd, 41),
            (0x3de, 40),
            (0x3df, 42),
            (0x3e0, 43),
            (0x3e1, 44),
            (0x3e2, 183),
        ]),
        // Original 2C647D admits four records; the saved enum indexes them.
        40 => choices(&[1, 0x3e4, 0x3e5, 0x3e6]),
        41 => choices(&[1, 0x2f5, 0x2f6, 0x2f7]),
        42 => consecutive_choices(0x3cf, 0x3d8),
        43 | 44 => vec![row(0x308, APPLY, 1), row(0x309, APPLY, 0)],
        45 => consecutive_choices(0x3d9, 0x3dc),
        47 => vec![
            row(0x2f8, 52, 0),
            row(0x2f9, 50, 0),
            row(0x2fa, 50, 1),
            row(0x2fb, 50, 2),
            row(0x2fc, 50, 3),
            row(0x2fd, 49, 0),
            row(0x2fe, 233, 0),
        ],
        48 => links(&[(0x146, -5), (0x147, -6)]),
        49 => consecutive_choices(0x30a, 0x349),
        50 => consecutive_choices(0x30a, 0x31b),
        52 => choices(&[0x2ff, 0x300]),
        53 => links(&[(0x301, 54), (0x302, 55)]),
        54 => choices(&[0x21c, 0x303]),
        55 => choices(&[0x304, 0x306, 0x305, 0x307]),
        58 => vec![row(0x183, APPLY, 1), row(0x184, APPLY, 0)],
        183 | 241 => vec![row(0x181, APPLY, 1), row(0x182, APPLY, 0)],
        233 => consecutive_choices(0x30a, 0x329),
        239 => consecutive_choices(0x6f6, 0x704),
        242 => choices(&[1, 0x181, 0x182]),
        60 | 207 => {
            let picker = if screen == 60 { 15 } else { 208 };
            [
                (0x10f, 0),
                (0x237, 3),
                (0x110, 1),
                (0x113, 2),
                (0x439, 4),
                (0x10e, 5),
                (0x115, 6),
            ]
            .into_iter()
            .map(|(label, engine)| row(label, picker, engine))
            .collect()
        }
        15 | 208 => vec![
            element(Part::BuiltinVariants, 0, -7, 0),
            element(Part::SavedVariants, 0, -9, 0),
        ],
        178 => {
            let [game_match, team, game] = variant_categories(engine)?;
            let mut sections: Vec<_> = [
                (0x434, game_match),
                (0x430, 9),
                (0x435, team),
                (0x42e, game),
                (0x433, 11),
                (0x436, 12),
            ]
            .into_iter()
            .map(|(label, category)| element(Part::VariantCategory, label, 221, category as i32))
            .collect();
            sections.push(element(Part::VariantDescription, 0, 0, 0));
            sections.push(element(
                Part::CustomVariantCategory,
                0,
                221,
                custom_category as i32,
            ));
            sections
        }
        221 | 222 => {
            let category = if screen == 222 {
                22 + variant_categories(engine)?[0]
            } else {
                context
            };
            let mut sections = Vec::new();
            if category == custom_category {
                sections.push(element(Part::CustomParameters, category, 223, 0));
            } else {
                sections.push(element(Part::Parameters, category, 223, 0));
                if screen == 221 {
                    sections.push(element(
                        Part::CategoryTitle,
                        category_heading(category),
                        0,
                        0,
                    ));
                }
            }
            sections.push(element(Part::VariantDescription, 0, 0, 0));
            sections
        }
        223 => vec![element(Part::ParameterChoices, context, APPLY, 0)],
        _ => Vec::new(),
    })
}

// Original 2C8BF5 populates each row's second text child from the working
// profile, including the embedded color and emblem glyphs in these strings.
pub(crate) fn frontend_appearance_readout(
    destination: i32,
    slot: i32,
    colors: &[i8],
    species: i8,
    foreground: u8,
    background: u8,
) -> u32 {
    match destination {
        52 if (0..2).contains(&species) => 0x2ff + species as u32,
        50 => match colors.get(slot as usize) {
            Some(&color) if (0..18).contains(&color) => 0x30a + color as u32,
            _ => 0,
        },
        49 if foreground < 64 => 0x36f + u32::from(foreground),
        233 if background < 32 => 0x3af + u32::from(background),
        _ => 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn appearance_readouts_follow_their_rows_and_reject_invalid_colors() {
        let colors = [10, -1, 18, 7];
        let appearance = frontend_shell_settings_layout(47, 0, 0, 0).unwrap();
        let readouts: Vec<_> = appearance
            .iter()
            .map(|row| frontend_appearance_readout(row.destination, row.value, &colors, 1, 63, 31))
            .collect();
        assert_eq!(readouts, [0x300, 0x314, 0, 0, 0x311, 0x3ae, 0x3ce]);
    }

    #[test]
    fn original_profile_actions_keep_their_saved_enum_order() {
        let layout = frontend_shell_settings_layout(55, 0, 0, 0).unwrap();
        let actions: Vec<_> = layout
            .iter()
            .map(|item| (item.key, item.destination, item.value))
            .collect();
        assert_eq!(
            actions,
            [
                (0x304, APPLY, 0),
                (0x306, APPLY, 1),
                (0x305, APPLY, 2),
                (0x307, APPLY, 3)
            ]
        );
        assert_eq!(
            frontend_shell_settings_layout(49, 0, 0, 0).unwrap().len(),
            64
        );
    }

    #[test]
    fn variant_layout_preserves_engine_categories_and_opaque_datums() {
        let layout = frontend_shell_settings_layout(178, 0, 2, 1234).unwrap();
        assert_eq!(layout[0].value, 6);
        assert_eq!(layout[2].value, 37);
        assert_eq!(layout[3].value, 19);
        assert_eq!(layout.last().unwrap().value, 1234);
        assert_eq!(
            frontend_shell_settings_layout(222, 0, 2, 1234).unwrap()[0].key,
            28
        );
        assert_eq!(
            frontend_shell_settings_layout(223, 0xe0020123, 0, 1234).unwrap()[0].key,
            0xe0020123
        );
        assert!(frontend_shell_settings_layout(178, 0, 7, 1234).is_err());
    }
}
