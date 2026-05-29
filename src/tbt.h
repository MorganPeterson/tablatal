/*
 * tablatal - A command-line utility for creating and editing tablatal files.
 * Copyright (C) 2026 M. Peterson
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with 
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200809L

#ifndef TBT_H
#define TBT_H

#include <stdio.h>
#include <stddef.h>

#define TBT_VERSION "0.1.0"

#define TBT_OK 0
#define TBT_ERR 1

typedef struct {
    char *name;
    size_t offset;
    size_t width;
    int unbounded;
} TbtField;

typedef struct {
    char *name;
    size_t width;
    int unbounded;
} FieldSpec;

typedef struct {
    TbtField *fields;
    size_t count;
} TbtSchema;

typedef struct {
    char **items;
    size_t count;
} TbtLines;

typedef struct {
    char *field;
    char *value;
} TbtPair;

/* commands */
int tbt_cmd_init(int argc, char **argv);
int tbt_cmd_schema(int argc, char **argv);
int tbt_cmd_list(int argc, char **argv);
int tbt_cmd_add(int argc, char **argv);
int tbt_cmd_get(int argc, char **argv);
int tbt_cmd_find(int argc, char **argv);
int tbt_cmd_delete(int argc, char **argv);
int tbt_cmd_set(int argc, char **argv);

/* file helpers */
int tbt_read_first_line(const char *path, char **out);
int tbt_print_file(const char *path);
int tbt_print_file_numbered(const char *path);
int tbt_append_line(const char *path, const char *line);
int tbt_read_all_lines(const char *path, TbtLines *lines);
int tbt_write_all_lines_atomic(const char *path, const TbtLines *lines);
void tbt_free_lines(TbtLines *lines);

/* schema helpers */
int tbt_parse_schema_line(const char *line, TbtSchema *schema);
void tbt_free_schema(TbtSchema *schema);
void tbt_print_schema(const TbtSchema *schema);
const TbtField *tbt_find_field(const TbtSchema *schema, const char *name);

/* pair/query helpers */
int tbt_parse_pair(const char *arg, TbtPair *out);
int tbt_parse_pairs_from_args(int argc, char **argv, TbtPair **out_pairs, size_t *out_count);
void tbt_free_pairs(TbtPair *pairs, size_t count);
int tbt_validate_pairs(const TbtSchema *schema, const TbtPair *pairs, size_t pair_count);

/* row helpers */
char *tbt_format_row(const TbtSchema *schema, const TbtPair *pairs, size_t pair_count);
char *tbt_get_field_value(const TbtSchema *schema, const char *line, const char *field_name);
int tbt_row_to_pairs(const TbtSchema *schema, const char *line, TbtPair **out_pairs, size_t *out_count);
int tbt_apply_updates_to_pairs(TbtPair *pairs, size_t pair_count, const TbtPair *updates, size_t update_count);

/* misc helpers */
int tbt_parse_row_number(const char *src, size_t *out);
char *tbt_strdup_range(const char *src, size_t start, size_t len);
char *tbt_trimmed_dup(const char *src, size_t len);

int write_header(FILE *fp, const FieldSpec *specs, size_t count);
int parse_field_spec(const char *arg, int is_final, FieldSpec *out);
void free_specs(FieldSpec *specs, size_t count);
#endif