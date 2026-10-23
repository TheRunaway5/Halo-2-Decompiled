//! Account-bound EOS Connect admission. HTTP and SDK calls are caller-owned I/O.
//!
//! An issuer credential is consumed once. The SDK identity becomes immutable,
//! and no networking admission is granted until the authenticated master has
//! acknowledged the SDK identity proof. There is no anonymous-login action.

use std::fmt;

const INITIAL_TIMEOUT_MS: u64 = 60_000;
const TOKEN_REQUEST_TIMEOUT_MS: u64 = 5_000;
// Official EOS proof verification may take 20 seconds. Native HTTP permits
// 25 seconds, leaving this ticket live for the completion/apply margin too.
const PROOF_REQUEST_TIMEOUT_MS: u64 = 30_000;
const MAX_TOKEN_BYTES: usize = 8192;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RequestKind {
    IssuerToken = 1,
    IdentityProof = 2,
}

pub struct Request {
    pub kind: RequestKind,
    pub ticket: u64,
    pub token: String,
    pub user: String,
}

pub struct Response<'a> {
    pub kind: RequestKind,
    pub ticket: u64,
    pub status: u16,
    pub ok: bool,
    pub token: &'a str,
    pub user: &'a str,
    pub expires_in: u32,
}

pub enum Command {
    None,
    ExternalLogin(String),
    Admitted,
}

pub struct Completion {
    pub accepted: bool,
    pub command: Command,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LoginResult {
    Success,
    InvalidUser,
    Failed,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SdkAction {
    CreateInitialUser,
    CopyIdentityProof,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    InitialTimeout,
    TicketExhausted,
    ServerDenied,
    InvalidResponse,
    InvalidCredential,
    InvalidIdentity,
    IdentityChanged,
    UnexpectedSdkResult,
    SdkFailure,
    InvalidUserAfterIdentity,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Never format credentials, proofs or account identifiers into errors.
        write!(f, "EOS account authentication failed: {self:?}")
    }
}
impl std::error::Error for Error {}

#[derive(Clone, Copy, PartialEq, Eq)]
enum Stage {
    NeedToken,
    AwaitToken,
    LoggingIn,
    CreatingInitialUser,
    CopyingProof,
    NeedProof,
    AwaitProof,
    Bound,
    Failed(Error),
}

struct Pending {
    kind: RequestKind,
    ticket: u64,
    deadline: u64,
}

pub struct AccountAuth {
    stage: Stage,
    user: String,
    proof: String,
    ever_bound: bool,
    pending: Option<Pending>,
    next_ticket: u64,
    now: u64,
    initial_deadline: u64,
    retry_at: u64,
    failures: u32,
}

pub fn valid_user(user: &str) -> bool {
    user.len() == 32
        && user
            .bytes()
            .all(|byte| byte.is_ascii_hexdigit() && !byte.is_ascii_uppercase())
}

fn valid_token(token: &str) -> bool {
    !token.is_empty()
        && token.len() <= MAX_TOKEN_BYTES
        && token.bytes().all(|byte| (33..127).contains(&byte))
}

impl AccountAuth {
    pub fn new(now: u64) -> Self {
        Self {
            stage: Stage::NeedToken,
            user: String::new(),
            proof: String::new(),
            ever_bound: false,
            pending: None,
            next_ticket: 1,
            now,
            initial_deadline: now.saturating_add(INITIAL_TIMEOUT_MS),
            retry_at: now,
            failures: 0,
        }
    }

    fn fail<T>(&mut self, error: Error) -> Result<T, Error> {
        self.stage = Stage::Failed(error);
        self.pending = None;
        self.proof.clear();
        Err(error)
    }

    fn retry(&mut self, kind: RequestKind) {
        let delay = 250_u64 << self.failures.min(3);
        self.failures = self.failures.saturating_add(1);
        self.retry_at = self.now.saturating_add(delay);
        self.pending = None;
        self.stage = match kind {
            RequestKind::IssuerToken => Stage::NeedToken,
            RequestKind::IdentityProof => Stage::NeedProof,
        };
    }

    pub fn tick(&mut self, now: u64) -> Result<(), Error> {
        self.now = self.now.max(now);
        if let Stage::Failed(error) = self.stage {
            return Err(error);
        }
        if !self.ever_bound && self.now >= self.initial_deadline {
            return self.fail(Error::InitialTimeout);
        }
        if let Some(pending) = &self.pending
            && self.now >= pending.deadline
        {
            self.retry(pending.kind);
        }
        Ok(())
    }

