use super::*;
use rusqlite::hooks::{AuthAction, AuthContext, Authorization, TransactionOperation};
use std::{
    path::PathBuf,
    sync::{
        Arc,
        atomic::{AtomicUsize, Ordering},
        mpsc,
    },
    time::Duration,
};

struct Fixture {
    runtime: Option<Runtime>,
    root: PathBuf,
    token: String,
}
impl Fixture {
    fn new() -> Self {
        let mut bytes = [0; 8];
        getrandom::getrandom(&mut bytes).unwrap();
        let root = PathBuf::from(
            std::env::var_os("HALO2_TEST_SCRATCH").expect("Run through tools/run_test.py"),
        )
        .join(format!("registration-clock-{}", u64::from_le_bytes(bytes)));
        std::fs::create_dir(&root).unwrap();
        let runtime = Runtime::open(&root.join("registry.sqlite3"), true, None).unwrap();
        let token = format!("h2s_{}", "K".repeat(43));
        runtime
            .db
            .execute(
                "INSERT INTO steam_login_sessions VALUES(?,?,?,?)",
                rusqlite::params![
                    authentication::steam_digest(&token).unwrap(),
                    "76561197960287960",
                    "Clock fixture",
                    database::wall_now().unwrap() + 300
                ],
            )
            .unwrap();
        Self {
            runtime: Some(runtime),
            root,
            token,
        }
    }
    fn rt(&mut self) -> &mut Runtime {
        self.runtime.as_mut().unwrap()
    }
    fn identity(&mut self) -> Identity {
        let token = self.token.clone();
        self.rt().steam(&token).unwrap()
    }
    fn floor(&mut self) -> i64 {
        self.rt()
            .db
            .query_row("SELECT value FROM clock_floor WHERE id=1", [], |r| r.get(0))
            .unwrap()
    }
    fn joined(&mut self) -> i64 {
        self.rt()
            .db
            .query_row("SELECT joined_at FROM player_profiles", [], |r| r.get(0))
            .unwrap()
    }
}
impl Drop for Fixture {
    fn drop(&mut self) {
        self.runtime.take();
        let _ = std::fs::remove_dir_all(&self.root);
    }
}

#[test]
fn login_registration_reads_the_floor_after_the_actual_sql_write_lock_wait() {
    let mut f = Fixture::new();
    let identity = f.identity();
    let floor = database::wall_now().unwrap() + 900;
    let lock = Connection::open(f.root.join("registry.sqlite3")).unwrap();
    lock.execute_batch("BEGIN IMMEDIATE").unwrap();
    let (ready, waiting) = mpsc::channel();
    f.rt()
        .db
        .authorizer(Some(move |ctx: AuthContext<'_>| {
            if matches!(
                ctx.action,
                AuthAction::Transaction {
                    operation: TransactionOperation::Begin
                }
            ) {
                let _ = ready.send(());
            }
            Authorization::Allow
        }))
        .unwrap();
    let mut runtime = f.runtime.take().unwrap();
    let thread = std::thread::spawn(move || {
        let reply = runtime.login(identity, json!({"screen_name":"Clock Pilot"}));
        (runtime, reply)
    });
    waiting.recv_timeout(Duration::from_secs(2)).unwrap();
    std::thread::sleep(Duration::from_millis(250));
    lock.execute("UPDATE clock_floor SET value=? WHERE id=1", [floor])
        .unwrap();
    lock.execute_batch("COMMIT").unwrap();
    let (runtime, reply) = thread.join().unwrap();
    f.runtime = Some(runtime);
    assert!(reply.is_ok(), "{reply:?}");
    assert_eq!(f.floor(), floor);
    assert_eq!(f.joined(), floor);
}

#[test]
fn rejected_login_commit_rolls_back_registration_and_retry_publishes_once() {
    let mut f = Fixture::new();
    let identity = f.identity();
    assert_eq!(f.floor(), 0);
    let denied = Arc::new(AtomicUsize::new(0));
    let observed = denied.clone();
    f.rt()
        .db
        .authorizer(Some(move |ctx: AuthContext<'_>| {
            // Pinned rusqlite0.41 decodes COMMIT as Unknown.
            if matches!(
                ctx.action,
                AuthAction::Transaction {
                    operation: TransactionOperation::Unknown
                }
            ) {
                observed.fetch_add(1, Ordering::SeqCst);
                Authorization::Deny
            } else {
                Authorization::Allow
            }
        }))
        .unwrap();
    assert!(
        f.rt()
            .login(identity, json!({"screen_name":"Clock Pilot"}))
            .is_err()
    );
    assert_eq!(denied.load(Ordering::SeqCst), 1);
    f.rt()
        .db
        .authorizer(None::<fn(AuthContext<'_>) -> Authorization>)
        .unwrap();
    assert_eq!(f.floor(), 0);
    assert_eq!(
        f.rt()
            .db
            .query_row("SELECT count(*) FROM players", [], |r| r.get::<_, i64>(0))
            .unwrap(),
        0
    );
    let identity = f.identity();
    let reply = f
        .rt()
        .login(identity, json!({"screen_name":"Clock Pilot"}))
        .unwrap();
    let header = format!("Bearer {}", reply["token"].as_str().unwrap());
    assert!(f.rt().session(&header).is_ok());
    assert_eq!(f.joined(), f.floor());
    assert!(f.floor() > 0);
    assert_eq!(
        f.rt()
            .db
            .query_row("SELECT count(*) FROM players", [], |r| r.get::<_, i64>(0))
            .unwrap(),
        1
    );
}
