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

#include "tbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *find_pair_value(const TbtPair *pairs, size_t pair_count, const char *field) {
    for (size_t i = 0; i < pair_count; i++) {
        if (strcmp(pairs[i].field, field) == 0) {
            return pairs[i].value;
        }
    }

    return "";
}

char *tbt_format_row(const TbtSchema *schema, const TbtPair *pairs, size_t pair_count) {
    if (tbt_validate_pairs(schema, pairs, pair_count) != TBT_OK) {
        return NULL;
    }

    size_t total = 0;

    for (size_t i = 0; i < schema->count; i++) {
        const TbtField *field = &schema->fields[i];
        const char *value = find_pair_value(pairs, pair_count, field->name);
        size_t value_len = strlen(value);

        if (field->unbounded) {
            total += value_len;
        } else {
            if (value_len > field->width) {
                fprintf(stderr,
                    "error: value for field %s is too long\n"
                    "field width: %zu\n"
                    "value length: %zu\n"
                    "value: %s\n",
                    field->name,
                    field->width,
                    value_len,
                    value
                );
                return NULL;
            }

            total += field->width;
        }
    }

    char *row = malloc(total + 1);
    if (!row) {
        return NULL;
    }

    size_t pos = 0;

    for (size_t i = 0; i < schema->count; i++) {
        const TbtField *field = &schema->fields[i];
        const char *value = find_pair_value(pairs, pair_count, field->name);
        size_t value_len = strlen(value);

        if (field->unbounded) {
            memcpy(row + pos, value, value_len);
            pos += value_len;
            continue;
        }

        memcpy(row + pos, value, value_len);
        pos += value_len;

        while (pos < field->offset + field->width) {
            row[pos++] = ' ';
        }
    }

    row[pos] = '\0';
    return row;
}

char *tbt_get_field_value(const TbtSchema *schema, const char *line, const char *field_name) {
    const TbtField *field = tbt_find_field(schema, field_name);
    if (!field) {
        fprintf(stderr, "error: unknown field: %s\n", field_name);
        return NULL;
    }

    size_t line_len = strlen(line);

    if (field->offset >= line_len) {
        return strdup("");
    }

    if (field->unbounded) {
        return tbt_trimmed_dup(line + field->offset, line_len - field->offset);
    }

    size_t available = line_len - field->offset;
    size_t width = field->width;

    if (available < width) {
        width = available;
    }

    return tbt_trimmed_dup(line + field->offset, width);
}

int tbt_row_to_pairs(const TbtSchema *schema, const char *line, TbtPair **out_pairs, size_t *out_count) {
    TbtPair *pairs = calloc(schema->count, sizeof(TbtPair));
    if (!pairs) {
        return TBT_ERR;
    }

    for (size_t i = 0; i < schema->count; i++) {
        const TbtField *field = &schema->fields[i];

        pairs[i].field = strdup(field->name);
        pairs[i].value = tbt_get_field_value(schema, line, field->name);

        if (!pairs[i].field || !pairs[i].value) {
            tbt_free_pairs(pairs, schema->count);
            return TBT_ERR;
        }
    }

    *out_pairs = pairs;
    *out_count = schema->count;
    return TBT_OK;
}

int tbt_apply_updates_to_pairs(TbtPair *pairs, size_t pair_count, const TbtPair *updates, size_t update_count) {
    for (size_t u = 0; u < update_count; u++) {
        int found = 0;

        for (size_t p = 0; p < pair_count; p++) {
            if (strcmp(pairs[p].field, updates[u].field) == 0) {
                char *next = strdup(updates[u].value);
                if (!next) {
                    return TBT_ERR;
                }

                free(pairs[p].value);
                pairs[p].value = next;
                found = 1;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "error: unknown field: %s\n", updates[u].field);
            return TBT_ERR;
        }
    }

    return TBT_OK;
}