    /// Take one request. Repeated polls cannot dispatch the same ticket twice.
    pub fn request(&mut self, now: u64) -> Result<Option<Request>, Error> {
        self.tick(now)?;
        if self.now < self.retry_at {
            return Ok(None);
        }
        let kind = match self.stage {
            Stage::NeedToken => RequestKind::IssuerToken,
            Stage::NeedProof => RequestKind::IdentityProof,
            _ => return Ok(None),
        };
        let Some(next) = self.next_ticket.checked_add(1) else {
            return self.fail(Error::TicketExhausted);
        };
        let ticket = self.next_ticket;
        self.next_ticket = next;
        self.pending = Some(Pending {
            kind,
            ticket,
            deadline: self.now.saturating_add(match kind {
                RequestKind::IssuerToken => TOKEN_REQUEST_TIMEOUT_MS,
                RequestKind::IdentityProof => PROOF_REQUEST_TIMEOUT_MS,
            }),
        });
        self.stage = match kind {
            RequestKind::IssuerToken => Stage::AwaitToken,
            RequestKind::IdentityProof => Stage::AwaitProof,
        };
        Ok(Some(Request {
            kind,
            ticket,
            token: if kind == RequestKind::IdentityProof {
                self.proof.clone()
            } else {
                String::new()
            },
            user: if kind == RequestKind::IdentityProof {
                self.user.clone()
            } else {
                String::new()
            },
        }))
    }

    pub fn complete(&mut self, response: Response<'_>, now: u64) -> Result<Completion, Error> {
        self.tick(now)?;
        if !self.pending.as_ref().is_some_and(|pending| {
            pending.ticket == response.ticket && pending.kind == response.kind
        }) {
            return Ok(Completion {
                accepted: false,
                command: Command::None,
            });
        }
        if response.status == 0 || (500..600).contains(&response.status) {
            self.retry(response.kind);
            return Ok(Completion {
                accepted: true,
                command: Command::None,
            });
        }
        if (400..500).contains(&response.status) {
            return self.fail(Error::ServerDenied);
        }
        if response.status != 200 || !response.ok {
            return self.fail(Error::InvalidResponse);
        }
        self.pending = None;
        self.failures = 0;
        let command = match response.kind {
            RequestKind::IssuerToken => {
                if !valid_token(response.token)
                    || !response.user.is_empty()
                    || !(1..=120).contains(&response.expires_in)
                {
                    return self.fail(Error::InvalidCredential);
                }
                self.stage = Stage::LoggingIn;
                // The issuer JWT is returned directly to one SDK call. It is
                // never retained in AccountAuth, including during refresh.
                Command::ExternalLogin(response.token.to_owned())
            }
            RequestKind::IdentityProof => {
                if !response.token.is_empty() || response.expires_in != 0 {
                    return self.fail(Error::InvalidResponse);
                }
                if !valid_user(response.user) {
                    return self.fail(Error::InvalidIdentity);
                }
                if response.user != self.user {
                    return self.fail(Error::IdentityChanged);
                }
                self.proof.clear();
                self.stage = Stage::Bound;
                self.ever_bound = true;
                Command::Admitted
            }
        };
        Ok(Completion {
            accepted: true,
            command,
        })
    }

    pub fn sdk_login_result(
        &mut self,
        created: bool,
        result: LoginResult,
        user: &str,
        now: u64,
    ) -> Result<SdkAction, Error> {
        self.tick(now)?;
        if !matches!(
            (self.stage, created),
            (Stage::LoggingIn, false) | (Stage::CreatingInitialUser, true)
        ) {
            return self.fail(Error::UnexpectedSdkResult);
        }
        match result {
            LoginResult::InvalidUser if !created && self.user.is_empty() && !self.ever_bound => {
                self.stage = Stage::CreatingInitialUser;
                return Ok(SdkAction::CreateInitialUser);
            }
            LoginResult::InvalidUser => return self.fail(Error::InvalidUserAfterIdentity),
            LoginResult::Failed => return self.fail(Error::SdkFailure),
            LoginResult::Success => {}
        }
        if !valid_user(user) {
            return self.fail(Error::InvalidIdentity);
        }
        if !self.user.is_empty() && self.user != user {
            return self.fail(Error::IdentityChanged);
        }
        if self.user.is_empty() {
            self.user = user.to_owned();
        }
        self.stage = Stage::CopyingProof;
        Ok(SdkAction::CopyIdentityProof)
    }

