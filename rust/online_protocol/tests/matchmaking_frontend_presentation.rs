use halo2_online_protocol::matchmaking_frontend_presentation::*;
use std::cell::RefCell;
#[derive(Default)]
struct Native {
    calls: RefCell<Vec<String>>,
    fail: Option<&'static str>,
    count: u64,
}
impl Native {
    fn read(&self, call: String, value: Vec<u8>) -> Result<Vec<u8>> {
        self.calls.borrow_mut().push(call.clone());
        if self.fail.is_some_and(|s| call.starts_with(s)) {
            Err(call)
        } else {
            Ok(value)
        }
    }
}
impl Io for Native {
    fn original_text(&self, s: u32, p: u32, i: u32) -> Result<Vec<u8>> {
        self.read(
            format!("text:{s}:{p}:{i}"),
            b"\xee\x90\xa5 \xee\x91\x9b \xee\x90\x8a \xee\x90\x88".to_vec(),
        )
    }
    fn original_string(&self, s: u32, id: u32) -> Result<Vec<u8>> {
        self.read(format!("string:{s}:{id:x}"), b"needed %d".to_vec())
    }
    fn pc_string(&self, id: u32) -> Result<Vec<u8>> {
        self.read(format!("pc:{id:x}"), b"start \xee\x91\xb8".to_vec())
    }
    fn pc_authored(&self, p: u32) -> Result<Vec<u8>> {
        self.read(format!("authored:{p}"), b"authored".to_vec())
    }
    fn pc_match_line(&self) -> Result<Vec<u8>> {
        self.read("match-line".into(), b"variant on map".to_vec())
    }
    fn map_name(&self) -> Result<Vec<u8>> {
        self.read("map".into(), b"map".to_vec())
    }
    fn quick_option(&self, i: u32) -> Result<Vec<u8>> {
        self.read(format!("option:{i}"), b"option".to_vec())
    }
    fn party_count(&self) -> Result<u64> {
        self.read("party".into(), Vec::new())?;
        Ok(self.count)
    }
    fn expand(&self, v: &[u8]) -> Result<Vec<u8>> {
        self.read("expand".into(), v.to_vec())
    }
}
#[test]
fn retained_source_reads_and_first_native_error_precede_text_override() {
    let f = Facts {
        title: 3,
        ..Facts::default()
    };
    let io = Native {
        fail: Some("party"),
        ..Native::default()
    };
    assert_eq!(original_text(&f, 0, 0, &io).unwrap_err(), "party");
    assert_eq!(*io.calls.borrow(), ["text:210:0:0", "party"]);
    let io = Native::default();
    original_text(&f, 0, 0, &io).unwrap();
    assert_eq!(
        *io.calls.borrow(),
        ["text:210:0:0", "party", "string:210:1300043d", "expand"]
    );
}
#[test]
fn raw_native_bytes_and_first_duplicate_id_selection_are_preserved() {
    let f = Facts {
        screen: 219,
        selected_row: 1,
        playlists: vec![
            Playlist {
                id: b"same".to_vec(),
                name: vec![0xff, 0, 0x80],
                ..Playlist::default()
            },
            Playlist {
                id: b"same".to_vec(),
                name: b"second".to_vec(),
                ..Playlist::default()
            },
        ],
        ..Facts::default()
    };
    assert_eq!(text(&f, 1, &Native::default()).unwrap(), [0xff, 0, 0x80]);
}
#[test]
fn source_counts_and_needed_visibility_do_not_use_global_queue_population() {
    let mut f = Facts {
        matched: Some(4),
        gathering: Some(2),
        minimum: 5,
        queued: 999,
        ..Facts::default()
    };
    let io = Native {
        count: 777,
        ..Native::default()
    };
    assert_eq!(original_text(&f, 1, 3, &io).unwrap(), b"needed 1");
    assert!(!io.calls.borrow().iter().any(|c| c == "party"));
    f.gathering = None;
    assert!(original_text(&f, 1, 3, &io).unwrap().is_empty());
}
#[test]
fn pc_header_uses_published_none_zero_and_original_title_separately() {
    let mut f = Facts {
        screen: 210,
        pc_progress: true,
        matched: Some(8),
        pane: 3,
        ui: 7,
        countdown: 0,
        ..Facts::default()
    };
    assert_eq!(
        text(&f, 0, &Native::default()).unwrap(),
        b"start :00\nvariant on map"
    );
    f.countdown = -1;
    let io = Native::default();
    text(&f, 0, &io).unwrap();
    assert_eq!(*io.calls.borrow(), ["pc:d00043e", "match-line"]);
    f.ui = -1;
    assert_eq!(text(&f, 0, &Native::default()).unwrap(), b"variant on map");
}
#[test]
fn unavailable_clan_details_remain_explicit_without_inventing_clan_team_names() {
    let mut f = Facts {
        screen: 219,
        playlists: vec![Playlist {
            id: b"clan".to_vec(),
            description: b"desc".to_vec(),
            clan: true,
            ..Playlist::default()
        }],
        ..Facts::default()
    };
    assert_eq!(
        text(&f, 2, &Native::default()).unwrap(),
        b"desc\n\nClan membership required; unavailable for this party."
    );
    f.matched = Some(8);
    assert!(
        original_text(&f, 3, 11, &Native::default())
            .unwrap()
            .is_empty()
    );
}
#[test]
fn invalid_full_width_status_does_not_alias_searching_or_ready() {
    let f = Facts {
        screen: 210,
        status: 261,
        pc_progress: true,
        party: true,
        party_message: b"party".to_vec(),
        message: b"queue".to_vec(),
        ..Facts::default()
    };
    assert_eq!(text(&f, 0, &Native::default()).unwrap(), b"queue");
    assert!(
        text(&f, 5, &Native::default())
            .unwrap()
            .starts_with(b"queue\n\nVoice:")
    );
}
