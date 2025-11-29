# Melian JavaScript Client

Node.js implementation of the Melian protocol. Provides an async API for fetching
rows by table/index IDs, mirroring the Perl/PHP/Python clients.

## Installation

```bash
cd clients/js
npm install
```

## Usage

```javascript
import { MelianClient } from './src/MelianClient.js';

const client = await MelianClient.create({ dsn: 'unix:///tmp/melian.sock' });

const { tableId, indexId } = client.resolveIndex('table2', 'hostname');
const row = await client.fetchByString(tableId, indexId, Buffer.from('host-00042'));
console.log(row);

await client.close();
```

Initialization options (pass to `create`):

- `dsn`: `unix:///path` or `tcp://host:port` (defaults to `/tmp/melian.sock`).
- `timeout`: socket timeout in milliseconds (default 1000).
- `schema`: prebuilt schema object.
- `schemaSpec`: inline schema spec string (`table#id|period|col#idx:type`).
- `schemaFile`: JSON schema file path.

If no schema option is provided, the client issues a `DESCRIBE` action on first
connection.

## Tests

The test suite assumes a running Melian server (change DSN via
`MELIAN_TEST_DSN`).

```bash
cd clients/js
npm install
MELIAN_TEST_DSN="unix:///tmp/melian.sock" npm test
```
