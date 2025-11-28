#pragma once

// A Config stores the configuration for the system.
// The default values can be modified by using environment variables.

typedef struct ConfigMySQL {
  const char* host;
  unsigned port;
  const char* database;
  const char* user;
  const char* password;
} ConfigMySQL;

typedef struct ConfigSocket {
  const char* host;
  unsigned port;
  const char* path;
} ConfigSocket;

#define MELIAN_MAX_TABLES 64
#define MELIAN_MAX_INDEXES 16
#define MELIAN_MAX_NAME_LEN 256

typedef enum ConfigIndexType {
  CONFIG_INDEX_TYPE_INT,
  CONFIG_INDEX_TYPE_STRING,
} ConfigIndexType;

typedef struct ConfigIndexSpec {
  unsigned id;
  char column[MELIAN_MAX_NAME_LEN];
  ConfigIndexType type;
} ConfigIndexSpec;

typedef struct ConfigTableSpec {
  unsigned id;
  char name[MELIAN_MAX_NAME_LEN];
  unsigned period;
  unsigned index_count;
  ConfigIndexSpec indexes[MELIAN_MAX_INDEXES];
} ConfigTableSpec;

typedef struct ConfigTable {
  unsigned period;
  unsigned strip_null;
  char* schema;
  unsigned table_count;
  ConfigTableSpec tables[MELIAN_MAX_TABLES];
} ConfigTable;

typedef struct ConfigServer {
  unsigned show_msgs;
} ConfigServer;

typedef struct Config {
  ConfigMySQL mysql;
  ConfigSocket socket;
  ConfigTable table;
  ConfigServer server;
} Config;

Config* config_build(void);
void config_destroy(Config* config);
void config_show_usage(void);