    pub fn identity_proof(&mut self, proof: &str, now: u64) -> Result<(), Error> {
        self.tick(now)?;
        if self.stage != Stage::CopyingProof {
            return self.fail(Error::UnexpectedSdkResult);
        }
        if !valid_token(proof) {
            return self.fail(Error::InvalidCredential);
        }
        self.proof = proof.to_owned();
        self.failures = 0;
        self.retry_at = self.now;
        self.stage = Stage::NeedProof;
        Ok(())
    }

    /// AuthExpiration is an SDK fact, not a periodic synthetic re-login timer.
    pub fn auth_expiration(&mut self, now: u64) -> Result<(), Error> {
        self.tick(now)?;
        if self.stage == Stage::Bound {
            self.stage = Stage::NeedToken;
            self.retry_at = self.now;
            self.failures = 0;
        }
        // A repeated expiration while an actual refresh is pending does not
        // reuse its credential or issue a concurrent SDK login.
        Ok(())
    }

    pub fn admitted(&self) -> bool {
        self.ever_bound && !matches!(self.stage, Stage::Failed(_))
    }

    pub fn user(&self) -> &str {
        &self.user
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const USER: &str = "0123456789abcdef0123456789abcdef";
    const OTHER: &str = "1123456789abcdef0123456789abcdef";

    fn response(request: &Request) -> Response<'_> {
        Response {
            kind: request.kind,
            ticket: request.ticket,
            status: 200,
            ok: true,
            token: if request.kind == RequestKind::IssuerToken {
                "fresh.jwt"
            } else {
                ""
            },
            user: if request.kind == RequestKind::IdentityProof {
                USER
            } else {
                ""
            },
            expires_in: if request.kind == RequestKind::IssuerToken {
                120
            } else {
                0
            },
        }
    }

    fn issuer(auth: &mut AccountAuth, now: u64) {
        let request = auth.request(now).unwrap().unwrap();
        assert_eq!(request.kind, RequestKind::IssuerToken);
        assert!(request.token.is_empty() && request.user.is_empty());
        let completion = auth.complete(response(&request), now).unwrap();
        assert!(completion.accepted);
        assert!(
            matches!(completion.command, Command::ExternalLogin(token) if token == "fresh.jwt")
        );
        assert!(auth.request(now).unwrap().is_none());
    }

    fn sdk_proof(auth: &mut AccountAuth, created: bool, now: u64) -> Request {
        assert_eq!(
            auth.sdk_login_result(created, LoginResult::Success, USER, now),
            Ok(SdkAction::CopyIdentityProof)
        );
        auth.identity_proof("sdk.identity.proof", now).unwrap();
        let request = auth.request(now).unwrap().unwrap();
        assert_eq!(request.kind, RequestKind::IdentityProof);
        assert_eq!(request.token, "sdk.identity.proof");
        assert_eq!(request.user, USER);
        request
    }

    fn bound() -> AccountAuth {
        let mut auth = AccountAuth::new(0);
        issuer(&mut auth, 1);
        let proof = sdk_proof(&mut auth, false, 2);
        assert!(matches!(
            auth.complete(response(&proof), 3).unwrap().command,
            Command::Admitted
        ));
        assert!(auth.admitted());
        auth
    }

    #[test]
    fn fresh_issuer_sdk_identity_and_exact_server_ack_are_all_required() {
        let mut auth = AccountAuth::new(0);
        assert!(!auth.admitted());
        issuer(&mut auth, 0);
        assert!(!auth.admitted() && auth.user().is_empty());
        let proof = sdk_proof(&mut auth, false, 1);
        assert!(!auth.admitted());
        assert_eq!(auth.user(), USER);
        assert!(auth.request(1).unwrap().is_none());
        assert!(matches!(
            auth.complete(response(&proof), 2).unwrap().command,
            Command::Admitted
        ));
        assert!(auth.admitted());
        assert!(auth.request(3).unwrap().is_none());
        assert!(!auth.complete(response(&proof), 3).unwrap().accepted);
    }

    #[test]
    fn only_initial_invalid_user_can_create_once_and_still_needs_master_ack() {
        let mut auth = AccountAuth::new(0);
        issuer(&mut auth, 0);
        assert_eq!(
            auth.sdk_login_result(false, LoginResult::InvalidUser, "", 1),
            Ok(SdkAction::CreateInitialUser)
        );
        assert!(!auth.admitted());
        let proof = sdk_proof(&mut auth, true, 2);
        assert!(!auth.admitted());
        auth.complete(response(&proof), 3).unwrap();
        assert!(auth.admitted());

        let mut duplicate = AccountAuth::new(0);
        issuer(&mut duplicate, 0);
        duplicate
            .sdk_login_result(false, LoginResult::InvalidUser, "", 1)
            .unwrap();
        assert_eq!(
            duplicate.sdk_login_result(false, LoginResult::InvalidUser, "", 2),
            Err(Error::UnexpectedSdkResult)
        );
        assert!(!duplicate.admitted());
    }

