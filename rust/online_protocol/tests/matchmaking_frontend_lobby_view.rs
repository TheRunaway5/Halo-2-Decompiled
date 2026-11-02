use halo2_online_protocol::matchmaking_frontend_lobby_view::*;
use std::cell::RefCell;
use std::collections::BTreeMap;

#[derive(Default)]
struct Probe {
    facts: QuickFacts,
    defaults: BTreeMap<(u32, u32), Option<i32>>,
    calls: RefCell<Vec<String>>,
    fail: Option<usize>,
}
impl Probe {
    fn observe(&self, call: String) -> Result<()> {
        let mut calls = self.calls.borrow_mut();
        calls.push(call);
        if self.fail == Some(calls.len() - 1) {
            Err("native failure".into())
        } else {
            Ok(())
        }
    }
}
impl Io for Probe {
    fn category(&self, id: u32) -> Result<Option<Vec<u32>>> {
        self.observe(format!("category:{id}"))?;
        Ok(self.facts.categories.get(&id).cloned())
    }
    fn parameter(&self, tag: u32) -> Result<Option<Parameter>> {
        self.observe(format!("parameter:{tag}"))?;
        Ok(self.facts.parameters.get(&tag).cloned())
    }
    fn value(&self, id: u32) -> Result<Option<i32>> {
        self.observe(format!("value:{id}"))?;
        Ok(self.facts.values.get(&id).copied())
    }
    fn default_value(&self, engine: u32, id: u32) -> Result<Option<i32>> {
        self.observe(format!("default:{engine}:{id}"))?;
        Ok(self.defaults.get(&(engine, id)).copied().flatten())
    }
}
fn populated() -> Probe {
    let mut p = Probe::default();
    p.facts.categories.insert(23, vec![10, 10]);
    p.facts.parameters.insert(
        10,
        Parameter {
            parameter: 5,
            title: vec![0xff, 0],
            choices: vec![
                Choice {
                    label: vec![0x80, 0],
                    value: -1,
                },
                Choice {
                    label: b"wrong duplicate".to_vec(),
                    value: -1,
                },
            ],
        },
    );
    p.defaults.insert((0, 5), Some(-1));
    p
}
#[test]
fn native_defaults_and_unknown_panes_are_retained() {
    assert_eq!(session(&Session::default()), -1);
    for previous in [i32::MIN, -1, 3, i32::MAX] {
        let p = presentation(&Input {
            previous,
            update_guard: true,
            ..Input::default()
        });
        assert_eq!(p.pane, previous);
        assert_eq!(p.buttons, None);
        assert_eq!(p.status, None);
        assert_eq!(p.privacy_text, None);
        assert_eq!(p.prior_stats_visible, None);
    }
}
#[test]
fn known_empty_bindings_are_distinct_from_untouched_widgets() {
    let p = presentation(&Input {
        previous: 2,
        privacy: i32::MIN,
        ..Input::default()
    });
    assert_eq!(p.status, Some(0));
    assert_eq!(p.privacy_visible, Some(true));
    assert_eq!(p.privacy, None);
    assert_eq!(p.buttons, Some([false, false]));
    assert_eq!(p.start_eligible, Some(false));
}
#[test]
fn local_team_selection_and_team_mode_have_distinct_owners() {
    let f = Input {
        host: true,
        team_mode: true,
        start_permission: true,
        ..Input::default()
    };
    assert_eq!(presentation(&f).start_eligible, Some(true));
    assert_eq!(
        presentation(&Input {
            local_team_selection: true,
            ..f
        })
        .start_eligible,
        Some(false)
    );
}
#[test]
fn signed_query_boundaries_remain_full_width() {
    for prior_stats in [i32::MIN, -1, 0, 1, i32::MAX] {
        assert_eq!(
            presentation(&Input {
                prior_stats,
                ..Input::default()
            })
            .prior_stats_visible,
            Some(prior_stats > 0)
        );
    }
    assert_eq!(player_count(-17, i32::MIN, i32::MIN), -17);
    assert_eq!(player_count(3, i32::MAX, i32::MIN), 16);
    assert_eq!(difficulty(i16::MIN), 0x060000b8);
    assert_eq!(difficulty(i16::MAX), 0x060000b8);
    assert!(help(2, 2, i32::MIN, 0, 6).visible);
    assert_eq!(help(2, 2, i32::MAX, 0, 6).value, None);
}
#[test]
fn unsupported_engines_do_not_read_cache_or_defaults() {
    let p = Probe::default();
    for engine in [7, 256, u32::MAX] {
        assert_eq!(quick_options(engine, b": ", 6, &p).unwrap(), vec![None; 6]);
    }
    assert!(p.calls.borrow().is_empty());
}
#[test]
fn guarded_default_calls_never_reach_native_gameplay() {
    let p = Probe::default();
    assert_eq!(default_value(7, 0, &p).unwrap(), None);
    assert_eq!(default_value(0, 112, &p).unwrap(), None);
    assert_eq!(default_value(u32::MAX, u32::MAX, &p).unwrap(), None);
    assert!(p.calls.borrow().is_empty());
}
#[test]
fn holes_keep_their_authored_row_and_empty_titles_skip_values() {
    let mut p = populated();
    p.facts.categories.insert(23, vec![404, 11, 10]);
    p.facts.parameters.insert(11, Parameter::default());
    assert_eq!(quick_options(0, b":", 2, &p).unwrap(), vec![None, None]);
    assert_eq!(
        *p.calls.borrow(),
        ["category:23", "parameter:404", "parameter:11"]
    );
}
#[test]
fn duplicates_repeat_native_getters_and_choose_first_label_with_opaque_bytes() {
    let p = populated();
    assert_eq!(
        quick_options(0, &[0xfe, 0], 2, &p).unwrap(),
        vec![Some(vec![0xff, 0, 0xfe, 0, 0x80, 0]); 2]
    );
    assert_eq!(
        *p.calls.borrow(),
        [
            "category:23",
            "parameter:10",
            "value:5",
            "default:0:5",
            "parameter:10",
            "value:5",
            "default:0:5"
        ]
    );
}
#[test]
fn current_override_precedes_the_default_parameter_guard() {
    let mut p = populated();
    p.facts.parameters.get_mut(&10).unwrap().parameter = u32::MAX;
    p.facts.values.insert(u32::MAX, -1);
    assert_eq!(
        quick_options(0, b"", 1, &p).unwrap(),
        vec![Some(vec![0xff, 0, 0x80, 0])]
    );
    assert_eq!(
        *p.calls.borrow(),
        ["category:23", "parameter:10", "value:4294967295"]
    );
}
#[test]
fn unknown_choice_keeps_title_and_separator() {
    let mut p = populated();
    p.facts.values.insert(5, i32::MIN);
    assert_eq!(
        quick_options(0, b": ", 1, &p).unwrap(),
        vec![Some(vec![0xff, 0, b':', b' '])]
    );
}
#[test]
fn missing_default_keeps_an_empty_authored_row() {
    let mut p = populated();
    p.defaults.clear();
    assert_eq!(quick_options(0, b": ", 1, &p).unwrap(), vec![None]);
}
#[test]
fn errors_stop_at_the_failing_native_observation() {
    for fail in 0..7 {
        let mut p = populated();
        p.fail = Some(fail);
        assert_eq!(quick_options(0, b": ", 2, &p), Err("native failure".into()));
        assert_eq!(p.calls.borrow().len(), fail + 1);
    }
}
