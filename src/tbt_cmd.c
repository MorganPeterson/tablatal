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

static int load_schema_from_file(const char *path, char **out_header, TbtSchema *out_schema) {
    char *header = NULL;

    if (tbt_read_first_line(path, &header) != TBT_OK) {
        return TBT_ERR;
    }

    if (tbt_parse_schema_line(header, out_schema) != TBT_OK) {
        free(header);
        return TBT_ERR;
    }

    *out_header = header;
    return TBT_OK;
}

int tbt_cmd_init(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: tablatal init <file> <field:width>... <final-field>\n");
        return TBT_ERR;
    }

    const char *path = argv[0];

    size_t spec_count = (size_t)(argc-1);
    FieldSpec *specs = calloc(spec_count, sizeof(FieldSpec));
    if (!specs) {
        return TBT_ERR;
    }

    for (size_t i = 0; i < spec_count; i++) {
        int is_final = i + 1 == spec_count;

        if (parse_field_spec(argv[i+1], is_final, &specs[i]) != TBT_OK) {
            free_specs(specs, spec_count);
            return TBT_ERR;
        }
    }

    FILE *fp = fopen(path, "wx");
    if (!fp) {
        perror(path);
        free_specs(specs, spec_count);
        return TBT_ERR;
    }

    int result = write_header(fp, specs, spec_count);
    if (fclose(fp) != 0) {
        perror(path);
        result = TBT_ERR;
    }

    if (result != TBT_OK) {
        fprintf(stderr, "error: failed to initialize table: %s\n", path);
    }

    free_specs(specs, spec_count);
    return result;
}

int tbt_cmd_add(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: tablatal add <file> <field=value>...\n");
        return TBT_ERR;
    }

    const char *path = argv[0];

    char *header = NULL;
    TbtSchema schema;

    if (load_schema_from_file(path, &header, &schema) != TBT_OK) {
        return TBT_ERR;
    }

    TbtPair *pairs = NULL;
    size_t pair_count = 0;

    if (tbt_parse_pairs_from_args(argc - 1, argv + 1, &pairs, &pair_count) != TBT_OK) {
        free(header);
        tbt_free_schema(&schema);
        return TBT_ERR;
    }

    char *row = tbt_format_row(&schema, pairs, pair_count);
    if (!row) {
        tbt_free_pairs(pairs, pair_count);
        free(header);
        tbt_free_schema(&schema);
        return TBT_ERR;
    }

    int result = tbt_append_line(path, row);

    free(row);
    tbt_free_pairs(pairs, pair_count);
    free(header);
    tbt_free_schema(&schema);

    return result;
}

int tbt_cmd_get(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: tablatal get <file> <row> [-n|--number]\n");
        return TBT_ERR;
    }

    const char *path = argv[0];
    int number_rows = 0;

    if (argc == 3) {
        if (strcmp(argv[2], "-n") == 0 || strcmp(argv[2], "--number") == 0) {
            number_rows = 1;
        } else {
            fprintf(stderr, "error: unknown option for get: %s\n", argv[2]);
            return TBT_ERR;
        }
    }

    size_t row_number = 0;
    if (tbt_parse_row_number(argv[1], &row_number) != TBT_OK) {
        fprintf(stderr, "error: invalid row number: %s\n", argv[1]);
        return TBT_ERR;
    }

    TbtLines lines;
    if (tbt_read_all_lines(path, &lines) != TBT_OK) {
        return TBT_ERR;
    }

    if (lines.count == 0) {
        fprintf(stderr, "error: empty table: %s\n", path);
        tbt_free_lines(&lines);
        return TBT_ERR;
    }

    size_t line_index = row_number;

    if (line_index >= lines.count) {
        fprintf(stderr, "error: row does not exist: %zu\n", row_number);
        tbt_free_lines(&lines);
        return TBT_ERR;
    }

    if (number_rows) {
        printf("%-4s %s\n", "ROW", lines.items[0]);
        printf("%-4zu %s\n", row_number, lines.items[line_index]);
    } else {
        printf("%s\n", lines.items[0]);
        printf("%s\n", lines.items[line_index]);
    }

    tbt_free_lines(&lines);
    return TBT_OK;
}

