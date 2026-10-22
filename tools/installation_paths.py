"""Bind mutable installation data to its separately owned read-only resources.

Source checkouts use the supplied root for both. Packaged launchers publish one
process-local binding inherited by their setup/UI children; it applies only to
the exact data root in that binding. Resource readers retain their existing
file/hash/containment checks after resolving this owner.
"""
import json
import os
from pathlib import Path

ENVIRONMENT = 'H2_INSTALLATION_LAYOUT'
SCHEMA = 'H2_INSTALLATION_LAYOUT_V1'


def resource_root(root):
    root = Path(root).resolve()
    raw = os.environ.get(ENVIRONMENT)
    if raw is None:
        return root
    try:
        value = json.loads(raw)
        if (not isinstance(value, dict) or value.get('schema') != SCHEMA or
                set(value) != {'schema', 'data', 'resources'}):
            raise ValueError('Invalid installation layout')
        data, resources = Path(value['data']), Path(value['resources'])
        if not data.is_absolute() or not resources.is_absolute():
            raise ValueError('Installation layout requires absolute roots')
        data = data.resolve()
        if root != data:
            return root
        resources = resources.resolve(strict=True)
        if not resources.is_dir():
            raise ValueError('Installation resource directory is unavailable')
        if data != resources and (data.is_relative_to(resources) or resources.is_relative_to(data)):
            raise ValueError('Installation resource and data roots overlap')
        return resources
    except (TypeError, KeyError, OSError, json.JSONDecodeError) as error:
        raise ValueError('Invalid installation resource binding') from error