    #[test]
    fn wrong_ticket_kind_and_stale_completion_cannot_consume_current_request() {
        let mut auth = AccountAuth::new(0);
        let request = auth.request(0).unwrap().unwrap();
        let mut wrong = response(&request);
        wrong.ticket += 1;
        assert!(!auth.complete(wrong, 1).unwrap().accepted);
        let mut wrong = response(&request);
        wrong.kind = RequestKind::IdentityProof;
        assert!(!auth.complete(wrong, 2).unwrap().accepted);
        assert!(matches!(
            auth.complete(response(&request), 3).unwrap().command,
            Command::ExternalLogin(_)
        ));
        assert!(!auth.complete(response(&request), 4).unwrap().accepted);
        assert!(!auth.admitted());
    }

    #[test]
    fn wrong_master_identity_and_noncanonical_ack_fail_closed() {
        for (user, token, expires, expected) in [
            (OTHER, "", 0, Error::IdentityChanged),
            (
                "0123456789ABCDEF0123456789ABCDEF",
                "",
                0,
                Error::InvalidIdentity,
            ),
            (USER, "unexpected", 0, Error::InvalidResponse),
            (USER, "", 1, Error::InvalidResponse),
        ] {
            let mut auth = AccountAuth::new(0);
            issuer(&mut auth, 0);
            let proof = sdk_proof(&mut auth, false, 1);
            let mut ack = response(&proof);
            ack.user = user;
            ack.token = token;
            ack.expires_in = expires;
            assert!(matches!(auth.complete(ack, 2), Err(error) if error == expected));
            assert!(!auth.admitted());
            assert!(auth.request(3).is_err());
            assert_eq!(auth.user(), USER);
        }
    }

    #[test]
    fn expiration_fetches_new_issuer_grant_without_reusing_login_or_changing_identity() {
        let mut auth = bound();
        auth.auth_expiration(100).unwrap();
        let fresh = auth.request(100).unwrap().unwrap();
        assert_eq!(fresh.kind, RequestKind::IssuerToken);
        assert!(fresh.token.is_empty() && fresh.user.is_empty());
        auth.auth_expiration(101).unwrap();
        assert!(auth.request(101).unwrap().is_none());
        let mut grant = response(&fresh);
        grant.token = "different.fresh.jwt";
        assert!(
            matches!(auth.complete(grant, 102).unwrap().command, Command::ExternalLogin(token) if token == "different.fresh.jwt")
        );
        assert!(auth.admitted());
        let proof = sdk_proof(&mut auth, false, 103);
        auth.complete(response(&proof), 104).unwrap();
        assert!(auth.admitted() && auth.user() == USER);
        auth.auth_expiration(200).unwrap();
        assert!(auth.request(200).unwrap().unwrap().ticket > fresh.ticket);
    }

    #[test]
    fn refresh_invalid_user_or_identity_change_never_creates_or_reassigns_user() {
        for (result, user, expected) in [
            (
                LoginResult::InvalidUser,
                "",
                Error::InvalidUserAfterIdentity,
            ),
            (LoginResult::Success, OTHER, Error::IdentityChanged),
            (LoginResult::Failed, "", Error::SdkFailure),
        ] {
            let mut auth = bound();
            auth.auth_expiration(100).unwrap();
            issuer(&mut auth, 100);
            assert_eq!(
                auth.sdk_login_result(false, result, user, 101),
                Err(expected)
            );
            assert!(!auth.admitted());
            assert_eq!(auth.user(), USER);
            assert!(auth.request(102).is_err());
        }
    }

    #[test]
    fn backend_failures_back_off_with_new_tickets_and_expired_requests_are_stale() {
        let mut auth = AccountAuth::new(0);
        let first = auth.request(0).unwrap().unwrap();
        assert!(!auth.complete(response(&first), 5_000).unwrap().accepted);
        assert!(auth.request(5_249).unwrap().is_none());
        let second = auth.request(5_250).unwrap().unwrap();
        assert!(second.ticket > first.ticket);
        assert!(!auth.complete(response(&first), 5_250).unwrap().accepted);
        let mut failure = response(&second);
        failure.status = 503;
        failure.ok = false;
        auth.complete(failure, 5_251).unwrap();
        assert!(auth.request(5_750).unwrap().is_none());
        let third = auth.request(5_751).unwrap().unwrap();
        assert!(third.ticket > second.ticket && third.token.is_empty());
        let mut failure = response(&third);
        failure.status = 0;
        failure.ok = false;
        auth.complete(failure, 5_752).unwrap();
        assert!(auth.request(6_751).unwrap().is_none());
        assert!(auth.request(6_752).unwrap().is_some());
    }

