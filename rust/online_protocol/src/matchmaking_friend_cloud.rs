//! Private friend-file outbox policy. SDK/file/process operations remain typed IO.
//! An opaque native claim binds revision, digest and existing EOS identity.
use super::*;
use sha2::{Digest, Sha256};

const SCHEMA: &str = "CREATE TABLE IF NOT EXISTS friend_cloud_outbox(player TEXT PRIMARY KEY REFERENCES players(id),revision INTEGER NOT NULL,fingerprint TEXT NOT NULL,payload TEXT,digest TEXT NOT NULL,uploaded_revision INTEGER NOT NULL DEFAULT 0,next_attempt INTEGER NOT NULL DEFAULT 0,failures INTEGER NOT NULL DEFAULT 0,last_error TEXT NOT NULL DEFAULT '')";
pub(super) fn schema(c: &mut Cursor<'_>) -> Flow<Value> {
    c.batch(vec![statement(SCHEMA,json!([]),None),statement("CREATE INDEX IF NOT EXISTS friend_cloud_pending ON friend_cloud_outbox(next_attempt) WHERE revision>uploaded_revision",json!([]),None)])?;
    Ok(Value::Null)
}
pub(super) fn status(c: &mut Cursor<'_>, who: &str, enabled: bool) -> Flow<Value> {
    if !enabled {
        return Ok(json!({"state":"disabled"}));
    }
    let row = c.one(
        "SELECT revision,uploaded_revision,last_error FROM friend_cloud_outbox WHERE player=?",
        json!([who]),
    )?;
    Ok(match row {
        Some(r) => {
            let error = r[2]
                .as_str()
                .ok_or(Error(503, "Invalid friend cloud status"))?;
            json!({"state":if r[0]==r[1]{"synced"}else if error.is_empty(){"pending"}else{"unavailable"},"revision":r[0],"uploaded_revision":r[1]})
        }
        None => json!({"state":"pending","revision":0,"uploaded_revision":0}),
    })
}
pub(super) fn enqueue(c: &mut Cursor<'_>, who: &str, enabled: bool) -> Flow<bool> {
    if !enabled {
        return Ok(false);
    }
    let tag = tag(c, who)?;
    if tag.is_null() {
        return Ok(false);
    }
    let public = c
        .one(
            "SELECT public_id FROM history_players WHERE player=?",
            json!([who]),
        )?
        .ok_or(Error(500, "Friend cloud account is not published"))?;
    let mut relationships = roster(c, who)?
        .into_iter()
        .map(|row| json!({"player":row[2],"state":row[1]}))
        .collect::<Vec<_>>();
    relationships.sort_by(|a, b| s(a, "player").cmp(s(b, "player")));
    let mut state = json!({"format":"halo2-friends-v1","player":public[0],"friend_tag":tag,"relationships":relationships});
    let fingerprint = format!("{:x}", Sha256::digest(ascii_json(&state).as_bytes()));
    let previous = c.one(
        "SELECT revision,fingerprint FROM friend_cloud_outbox WHERE player=?",
        json!([who]),
    )?;
    if previous.as_ref().is_some_and(|r| r[1] == fingerprint) {
        return Ok(false);
    }
    let revision = previous
        .as_ref()
        .map(|r| m::integer(&r[0], 1, i64::MAX - 1))
        .transpose()?
        .unwrap_or(0)
        + 1;
    state["revision"] = json!(revision);
    let payload = ascii_json(&state);
    need(
        payload.len() <= 32768,
        500,
        "Friend cloud record exceeds bound",
    )?;
    let digest = format!("{:x}", Sha256::digest(payload.as_bytes()));
    c.write("INSERT INTO friend_cloud_outbox(player,revision,fingerprint,payload,digest) VALUES(?,?,?,?,?) ON CONFLICT(player) DO UPDATE SET revision=excluded.revision,fingerprint=excluded.fingerprint,payload=excluded.payload,digest=excluded.digest,next_attempt=0,failures=0,last_error=''",json!([who,revision,fingerprint,payload,digest]),Some(1))?;
    Ok(true)
}
fn wall(c: &mut Cursor<'_>, r: &Value) -> Flow<i64> {
    let floor = c
        .one("SELECT value FROM clock_floor WHERE id=1", json!([]))?
        .ok_or(Error(503, "Friend clock unavailable"))?;
    let now = m::integer(&r["wall"], 0, 9007199254650000)?.max(m::integer(
        &floor[0],
        0,
        9007199254740991,
    )?);
    need(now <= 9007199254650000, 503, "Friend clock exhausted")?;
    c.write(
        "UPDATE clock_floor SET value=? WHERE id=1 AND value<=?",
        json!([now, now]),
        Some(1),
    )?;
    Ok(now)
}
pub(super) fn next(c: &mut Cursor<'_>, r: &Value) -> Flow<Value> {
    // Never send a caller's uncommitted relationship state to a provider.
    if r["friend_cloud"] != true || !c.transaction_owned {
        return Ok(Value::Null);
    }
    let now = wall(c, r)?;
    let row=c.one("SELECT o.player,o.revision,o.payload,o.digest,e.puid,p.screen_name,o.failures FROM friend_cloud_outbox o JOIN eos_players e ON e.player=o.player JOIN player_profiles p ON p.player=o.player WHERE o.revision>o.uploaded_revision AND o.next_attempt<=? ORDER BY o.next_attempt,o.player LIMIT 1",json!([now]))?;
    let Some(row) = row else {
        return Ok(Value::Null);
    };
    let mut context = json!({"player":row[0],"revision":row[1],"payload":row[2],"digest":row[3],"puid":row[4],"name":row[5],"failures":row[6]});
    let token = c.friend_cache(json!({"op":"cloud_prepare","context":context}))?;
    context["claim"] = token;
    Ok(context)
}
pub(super) fn finish(c: &mut Cursor<'_>, r: &Value) -> Flow<Value> {
    need(
        c.transaction_owned,
        409,
        "Friend cloud completion requires an owned transaction",
    )?;
    m::fields(&r["body"], &["claim", "outcome"])?;
    let token = uuid(&r["body"]["claim"])?;
    let outcome = m::text(&r["body"]["outcome"], 16)?;
    need(
        ["success", "failure", "cancel"].contains(&outcome),
        400,
        "Invalid friend cloud outcome",
    )?;
    let context = c.friend_cache(json!({"op":"cloud_context","claim":token}))?;
    if outcome == "cancel" {
        return Ok(Value::Null);
    }
    let now = wall(c, r)?;
    let who = txt(&context["player"])?;
    let bound = c.one("SELECT puid FROM eos_players WHERE player=?", json!([who]))?;
    if outcome == "success" && bound.as_ref().is_some_and(|b| b[0] == context["puid"]) {
        c.write("UPDATE friend_cloud_outbox SET uploaded_revision=?,payload=NULL,failures=0,last_error='' WHERE player=? AND revision=? AND digest=? AND uploaded_revision<revision",json!([context["revision"],who,context["revision"],context["digest"]]),None)?;
    } else {
        let failures = m::integer(&context["failures"], 0, i64::MAX - 1)?;
        let delay = (30 * (1i64 << failures.min(5))).min(900);
        c.write("UPDATE friend_cloud_outbox SET failures=failures+1,last_error='unavailable',next_attempt=? WHERE player=? AND revision=? AND uploaded_revision<revision",json!([now+delay,who,context["revision"]]),None)?;
    }
    Ok(Value::Null)
}
