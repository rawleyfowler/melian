PHP client library for the Melian caching server.

```bash
# Install dependencies
composer install

# Run
./vendor/bin/phpunit
```

Some PHP sample code:

```php
$client = new Melian\Client(['dsn' => 'unix:///tmp/melian.sock']);
[$tableId, $indexId] = $client->resolveIndex('table2', 'id');
$row = $client->fetchJsonById($tableId, $indexId, 42);
```