    #[test]
    fn long_bound_refresh_outage_keeps_network_but_actual_denial_revokes_admission() {
        let mut auth = bound();
        auth.auth_expiration(100).unwrap();
        for now in [100, 10_000, 100_000, 1_000_000] {
            let request = auth.request(now).unwrap().unwrap();
            let mut unavailable = response(&request);
            unavailable.status = 503;
            unavailable.ok = false;
            auth.complete(unavailable, now).unwrap();
            assert!(auth.admitted());
        }
        let request = auth.request(1_100_000).unwrap().unwrap();
        let mut denied = response(&request);
        denied.status = 401;
        denied.ok = false;
        assert!(matches!(
            auth.complete(denied, 1_100_001),
            Err(Error::ServerDenied)
        ));
        assert!(!auth.admitted());
    }

    #[test]
    fn proof_retry_never_forges_ack_or_restarts_sdk_login() {
        let mut auth = AccountAuth::new(0);
        issuer(&mut auth, 0);
        let proof = sdk_proof(&mut auth, false, 1);
        let mut unavailable = response(&proof);
        unavailable.status = 500;
        unavailable.ok = false;
        auth.complete(unavailable, 2).unwrap();
        assert!(!auth.admitted());
        let retry = auth.request(252).unwrap().unwrap();
        assert_eq!(retry.kind, RequestKind::IdentityProof);
        assert_eq!(retry.token, proof.token);
        assert_eq!(retry.user, USER);
        assert!(retry.ticket > proof.ticket);
        assert!(!auth.complete(response(&proof), 253).unwrap().accepted);
        auth.complete(response(&retry), 254).unwrap();
        assert!(auth.admitted());
    }

    #[test]
    fn slow_official_proof_response_remains_live_through_http_completion_margin() {
        let mut auth = AccountAuth::new(0);
        issuer(&mut auth, 1);
        let proof = sdk_proof(&mut auth, false, 20_000);
        assert!(!auth.admitted());
        assert!(auth.request(45_000).unwrap().is_none());
        assert!(matches!(
            auth.complete(response(&proof), 49_999).unwrap().command,
            Command::Admitted
        ));
        assert!(auth.admitted());

        let mut auth = AccountAuth::new(0);
        issuer(&mut auth, 1);
        let proof = sdk_proof(&mut auth, false, 2);
        assert!(!auth.complete(response(&proof), 30_002).unwrap().accepted);
        assert!(!auth.admitted());
        assert_eq!(
            auth.request(30_252).unwrap().unwrap().kind,
            RequestKind::IdentityProof
        );
    }

    #[test]
    fn malformed_success_credentials_and_http_shapes_are_terminal() {
        for (token, ttl) in [
            ("", 120),
            ("has whitespace", 120),
            ("token", 0),
            ("token", 121),
        ] {
            let mut auth = AccountAuth::new(0);
            let request = auth.request(0).unwrap().unwrap();
            let mut value = response(&request);
            value.token = token;
            value.expires_in = ttl;
            assert!(matches!(
                auth.complete(value, 1),
                Err(Error::InvalidCredential)
            ));
            assert!(!auth.admitted());
        }
        for status in [200, 201, 302, 403, 404, 409] {
            let mut auth = AccountAuth::new(0);
            let request = auth.request(0).unwrap().unwrap();
            let mut value = response(&request);
            value.status = status;
            value.ok = false;
            assert!(auth.complete(value, 1).is_err());
            assert!(!auth.admitted());
        }
    }

    #[test]
    fn initial_timeout_monotonic_clock_and_ticket_exhaustion_cannot_wrap() {
        let mut auth = AccountAuth::new(10);
        auth.request(20).unwrap();
        auth.tick(0).unwrap();
        assert_eq!(auth.now, 20);
        assert_eq!(auth.tick(60_010), Err(Error::InitialTimeout));
        assert!(auth.request(0).is_err());

        let mut auth = AccountAuth::new(0);
        auth.next_ticket = u64::MAX;
        assert!(matches!(auth.request(0), Err(Error::TicketExhausted)));
        assert!(!auth.admitted());
    }
}