int tbt_cmd_find(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: tablatal find <file> <field=value> [-n|--number]\n");
        return TBT_ERR;
    }

    const char *path = argv[0];
    int number_rows = 0;

    if (argc == 3) {
        if (strcmp(argv[2], "-n") == 0 || strcmp(argv[2], "--number") == 0) {
            number_rows = 1;
        } else {
            fprintf(stderr, "error: unknown option for find: %s\n", argv[2]);
            return TBT_ERR;
        }
    }

    TbtPair query;
    memset(&query, 0, sizeof(query));

    if (tbt_parse_pair(argv[1], &query) != TBT_OK) {
        return TBT_ERR;
    }

    TbtLines lines;
    if (tbt_read_all_lines(path, &lines) != TBT_OK) {
        free(query.field);
        free(query.value);
        return TBT_ERR;
    }

    if (lines.count == 0) {
        fprintf(stderr, "error: empty table: %s\n", path);
        tbt_free_lines(&lines);
        free(query.field);
        free(query.value);
        return TBT_ERR;
    }

    TbtSchema schema;
    if (tbt_parse_schema_line(lines.items[0], &schema) != TBT_OK) {
        tbt_free_lines(&lines);
        free(query.field);
        free(query.value);
        return TBT_ERR;
    }

    if (!tbt_find_field(&schema, query.field)) {
        fprintf(stderr, "error: unknown field: %s\n", query.field);
        tbt_free_schema(&schema);
        tbt_free_lines(&lines);
        free(query.field);
        free(query.value);
        return TBT_ERR;
    }

    int printed_header = 0;

    for (size_t i = 1; i < lines.count; i++) {
        char *actual = tbt_get_field_value(&schema, lines.items[i], query.field);
        if (!actual) {
            tbt_free_schema(&schema);
            tbt_free_lines(&lines);
            free(query.field);
            free(query.value);
            return TBT_ERR;
        }

        if (strcmp(actual, query.value) == 0) {
            if (!printed_header) {
                if (number_rows) {
                    printf("%-4s %s\n", "ROW", lines.items[0]);
                } else {
                    printf("%s\n", lines.items[0]);
                }

                printed_header = 1;
            }

            if (number_rows) {
                printf("%-4zu %s\n", i, lines.items[i]);
            } else {
                printf("%s\n", lines.items[i]);
            }
        }

        free(actual);
    }

    tbt_free_schema(&schema);
    tbt_free_lines(&lines);
    free(query.field);
    free(query.value);

    return TBT_OK;
}

int tbt_cmd_delete(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: tablatal delete <file> <row>\n");
        return TBT_ERR;
    }

    const char *path = argv[0];

    size_t row_number = 0;
    if (tbt_parse_row_number(argv[1], &row_number) != TBT_OK) {
        fprintf(stderr, "error: invalid row number: %s\n", argv[1]);
        return TBT_ERR;
    }

    TbtLines lines;
    if (tbt_read_all_lines(path, &lines) != TBT_OK) {
        return TBT_ERR;
    }

    if (lines.count == 0) {
        fprintf(stderr, "error: empty table: %s\n", path);
        tbt_free_lines(&lines);
        return TBT_ERR;
    }

    size_t line_index = row_number;

    if (line_index >= lines.count) {
        fprintf(stderr, "error: row does not exist: %zu\n", row_number);
        tbt_free_lines(&lines);
        return TBT_ERR;
    }

    free(lines.items[line_index]);

    for (size_t i = line_index; i + 1 < lines.count; i++) {
        lines.items[i] = lines.items[i + 1];
    }

    lines.count--;

    int result = tbt_write_all_lines_atomic(path, &lines);

    tbt_free_lines(&lines);
    return result;
}

