#include <stdio.h>
#include <time.h>
#include "util.h"
#include "json.h"

#define CHAR_ESCAPE(c, p, l) { p[l++] = '\\'; p[l++] = c; continue; }
#define QUOTE '\"'

static unsigned json_XXX_beg(unsigned pos, const char* name, const char* delims, char* p, unsigned c, unsigned l);
static unsigned json_XXX_end(const char* delims, char* p, unsigned c, unsigned l);

unsigned json_bool(unsigned pos, const char* name, unsigned value, char* p, unsigned c, unsigned l) {
  if (pos) p[l++] = ',';
  l += snprintf(p+l, c-l, "%c%s%c:%s", QUOTE, name, QUOTE, value ? "true" : "false");
  return l;
}

unsigned json_integer(unsigned pos, const char* name, int value, char* p, unsigned c, unsigned l) {
  if (pos) p[l++] = ',';
  l += snprintf(p+l, c-l, "%c%s%c:%d", QUOTE, name, QUOTE, value);
  return l;
}

unsigned json_real(unsigned pos, const char* name, double value, unsigned dec, char* p, unsigned c, unsigned l) {
  if (pos) p[l++] = ',';
  l += snprintf(p+l, c-l, "%c%s%c:%.*f", QUOTE, name, QUOTE, dec, value);
  return l;
}

unsigned json_string(unsigned pos, const char* name, const char* value, char* p, unsigned c, unsigned l) {
  if (pos) p[l++] = ',';
  l += snprintf(p+l, c-l,   "%c%s%c:", QUOTE, name, QUOTE);
  p[l++] = QUOTE;
  for (unsigned j = 0; value[j] != '\0'; ++j) {
    char x = value[j];

    if (x == '"') CHAR_ESCAPE('"', p, l);
    if (x == '\\') CHAR_ESCAPE('\\', p, l);
    // included in standard, but don't want to
    // if (x == '/' ) CHAR_ESCAPE('/', p, l);
    if (x == '\b') CHAR_ESCAPE('b', p, l);
    if (x == '\f') CHAR_ESCAPE('f', p, l);
    if (x == '\n') CHAR_ESCAPE('n', p, l);
    if (x == '\r') CHAR_ESCAPE('r', p, l);
    if (x == '\t') CHAR_ESCAPE('t', p, l);
    p[l++] = x;
  }
  p[l++] = QUOTE;
  return l;
}

unsigned json_timestamp(unsigned pos, unsigned stamp, const char* name, char* p, unsigned c, unsigned l) {
  if (pos) p[l++] = ',';
  struct tm local;
  time_t time = stamp;
  localtime_r(&time, &local);
  l += snprintf(p+l, c-l,   "%c%s%c:%c%04u/%02u/%02u %02u:%02u:%02u%c", QUOTE, name, QUOTE, QUOTE,
                local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                local.tm_hour, local.tm_min, local.tm_sec,
                QUOTE);
  return l;
}

unsigned json_epoch(unsigned pos, const char* name, unsigned epoch, char* p, unsigned c, unsigned l) {
  l = json_obj_beg(pos, name, p, c, l);
  l = json_timestamp(0, epoch, "formatted", p, c, l);
  l = json_integer(1, "epoch", epoch, p, c, l);
  l = json_obj_end(p, c, l);
  return l;
}

unsigned json_obj_beg(unsigned pos, const char* name, char* p, unsigned c, unsigned l) {
  return json_XXX_beg(pos, name, "{}", p, c, l);
}

unsigned json_obj_end(char* p, unsigned c, unsigned l) {
  return json_XXX_end("{}", p, c, l);
}

unsigned json_arr_beg(unsigned pos, const char* name, char* p, unsigned c, unsigned l) {
  return json_XXX_beg(pos, name, "[]", p, c, l);
}

unsigned json_arr_end(char* p, unsigned c, unsigned l) {
  return json_XXX_end("[]", p, c, l);
}

static unsigned json_XXX_beg(unsigned pos, const char* name, const char* delims, char* p, unsigned c, unsigned l) {
  if (pos) p[l++] = ',';
  if (name) {
    l += snprintf(p+l, c-l, "%c%s%c:", QUOTE, name, QUOTE);
  }
  p[l++] = delims[0];
  return l;
}

static unsigned json_XXX_end(const char* delims, char* p, unsigned c, unsigned l) {
  UNUSED(c);
  p[l++] = delims[1];
  return l;
}
