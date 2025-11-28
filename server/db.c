#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "util.h"
#include "log.h"
#include "arena.h"
#include "hash.h"
#include "config.h"
#include "db.h"
#include "data.h"

// TODO: make these limits dynamic? Arena?
enum {
  MAX_FIELDS = 99,
  MAX_FIELD_NAME_LEN = 100,
  MAX_JSON_LEN = 10240,
  MAX_SQL_LEN = 1024,
};

static void get_mysql_versions(DB* db);

DB* db_build(Config* config) {
  DB* db = 0;
  do {
    mysql_library_init(0, 0, 0);

    db = calloc(1, sizeof(DB));
    if (!db) {
      LOG_WARN("Could not allocate DB object");
      break;
    }
    db->config = config;
    get_mysql_versions(db);
  } while (0);
  return db;
}

void db_destroy(DB* db) {
  do {
    if (!db) break;
    db_disconnect(db);
    free(db);
  } while (0);
  mysql_library_end();
}

void db_connect(DB* db) {
  do {
    db->conn = (struct MYSQL *) mysql_init(NULL);
    if (!db->conn) {
      LOG_WARN("Could not initialize MySQL client");
      break;
    }

    ConfigMySQL* cfg = &db->config->mysql;
    MYSQL* conn = mysql_real_connect((MYSQL*) db->conn, cfg->host, cfg->user, cfg->password, cfg->database, cfg->port, NULL, 0);
    if (!conn || conn != (MYSQL*)db->conn) {
      LOG_WARN("Could not connect to MySQL server at %s:%u as user %s", cfg->host, cfg->port, cfg->user);
      break;
    }

    get_mysql_versions(db);

    LOG_INFO("Connected to MySQL server version [%s] at %s:%u as user %s", db->server_version, cfg->host, cfg->port, cfg->user);
  } while (0);
}

void db_disconnect(DB* db) {
  if (!db) return;
  if (!db->conn) return;
  ConfigMySQL* cfg = &db->config->mysql;
  mysql_close((MYSQL*) db->conn);
  db->conn = 0;
  LOG_INFO("Disconnected from MySQL server at %s:%u", cfg->host, cfg->port);
}

unsigned db_get_table_size(DB* db, const char* table) {
  unsigned rows = 0;
  unsigned count = 0;
  MYSQL_RES *result = 0;
  do {
    if (!db || !db->conn) {
      LOG_WARN("Cannot get table size for %s, invalid MySQL connection", table);
      break;
    }

    LOG_DEBUG("Counting rows from table %s", table);
    char sql[MAX_SQL_LEN];
    snprintf(sql, MAX_SQL_LEN, "SELECT COUNT(*) FROM %s", table);
    if (mysql_query((MYSQL*) db->conn, sql)) {
      LOG_WARN("Cannot run query [%s] for table %s", sql, table);
      break;
    }

    result = mysql_store_result((MYSQL*) db->conn);
    if (!result) {
      LOG_WARN("Cannot store MySQL result for COUNT query for table %s", table);
      break;
    }

    unsigned num_fields = mysql_num_fields(result);
    if (num_fields != 1) {
      LOG_WARN("Expected %u number of fields for COUNT query for table %s, got %u", 1, table, num_fields);
      break;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
      for (unsigned col = 0; col < num_fields; col++) {
        count = atoi(row[col]);
      }
      ++rows;
    }
    if (rows != 1) {
      LOG_WARN("Expected %u rows for COUNT query for table %s, got %u", 1, table, rows);
      break;
    }
  } while (0);

  if (result) mysql_free_result(result);
  LOG_DEBUG("Counted %u rows from table %s", count, table);
  return count;
}

