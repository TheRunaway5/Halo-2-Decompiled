"""The legacy discovery process cannot become a second matchmaking owner."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from aiohttp.test_utils import TestClient, TestServer

from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.service import Directory, Settings, create_app


class DiscoveryOnlyTests(unittest.IsolatedAsyncioTestCase):
    async def test_listings_work_without_account_or_queue_owners(self):
        with tempfile.TemporaryDirectory() as scratch:
            settings = Settings(state=Path(scratch), public_url="http://localhost",
                                allow_http=True, discovery_only=True, sync_interval=0.1)
            async with TestClient(TestServer(create_app(settings))) as client:
                directory = next(v for v in client.server.app.values() if isinstance(v, Directory))
                self.assertFalse(hasattr(directory, "matchmaking"))
                self.assertFalse(hasattr(directory, "clans"))
                self.assertFalse(hasattr(directory, "steam"))
                for path in ("/v1/health", "/v1/masters", "/v1/servers"):
                    self.assertEqual((await client.get(path)).status, 200, path)
                tables = {r[0] for r in directory.store.db.execute("SELECT name FROM sqlite_master WHERE type='table'")}
                self.assertFalse(tables & {"players", "steam_login_sessions", "clans", "match_results"})
                for method, path in (("POST", "/v1/account/session"),
                                     ("POST", "/v1/matchmaking/session"),
                                     ("GET", "/v1/matchmaking/playlists"),
                                     ("GET", "/v1/online/status"),
                                     ("GET", "/v1/party"),
                                     ("GET", "/v1/clans"),
                                     ("GET", "/v1/auth/steam/status"),
                                     ("GET", "/v1/matches/absent/signal")):
                    self.assertEqual((await client.request(method, path)).status, 404, path)
                with self.assertRaises(Problem) as caught:
                    await directory.verify_identity("h2s_" + "A" * 43)
                self.assertEqual(caught.exception.status, 503)

    def test_conflicting_catalog_fails_before_opening_database(self):
        with tempfile.TemporaryDirectory() as scratch:
            state = Path(scratch) / "state"
            with self.assertRaisesRegex(Problem, "cannot own a matchmaking catalog"):
                create_app(Settings(state=state, public_url="http://localhost", allow_http=True,
                                    discovery_only=True, matchmaking_catalog="catalog.json"))
            self.assertFalse(state.exists())

    def test_environment_selects_explicit_discovery_mode(self):
        with tempfile.TemporaryDirectory() as scratch, patch.dict("os.environ", {
            "H2_DIRECTORY_ONLY": "1", "H2_MASTER_STATE": scratch,
            "H2_MATCHMAKING_CATALOG": "", "H2_STEAM_KEY_FILE": "", "H2_STEAM_KEY": "",
            "STEAM_KEY": "", "H2_MASTER_ADMIN_TOKEN_FILE": "", "H2_MASTER_PEERS_FILE": scratch + "/none",
        }):
            self.assertTrue(Settings.environment().discovery_only)
