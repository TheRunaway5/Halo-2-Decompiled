"""Small source contracts for test-owned custom cohorts."""
import json
from pathlib import Path


def contract_fixture(root, maps):
    from custom_asset_cache import CONTRACT, POLICY
    from native_variant_assets import DEFINITIONS
    path = Path(root)/CONTRACT
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(dict(schema='H2_PORT_CUSTOM_ASSET_BUNDLE_V1',version='fixture',
        maps=maps,game_modes=list(DEFINITIONS),controller_families=['xbox','playstation'],
        celestial_slots=list(range(43,50)),policy=POLICY)))
