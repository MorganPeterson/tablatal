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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tbt_parse_pair(const char *arg, TbtPair *out) {
    const char *eq = strchr(arg, '=');
    if (!eq) {
        fprintf(stderr, "error: expected field=value, got: %s\n", arg);
        return TBT_ERR;
    }

    size_t field_len = (size_t)(eq - arg);
    if (field_len == 0) {
        fprintf(stderr, "error: empty field name in: %s\n", arg);
        return TBT_ERR;
    }

    out->field = tbt_strdup_range(arg, 0, field_len);
    out->value = strdup(eq + 1);

    if (!out->field || !out->value) {
        free(out->field);
        free(out->value);
        return TBT_ERR;
    }

    return TBT_OK;
}

int tbt_parse_pairs_from_args(int argc, char **argv, TbtPair **out_pairs, size_t *out_count) {
    TbtPair *pairs = calloc((size_t)argc, sizeof(TbtPair));
    if (!pairs) {
        return TBT_ERR;
    }

    for (int i = 0; i < argc; i++) {
        if (tbt_parse_pair(argv[i], &pairs[i]) != TBT_OK) {
            tbt_free_pairs(pairs, (size_t)argc);
            return TBT_ERR;
        }
    }

    *out_pairs = pairs;
    *out_count = (size_t)argc;
    return TBT_OK;
}

void tbt_free_pairs(TbtPair *pairs, size_t count) {
    if (!pairs) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(pairs[i].field);
        free(pairs[i].value);
    }

    free(pairs);
}

int tbt_validate_pairs(const TbtSchema *schema, const TbtPair *pairs, size_t pair_count) {
    for (size_t i = 0; i < pair_count; i++) {
        const TbtField *field = tbt_find_field(schema, pairs[i].field);
        if (!field) {
            fprintf(stderr, "error: unknown field: %s\n", pairs[i].field);
            return TBT_ERR;
        }
    }

    return TBT_OK;
}

int tbt_parse_row_number(const char *src, size_t *out) {
    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(src, &end, 10);

    if (errno != 0 || *end != '\0' || value == 0) {
        return TBT_ERR;
    }

    *out = (size_t)value;
    return TBT_OK;
}