unsigned db_query_into_hash(DB* db, Table* table, struct TableSlot* slot,
                            unsigned* min_id, unsigned* max_id) {
  unsigned rows = 0;
  MYSQL_RES *result = 0;
  do {
    if (!db || !db->conn) {
      LOG_WARN("Cannot query table data for %s, invalid MySQL connection", table_name(table));
      break;
    }

    double t0 = now_sec();
    LOG_DEBUG("Fetching from table %s", table_name(table));
    char sql[MAX_SQL_LEN];
    snprintf(sql, MAX_SQL_LEN, "SELECT * FROM %s", table_name(table));
    if (mysql_query((MYSQL*) db->conn, sql)) {
      LOG_WARN("Cannot run query [%s] for table %s", sql, table_name(table));
      break;
    }

    result = mysql_store_result((MYSQL*) db->conn);
    if (!result) {
      LOG_WARN("Cannot store MySQL result for SELECT query for table %s", table_name(table));
      break;
    }

    unsigned num_fields = mysql_num_fields(result);
    if (num_fields > MAX_FIELDS) {
      LOG_WARN("Expected at most %u number of fields for SELECT query for table %s, got %u", MAX_FIELDS, table_name(table), num_fields);
      break;
    }

    char names[MAX_FIELDS][MAX_FIELD_NAME_LEN];
    enum enum_field_types types[MAX_FIELDS];
    int index_pos[MELIAN_MAX_INDEXES];
    for (unsigned idx = 0; idx < MELIAN_MAX_INDEXES; ++idx) index_pos[idx] = -1;
    unsigned bad = 0;
    for (unsigned col = 0; col < num_fields; ++col) {
      MYSQL_FIELD *field = mysql_fetch_field(result);
      if (!field) {
        LOG_WARN("Could not fetch field %u for SELECT query for table %s", col, table_name(table));
        ++bad;
        break;
      }

      types[col] = field->type;
      LOG_DEBUG("Column %u type %u", col, (unsigned) field->type);
      snprintf(names[col], MAX_FIELD_NAME_LEN, "%s", field->name);
      for (unsigned idx = 0; idx < table->index_count; ++idx) {
        if (strcmp(field->name, table->indexes[idx].column) == 0) {
          index_pos[idx] = col;
        }
      }
    }
    if (bad) break;

    *min_id = (unsigned) -1;
    *max_id = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
      char jbuf[MAX_JSON_LEN];
      unsigned jpos = 0;
      jpos += snprintf(jbuf + jpos, MAX_JSON_LEN - jpos, "{");
      unsigned key_int = 0;
      unsigned cols = 0;
      for (unsigned col = 0; col < num_fields; col++) {
        unsigned col_is_null = !row[col] || types[col] == MYSQL_TYPE_NULL;
        if (db->config->table.strip_null && col_is_null) continue;

        if (cols > 0) jpos += snprintf(jbuf + jpos, MAX_JSON_LEN - jpos, ",");
        jpos += snprintf(jbuf + jpos, MAX_JSON_LEN - jpos, "\"%s\":", names[col]);
        if (col_is_null) {
          jpos += snprintf(jbuf + jpos, MAX_JSON_LEN - jpos, "null");
        } else {
          switch (types[col]) {
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_DATE:
            case MYSQL_TYPE_TIME:
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_VARCHAR:
            case MYSQL_TYPE_TIMESTAMP2:
            case MYSQL_TYPE_DATETIME2:
            case MYSQL_TYPE_TIME2:
            case MYSQL_TYPE_JSON:
            case MYSQL_TYPE_ENUM:
            case MYSQL_TYPE_SET:
            case MYSQL_TYPE_TINY_BLOB:
            case MYSQL_TYPE_MEDIUM_BLOB:
            case MYSQL_TYPE_LONG_BLOB:
            case MYSQL_TYPE_BLOB:
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_STRING:
            case MYSQL_TYPE_GEOMETRY:
              jpos += snprintf(jbuf + jpos, MAX_JSON_LEN - jpos, "\"%s\"", row[col]);
              break;
            default:
              jpos += snprintf(jbuf + jpos, MAX_JSON_LEN - jpos, "%s", row[col]);
              break;
          }
        }
        ++cols;
      }
      jpos += snprintf(jbuf + jpos, MAX_JSON_LEN - jpos, "}");
      ++rows;
      LOG_DEBUG("Fetched row %u: %p %u [%.*s]", rows, row, jpos, jpos, jbuf);

      unsigned frame = arena_store_framed(slot->arena, (uint8_t*)jbuf, jpos);
      LOG_DEBUG("Stored frame %p", frame);
      if (frame == (unsigned)-1) {
        LOG_WARN("Could not store framed JSON for SELECT query for table %s", table_name(table));
        break;
      }

      int insert_error = 0;
      for (unsigned idx = 0; idx < table->index_count; ++idx) {
        int col_pos = index_pos[idx];
        if (col_pos < 0) continue;
        if (!slot->indexes[idx]) continue;
        const char* value = row[col_pos];
        if (!value) continue;
        if (table->indexes[idx].type == CONFIG_INDEX_TYPE_INT) {
          key_int = (unsigned) atoi(value);
          if (!hash_insert(slot->indexes[idx], &key_int, sizeof(unsigned),
                           frame, jpos + sizeof(unsigned))) {
            LOG_WARN("Could not insert row for table %s key %u index %u", table_name(table), key_int, idx);
            insert_error = 1;
            break;
          }
          if (idx == 0) {
            if (*min_id > key_int) *min_id = key_int;
            if (*max_id < key_int) *max_id = key_int;
          }
        } else {
          unsigned hlen = strlen(value);
          if (!hlen) continue;
          if (!hash_insert(slot->indexes[idx], value, hlen, frame, jpos + sizeof(unsigned))) {
            LOG_WARN("Could not insert row for table %s key %.*s index %u", table_name(table), hlen, value, idx);
            insert_error = 1;
            break;
          }
        }
      }
      if (insert_error) break;
    }
    double t1 = now_sec();
    unsigned long elapsed = (t1 - t0) * 1000000;
    LOG_INFO("Fetched %u rows from table %s in %lu us", rows, table_name(table), elapsed);
  } while (0);

  if (result) mysql_free_result(result);
  return rows;
}

static void get_mysql_versions(DB* db) {
  db->client_version[0] = '\0';
  const char* c = mysql_get_client_info();
  if (c) strcpy(db->client_version, c);

  if (!db->conn) return;

  db->server_version[0] = '\0';
  const char* s = mysql_get_server_info((MYSQL*) db->conn);
  if (s) strcpy(db->server_version, s);
}
