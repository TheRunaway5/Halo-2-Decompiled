//! EOS-purpose account/room routes. SDK awaits hold no matchmaking/SQL lock.
use super::*;
use crate::eos_identity;

pub(super) enum Route {
    Jwks,
    Token(Option<String>),
    Proof(Option<String>),
}
pub(super) fn route(method: &str, path: &str) -> Option<Route> {
    match (method, path) {
        ("GET", "/v1/auth/eos/jwks") => Some(Route::Jwks),
        ("POST", "/v1/account/eos-token") => Some(Route::Token(None)),
        ("POST", "/v1/account/eos-proof") => Some(Route::Proof(None)),
        ("POST", _) => match path.split('/').collect::<Vec<_>>().as_slice() {
            ["", "v1", "matches", id, "eos-token"] if !id.is_empty() && id.len() <= 64 => {
                Some(Route::Token(Some((*id).into())))
            }
            ["", "v1", "matches", id, "eos-proof"] if !id.is_empty() && id.len() <= 64 => {
                Some(Route::Proof(Some((*id).into())))
            }
            _ => None,
        },
        _ => None,
    }
}
pub(super) async fn handle(
    app: Arc<App>,
    request: Request,
    header: String,
    route: Route,
) -> Result<Response> {
    let issuer = app
        .eos
        .clone()
        .ok_or(Error(503, "EOS identity is not configured"))?;
    let deadline = Instant::now() + Duration::from_secs(25);
    let (proof, room) = match route {
        Route::Jwks => return Ok(response(200, issuer.jwks()?)),
        Route::Token(room) => (false, room),
        Route::Proof(room) => (true, room),
    };
    let is_room = room.is_some();
    let authorization = header.clone();
    let owner: crate::runtime::EosOwner = websocket::call(app.clone(), move |runtime| {
        runtime.eos_owner(&authorization, room.as_deref())
    })
    .await?;
    if owner.is_room() {
        app.budgets
            .lock()
            .map_err(|_| Error(503, "Request budgets unavailable"))?
            .eos_room(owner.player(), app.clock())?;
    }
    let body = read_body(request).await?;
    let value = if proof {
        let (puid, token) = eos_identity::proof_input(&body, is_room)?;
        let previous = owner.clone();
        let authorization = header.clone();
        websocket::call(app.clone(), move |runtime| {
            runtime.eos_recheck(&authorization, &previous)
        })
        .await?;
        let verified = issuer.verifier.verify(puid, token, owner.player()).await?;
        websocket::call(app, move |runtime| {
            if Instant::now() >= deadline {
                return Err(Error(503, "EOS request expired"));
            }
            runtime.eos_bind(&header, &owner, &verified)
        })
        .await?
    } else {
        halo2_online_protocol::matchmaking_model::fields(&body, &[])?;
        websocket::call(app, move |runtime| {
            if Instant::now() >= deadline {
                return Err(Error(503, "EOS request expired"));
            }
            runtime.eos_token(&header, &owner, &issuer)
        })
        .await?
    };
    Ok(response(200, value))
}
