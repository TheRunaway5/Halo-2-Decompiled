//! Declared socket/scheduler observations at the complete Rust worker boundary.
//! These controls do not supply original XNet measurements or match authority.
use halo2_online_protocol::{matchmaking_native_qos::*, matchmaking_origin::UrlParts};
use std::{
    cell::{Cell, RefCell},
    collections::VecDeque,
};

fn parts() -> UrlParts {
    UrlParts {
        parsed: true,
        normalized: "https://queue.example/".into(),
        scheme: "https".into(),
        host: "queue.example".into(),
        path: "/".into(),
        ..Default::default()
    }
}
fn worker() -> Worker {
    Worker::new(
        true,
        "https://queue.example",
        &parts(),
        &"s".repeat(43),
        false,
    )
    .unwrap()
}
fn text(value: impl Into<Vec<u8>>) -> Receive {
    Receive::Frame(Frame {
        text: true,
        data: value.into(),
        ..Default::default()
    })
}
fn challenge(key: &str) -> Vec<u8> {
    format!("{{\"{key}\":\"{}\"}}", "a".repeat(32)).into_bytes()
}

enum Write {
    Again,
    Bytes(usize),
    Fail,
}
struct Socket {
    now: Cell<u64>,
    cancelled: Cell<bool>,
    pending: RefCell<VecDeque<VecDeque<Receive>>>,
    current: RefCell<VecDeque<Receive>>,
    opened: RefCell<Vec<u64>>,
    closed: RefCell<Vec<u64>>,
    requests: RefCell<Vec<(String, String, u32, u32)>>,
    writes: RefCell<Vec<(SendKind, Vec<u8>)>>,
    results: RefCell<VecDeque<Write>>,
    forever_again: bool,
    cancel_at: Option<u64>,
}
impl Socket {
    fn new(connections: Vec<Vec<Receive>>) -> Self {
        Self {
            now: Cell::new(0),
            cancelled: Cell::new(false),
            pending: RefCell::new(connections.into_iter().map(VecDeque::from).collect()),
            current: RefCell::new(VecDeque::new()),
            opened: RefCell::new(vec![]),
            closed: RefCell::new(vec![]),
            requests: RefCell::new(vec![]),
            writes: RefCell::new(vec![]),
            results: RefCell::new(VecDeque::new()),
            forever_again: false,
            cancel_at: None,
        }
    }
}
impl Io for Socket {
    fn now_ms(&self) -> u64 {
        self.now.get()
    }
    fn cancelled(&self) -> bool {
        self.cancelled.get()
    }
    fn wait(&self, milliseconds: u32) {
        self.now.set(self.now.get() + u64::from(milliseconds));
        if self.cancel_at.is_some_and(|v| self.now.get() >= v) {
            self.cancelled.set(true);
        }
    }
    fn open(&self, o: Open<'_>) -> Result<()> {
        let next = self.pending.borrow_mut().pop_front();
        match next {
            Some(events) => {
                self.opened.borrow_mut().push(self.now.get());
                *self.current.borrow_mut() = events;
                self.requests.borrow_mut().push((
                    o.url.into(),
                    o.bearer.into(),
                    o.connect_ms,
                    o.timeout_ms,
                ));
                Ok(())
            }
            None => {
                self.cancelled.set(true);
                Err(Error::Cancelled)
            }
        }
    }
    fn receive(&self) -> Result<Receive> {
        if let Some(v) = self.current.borrow_mut().pop_front() {
            return Ok(v);
        }
        if !self.forever_again {
            self.cancelled.set(true);
        }
        Ok(Receive::Again)
    }
    fn send(&self, kind: SendKind, data: &[u8]) -> Result<Sent> {
        self.writes.borrow_mut().push((kind, data.into()));
        match self.results.borrow_mut().pop_front() {
            Some(Write::Again) => Ok(Sent {
                again: true,
                count: 0,
            }),
            Some(Write::Bytes(count)) => Ok(Sent {
                again: false,
                count,
            }),
            Some(Write::Fail) => Err(Error::Io),
            None => Ok(Sent {
                again: false,
                count: data.len(),
            }),
        }
    }
    fn close(&self) {
        self.closed.borrow_mut().push(self.now.get());
    }
}

