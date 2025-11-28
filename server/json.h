#pragma once

unsigned json_bool(unsigned pos, const char* name, unsigned value, char* p, unsigned c, unsigned l);
unsigned json_integer(unsigned pos, const char* name, int value, char* p, unsigned c, unsigned l);
unsigned json_real(unsigned pos, const char* name, double value, unsigned dec, char* p, unsigned c, unsigned l);
unsigned json_string(unsigned pos, const char* name, const char* value, char* p, unsigned c, unsigned l);

unsigned json_timestamp(unsigned pos, unsigned stamp, const char* name, char* p, unsigned c, unsigned l);
unsigned json_epoch(unsigned pos, const char* name, unsigned epoch, char* p, unsigned c, unsigned l);

unsigned json_obj_beg(unsigned pos, const char* name, char* p, unsigned c, unsigned l);
unsigned json_obj_end(char* p, unsigned c, unsigned l);
unsigned json_arr_beg(unsigned pos, const char* name, char* p, unsigned c, unsigned l);
unsigned json_arr_end(char* p, unsigned c, unsigned l);
