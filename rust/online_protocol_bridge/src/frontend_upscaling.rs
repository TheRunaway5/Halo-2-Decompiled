use crate::ffi::{UpscalingPanel, UpscalingPanelFacts, UpscalingPanelRow};

struct PanelText {
    title: &'static str,
    help: &'static str,
    help_when_off: Option<&'static str>,
}

const OVERVIEW: PanelText = PanelText {
    title: "UPSCALING",
    help: "Upscale the 3D world. Menus and HUD stay at native resolution.",
    help_when_off: Some(
        "Upscaling is off. Choose a method to upscale the world. Menus and HUD stay at native resolution.",
    ),
};
const METHOD: PanelText = PanelText {
    title: "UPSCALING METHOD",
    help: "FSR supports a wide range of graphics cards. DLSS requires an NVIDIA RTX graphics card.",
    help_when_off: None,
};
const QUALITY: PanelText = PanelText {
    title: "QUALITY MODE",
    help: "DLAA and Native AA render the world at full resolution. Other modes trade detail for performance.",
    help_when_off: Some("Choose a quality mode, then select an upscaling method to enable it."),
};

pub(crate) fn frontend_upscaling_panel(context: u32, facts: UpscalingPanelFacts) -> UpscalingPanel {
    let (text, rows) = match context {
        0 => (
            &OVERVIEW,
            [
                ("Method", facts.method_name, 1),
                ("Quality", facts.quality_name, 2),
            ]
            .into_iter()
            .map(|(caption, name, page)| UpscalingPanelRow {
                label: format!("{caption}: {name}"),
                opens_page: true,
                value: page,
            })
            .collect(),
        ),
        page => (
            if page == 1 { &METHOD } else { &QUALITY },
            facts
                .choices
                .into_iter()
                .enumerate()
                .map(|(index, label)| UpscalingPanelRow {
                    label,
                    opens_page: false,
                    value: index as i32,
                })
                .collect(),
        ),
    };
    let help = match (facts.method_enabled, text.help_when_off) {
        (false, Some(help)) => help,
        _ => text.help,
    };
    UpscalingPanel {
        title: text.title.into(),
        description: help.into(),
        rows,
        selected: facts.selected,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn overview_labels_and_destinations_follow_current_values() {
        let panel = frontend_upscaling_panel(
            0,
            UpscalingPanelFacts {
                method_enabled: true,
                selected: 0,
                method_name: "DLSS 4.5".into(),
                quality_name: "DLAA".into(),
                choices: Vec::new(),
            },
        );
        assert_eq!(panel.title, "UPSCALING");
        assert_eq!(panel.description, OVERVIEW.help);
        assert_eq!(panel.rows.len(), 2);
        assert_eq!(panel.rows[0].label, "Method: DLSS 4.5");
        assert_eq!(panel.rows[1].label, "Quality: DLAA");
        assert_eq!(panel.rows[0].value, 1);
        assert_eq!(panel.rows[1].value, 2);
        assert!(panel.rows.iter().all(|row| row.opens_page));
    }

    #[test]
    fn choice_pages_keep_native_order_and_selection() {
        let panel = frontend_upscaling_panel(
            2,
            UpscalingPanelFacts {
                method_enabled: false,
                selected: 2,
                method_name: String::new(),
                quality_name: String::new(),
                choices: ["Native AA", "Ultra Quality", "Quality"]
                    .into_iter()
                    .map(String::from)
                    .collect(),
            },
        );
        assert_eq!(panel.title, "QUALITY MODE");
        assert_eq!(panel.description, QUALITY.help_when_off.unwrap());
        assert_eq!(panel.selected, 2);
        let rows: Vec<_> = panel
            .rows
            .iter()
            .map(|row| (row.label.as_str(), row.opens_page, row.value))
            .collect();
        assert_eq!(
            rows,
            [
                ("Native AA", false, 0),
                ("Ultra Quality", false, 1),
                ("Quality", false, 2),
            ],
        );
    }
}