#[test]
fn pinned_origin_bearer_and_native_connection_budgets() {
    let s = Socket::new(vec![vec![text(challenge("ping"))]]);
    worker().run(&s);
    assert_eq!(
        s.requests.borrow().as_slice(),
        &[(
            "wss://queue.example/v1/matchmaking/qos".into(),
            "s".repeat(43),
            2000,
            3000
        )]
    );
    let mut p = parts();
    p.scheme = "http".into();
    p.host = "127.0.0.1".into();
    p.normalized = "http://127.0.0.1:1234/".into();
    assert!(Worker::new(true, "http://127.0.0.1:1234", &p, &"s".repeat(32), false).is_err());
    assert!(Worker::new(true, "http://127.0.0.1:1234", &p, &"s".repeat(128), true).is_ok());
    p.host = "queue.example".into();
    assert!(Worker::new(true, "http://queue.example", &p, &"s".repeat(43), true).is_err());
    for token in [
        "short".into(),
        "s".repeat(129),
        "s".repeat(43) + "\r\nX: forged",
        "é".repeat(32),
    ] {
        assert!(Worker::new(true, "https://queue.example", &parts(), &token, false).is_err());
    }
    for raw in [
        "https://user@queue.example",
        "https://queue.example?x",
        "https://queue.example#x",
        "https://queue.example\n",
        "",
    ] {
        assert!(Worker::new(true, raw, &parts(), &"s".repeat(43), false).is_err());
    }
    assert!(
        Worker::new(
            false,
            "https://queue.example",
            &parts(),
            &"s".repeat(43),
            false
        )
        .is_err()
    );
    let mut p = parts();
    p.path = "/path".into();
    assert!(
        Worker::new(
            true,
            "https://queue.example/path",
            &p,
            &"s".repeat(43),
            false
        )
        .is_err()
    );
}

#[test]
fn only_echoes_the_exact_service_nonce_and_never_claims_measurements() {
    let s = Socket::new(vec![vec![
        text(challenge("ping")),
        text(challenge("probe")),
    ]]);
    worker().run(&s);
    assert_eq!(
        s.writes.borrow().as_slice(),
        &[
            (SendKind::Text, challenge("pong")),
            (SendKind::Text, challenge("echo"))
        ]
    );
    assert_eq!(s.opened.borrow().len(), 1);
    assert_eq!(s.closed.borrow().len(), 1);
}

#[test]
fn rejects_duplicate_extra_unknown_nested_and_invalid_nonce_controls() {
    let a = "a".repeat(32);
    let b = "b".repeat(32);
    let bodies = vec![
        format!("{{\"probe\":\"{a}\",\"probe\":\"{b}\"}}"),
        format!("{{\"probe\":\"{a}\",\"rank\":127}}"),
        format!("{{\"ping\":\"{a}\",\"probe\":\"{b}\"}}"),
        format!("{{\"score\":\"{a}\"}}"),
        "{\"ping\":{\"nonce\":1}}".into(),
        "{\"ping\":1}".into(),
        "{\"ping\":null}".into(),
        "[]".into(),
        format!("{{\"probe\":\"{}\"}}", "A".repeat(32)),
        format!("{{\"probe\":\"{}\"}}", "a".repeat(31)),
        format!("{{\"probe\":\"{}\"}}", "a".repeat(33)),
    ];
    for body in bodies {
        let s = Socket::new(vec![vec![text(body.into_bytes())]]);
        worker().run(&s);
        assert!(s.writes.borrow().is_empty());
    }
    let s = Socket::new(vec![vec![text(vec![255])]]);
    worker().run(&s);
    assert!(s.writes.borrow().is_empty());
}

#[test]
fn text_chunks_and_fragments_survive_interleaved_control_frames() {
    for continued in [false, true] {
        let body = challenge("probe");
        let cut = 10;
        let s = Socket::new(vec![vec![
            Receive::Frame(Frame {
                text: true,
                data: body[..cut].to_vec(),
                continued,
                bytes_left: if continued {
                    0
                } else {
                    (body.len() - cut) as i64
                },
                ..Default::default()
            }),
            Receive::Frame(Frame {
                ping: true,
                data: vec![1, 2, 255],
                ..Default::default()
            }),
            text(body[cut..].to_vec()),
        ]]);
        worker().run(&s);
        assert_eq!(
            s.writes.borrow().as_slice(),
            &[
                (SendKind::Pong, vec![1, 2, 255]),
                (SendKind::Text, challenge("echo"))
            ]
        );
    }
}

#[test]
fn total_text_limit_is_exact_and_applies_across_chunks() {
    let mut exact = challenge("ping");
    exact.resize(256, b' ');
    let s = Socket::new(vec![vec![text(exact.clone())]]);
    worker().run(&s);
    assert_eq!(s.writes.borrow().len(), 1);
    exact.push(b' ');
    let s = Socket::new(vec![vec![text(exact)]]);
    worker().run(&s);
    assert!(s.writes.borrow().is_empty());
    let s = Socket::new(vec![vec![
        Receive::Frame(Frame {
            text: true,
            continued: true,
            data: vec![b' '; 200],
            ..Default::default()
        }),
        text(vec![b' '; 57]),
    ]]);
    worker().run(&s);
    assert!(s.writes.borrow().is_empty());
}