int tbt_cmd_set(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: tablatal set <file> <row> <field=value>...\n");
        return TBT_ERR;
    }

    const char *path = argv[0];

    size_t row_number = 0;
    if (tbt_parse_row_number(argv[1], &row_number) != TBT_OK) {
        fprintf(stderr, "error: invalid row number: %s\n", argv[1]);
        return TBT_ERR;
    }

    TbtPair *updates = NULL;
    size_t update_count = 0;

    if (tbt_parse_pairs_from_args(argc - 2, argv + 2, &updates, &update_count) != TBT_OK) {
        return TBT_ERR;
    }

    TbtLines lines;
    if (tbt_read_all_lines(path, &lines) != TBT_OK) {
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    if (lines.count == 0) {
        fprintf(stderr, "error: empty table: %s\n", path);
        tbt_free_lines(&lines);
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    size_t line_index = row_number;

    if (line_index >= lines.count) {
        fprintf(stderr, "error: row does not exist: %zu\n", row_number);
        tbt_free_lines(&lines);
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    TbtSchema schema;
    if (tbt_parse_schema_line(lines.items[0], &schema) != TBT_OK) {
        tbt_free_lines(&lines);
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    if (tbt_validate_pairs(&schema, updates, update_count) != TBT_OK) {
        tbt_free_schema(&schema);
        tbt_free_lines(&lines);
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    TbtPair *pairs = NULL;
    size_t pair_count = 0;

    if (tbt_row_to_pairs(&schema, lines.items[line_index], &pairs, &pair_count) != TBT_OK) {
        tbt_free_schema(&schema);
        tbt_free_lines(&lines);
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    if (tbt_apply_updates_to_pairs(pairs, pair_count, updates, update_count) != TBT_OK) {
        tbt_free_pairs(pairs, pair_count);
        tbt_free_schema(&schema);
        tbt_free_lines(&lines);
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    char *new_row = tbt_format_row(&schema, pairs, pair_count);
    if (!new_row) {
        tbt_free_pairs(pairs, pair_count);
        tbt_free_schema(&schema);
        tbt_free_lines(&lines);
        tbt_free_pairs(updates, update_count);
        return TBT_ERR;
    }

    free(lines.items[line_index]);
    lines.items[line_index] = new_row;

    int result = tbt_write_all_lines_atomic(path, &lines);

    tbt_free_pairs(pairs, pair_count);
    tbt_free_schema(&schema);
    tbt_free_lines(&lines);
    tbt_free_pairs(updates, update_count);

    return result;
}

int tbt_cmd_list(int argc, char **argv) {
    if (argc != 1 && argc != 2) {
        fprintf(stderr, "usage: tablatal list <file> [-n|--number]\n");
        return TBT_ERR;
    }

    const char *path = argv[0];

    if (argc == 2) {
        if (strcmp(argv[1], "-n") == 0 || strcmp(argv[1], "--number") == 0) {
            return tbt_print_file_numbered(path);
        }

        fprintf(stderr, "error: unknown option for list: %s\n", argv[1]);
        return TBT_ERR;
    }

    return tbt_print_file(path);
}

int tbt_cmd_schema(int argc, char **argv) {
    if (argc != 1) {
        fprintf(stderr, "usage: tablatal schema <file>\n");
        return TBT_ERR;
    }

    const char *path = argv[0];

    char *header = NULL;
    if (tbt_read_first_line(path, &header) != TBT_OK) {
        return TBT_ERR;
    }

    TbtSchema schema;
    if (tbt_parse_schema_line(header, &schema) != TBT_OK) {
        free(header);
        return TBT_ERR;
    }

    tbt_print_schema(&schema);

    tbt_free_schema(&schema);
    free(header);

    return TBT_OK;
}
