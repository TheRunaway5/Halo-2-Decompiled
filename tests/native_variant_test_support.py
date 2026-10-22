"""Small authored-definition inputs for onboarding and packaging transactions."""
from pathlib import Path
import shutil

from native_variant_assets import DEFINITIONS, DIRECTORY, LAYOUTS


def variant_fixture(root):
    source = Path(__file__).resolve().parents[1]
    target = Path(root) / DIRECTORY
    target.mkdir(parents=True, exist_ok=True)
    for identifier in DEFINITIONS:
        shutil.copyfile(source / DIRECTORY / (identifier + ".json"), target / (identifier + ".json"))
    shutil.copyfile(source / LAYOUTS, Path(root) / LAYOUTS)