#[test]
fn ping_and_pong_payloads_are_opaque_and_bounded() {
    for length in [0, 1, 125] {
        let s = Socket::new(vec![vec![
            Receive::Frame(Frame {
                ping: true,
                data: vec![255; length],
                ..Default::default()
            }),
            Receive::Frame(Frame {
                pong: true,
                data: vec![255; length],
                ..Default::default()
            }),
        ]]);
        worker().run(&s);
        assert_eq!(
            s.writes.borrow().as_slice(),
            &[(SendKind::Pong, vec![255; length])]
        );
    }
    for frame in [
        Frame {
            ping: true,
            data: vec![0; 126],
            ..Default::default()
        },
        Frame {
            ping: true,
            bytes_left: 1,
            ..Default::default()
        },
        Frame {
            pong: true,
            continued: true,
            ..Default::default()
        },
        Frame {
            ping: true,
            text: true,
            ..Default::default()
        },
        Frame {
            bytes_left: -1,
            text: true,
            ..Default::default()
        },
    ] {
        let s = Socket::new(vec![vec![Receive::Frame(frame)]]);
        worker().run(&s);
        assert!(s.writes.borrow().is_empty());
    }
}

#[test]
fn invalid_binary_and_close_frames_retire_only_the_current_connection() {
    for frame in [
        Frame {
            binary: true,
            data: challenge("ping"),
            ..Default::default()
        },
        Frame {
            close: true,
            ..Default::default()
        },
        Frame::default(),
    ] {
        let s = Socket::new(vec![
            vec![Receive::Frame(frame)],
            vec![text(challenge("ping"))],
        ]);
        worker().run(&s);
        assert_eq!(s.opened.borrow().as_slice(), &[0, 1000]);
        assert_eq!(
            s.writes.borrow().as_slice(),
            &[(SendKind::Text, challenge("pong"))]
        );
    }
}

#[test]
fn partial_send_uses_remaining_bytes_and_preserves_message_type() {
    let s = Socket::new(vec![vec![text(challenge("ping"))]]);
    *s.results.borrow_mut() = VecDeque::from([Write::Again, Write::Bytes(7)]);
    worker().run(&s);
    let out = challenge("pong");
    assert_eq!(
        s.writes.borrow().as_slice(),
        &[
            (SendKind::Text, out.clone()),
            (SendKind::Text, out.clone()),
            (SendKind::Text, out[7..].to_vec())
        ]
    );
}

#[test]
fn zero_payload_again_is_not_a_completed_pong() {
    let s = Socket::new(vec![vec![Receive::Frame(Frame {
        ping: true,
        ..Default::default()
    })]]);
    s.results.borrow_mut().push_back(Write::Again);
    worker().run(&s);
    assert_eq!(
        s.writes.borrow().as_slice(),
        &[(SendKind::Pong, vec![]), (SendKind::Pong, vec![])],
        "AGAIN acknowledged an unsent empty Pong"
    );
}

#[test]
fn send_deadline_cannot_spin_forever_and_io_errors_reconnect() {
    let s = Socket::new(vec![
        vec![text(challenge("ping"))],
        vec![text(challenge("probe"))],
    ]);
    *s.results.borrow_mut() = (0..300).map(|_| Write::Again).collect();
    worker().run(&s);
    assert_eq!(s.closed.borrow()[0], 500);
    assert_eq!(s.opened.borrow()[1], 1500);
    let s = Socket::new(vec![
        vec![text(challenge("ping"))],
        vec![text(challenge("probe"))],
    ]);
    s.results.borrow_mut().push_back(Write::Fail);
    worker().run(&s);
    assert_eq!(s.opened.borrow().as_slice(), &[0, 1000]);
    assert_eq!(s.writes.borrow().len(), 2);
}

#[test]
fn idle_deadline_closes_without_inventing_client_challenges() {
    let mut s = Socket::new(vec![vec![]]);
    s.forever_again = true;
    s.cancel_at = Some(21_000);
    worker().run(&s);
    assert_eq!(s.closed.borrow().as_slice(), &[20_000]);
    assert!(s.writes.borrow().is_empty());
}

#[test]
fn cancellation_interrupts_retry_and_stalled_send() {
    let mut s = Socket::new(vec![vec![Receive::Frame(Frame {
        close: true,
        ..Default::default()
    })]]);
    s.cancel_at = Some(10);
    worker().run(&s);
    assert_eq!(s.opened.borrow().len(), 1);
    assert_eq!(s.now.get(), 10);
    let mut s = Socket::new(vec![vec![text(challenge("ping"))]]);
    s.cancel_at = Some(4);
    *s.results.borrow_mut() = VecDeque::from([Write::Again, Write::Again, Write::Again]);
    worker().run(&s);
    assert_eq!(s.writes.borrow().len(), 2);
    assert_eq!(s.now.get(), 4);
}

#[test]
fn impossible_send_progress_is_not_a_successful_echo() {
    let s = Socket::new(vec![
        vec![text(challenge("ping"))],
        vec![text(challenge("probe"))],
    ]);
    s.results.borrow_mut().push_back(Write::Bytes(1000));
    worker().run(&s);
    assert_eq!(s.opened.borrow().as_slice(), &[0, 1000]);
}
