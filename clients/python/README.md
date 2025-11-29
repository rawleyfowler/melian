# Melian Python Client

Thin Python wrapper for the Melian cache server protocol. It exposes a simple
API for fetching rows by table/index identifiers.

## Installation

```bash
cd clients/python
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

The library itself has no third-party dependencies; the requirements file only
includes tooling for running the test suite (pytest).

## Usage

```python
from melian import MelianClient

client = MelianClient(dsn="unix:///tmp/melian.sock")

# Resolve identifiers from table/index names (optional helper)
table_id, index_id = client.resolve_index("table2", "hostname")

# Fetch JSON row by hostname
row = client.fetch_by_string(table_id, index_id, b"host-00042")
print(row)

# Fetch by numeric ID (encoded as little-endian uint32)
row = client.fetch_by_int(table_id, index_id=0, record_id=42)
print(row)

client.close()
```

Schema loading options (pass via `__init__`):

- `schema`: pre-parsed schema dict.
- `schema_spec`: inline spec string (e.g. `table1#0|60|id#0:int`).
- `schema_file`: JSON file on disk.
- default: issue a `DESCRIBE` action to the running server.

## Tests

The tests assume a Melian server is available (default `unix:///tmp/melian.sock`).
Override via `MELIAN_TEST_DSN` when needed.

```bash
cd clients/python
source .venv/bin/activate
pytest
```
