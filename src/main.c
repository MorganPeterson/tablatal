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
#include <string.h>

static void print_usage(FILE *out) {
    fprintf(out,
        "usage:\n"
        "  tablatal init <file> <field:width>... <final-field>\n"
        "  tablatal schema <file>\n"
        "  tablatal list <file> [-n|--number]\n"
        "  tablatal get <file> <row> [-n|--number]\n"
        "  tablatal add <file> <field=value>...\n"
        "  tablatal set <file> <row> <field=value>...\n"
        "  tablatal delete <file> <row>\n"
        "  tablatal find <file> <field=value> [-n|--number]\n"
        "\n"
        "format:\n"
        "  The first line is the schema/header.\n"
        "  Header field positions define column offsets.\n"
        "  All fields before the final field are fixed-width.\n"
        "  The final field is unbounded.\n"
        "\n"
    );
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(stderr);
        return TBT_ERR;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(stdout);
        return TBT_OK;
    }

    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0) {
        printf("tablatal %s\n", TBT_VERSION);
        return TBT_OK;
    }

    if (strcmp(cmd, "init") == 0) {
        return tbt_cmd_init(argc-2, argv+2);
    }

    if (strcmp(cmd, "schema") == 0) {
        return tbt_cmd_schema(argc-2, argv+2);
    }

    if (strcmp(cmd, "list") == 0) {
        return tbt_cmd_list(argc-2, argv+2);
    }

    if (strcmp(cmd, "add") == 0) {
        return tbt_cmd_add(argc-2, argv+2);
    }

    if (strcmp(cmd, "get") == 0) {
        return tbt_cmd_get(argc-2, argv+2);
    }

    if (strcmp(cmd, "find") == 0) {
        return tbt_cmd_find(argc-2, argv+2);
    }

    if (strcmp(cmd, "delete") == 0) {
        return tbt_cmd_delete(argc-2, argv+2);
    }

    if (strcmp(cmd, "set") == 0) {
        return tbt_cmd_set(argc-2, argv+2);
    }
    
    fprintf(stderr, "error: unknown command: %s\n\n", cmd);
    print_usage(stderr);
    return TBT_ERR;
}