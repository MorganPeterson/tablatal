# tablatal

`tablatal` is a small Linux only command-line utility for working with [Tablatal](https://wiki.xxiivv.com/site/tablatal.html)-style plaintext table files.

It is designed as a clean, human-readable personal-data utility rather than a full database engine. One file is one table. The first line defines the schema. Field positions in the header define fixed-width columns. The final field is unbounded.

```txt
NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Cyan
Nike    34    Red
Ruca    45    Grey
```

## Features

- Plaintext table format
- One table per file
- Create, read, update, delete, and find rows
- Strict fixed-width fields before the final field
- Unbounded final field

## Format

A Tablatal file begins with a header row:

```txt
NAME    AGE   COLOR
```

The header defines the fields and their offsets:

```txt
FIELD   OFFSET   WIDTH
NAME    0        8
AGE     8        6
COLOR   14       unbounded
```

All fields before the final field are fixed-width. The final field is unbounded.

Given this schema:

```txt
NAME    AGE   COLOR
```

these rows are valid:

```txt
Erica   12    Opal
Alex    23    Cyan
```

The `NAME` field is 8 characters wide, `AGE` is 6 characters wide, and `COLOR` extends to the end of the line.

## Building

Requirements:

- Linux or another POSIX-like system
- C compiler such as `cc`, `gcc`, or `clang`
- `make`

Build:

```sh
make
```

This creates the `tablatal` binary:

```sh
./tablatal --version
```

Example output:

```txt
tablatal 0.1.0
```

## Installing

Install to `/usr/local/bin`:

```sh
sudo make install
```

Then run:

```sh
tablatal --version
```

Uninstall:

```sh
sudo make uninstall
```

## Running tests

Run the shell test suite:

```sh
./test.sh
```

Or, if your `Makefile` includes a test target:

```sh
make test
```

The tests cover initialization, schema parsing, adding rows, listing rows, finding rows, updating rows, deleting rows, row-number output, strict width checks, and preservation of files after failed writes.

## Usage

```txt
usage:
  tablatal init <file> <field:width>... <final-field>
  tablatal schema <file>
  tablatal list <file> [-n|--number]
  tablatal get <file> <row> [-n|--number]
  tablatal add <file> <field=value>...
  tablatal set <file> <row> <field=value>...
  tablatal delete <file> <row>
  tablatal find <file> <field=value> [-n|--number]
  tablatal version
  tablatal --version
```

## Creating a table

Use `init` to create a new table file.

```sh
tablatal init people.tbl NAME:8 AGE:6 COLOR
```

This creates:

```txt
NAME    AGE   COLOR
```

All fields except the final field require an explicit width. The final field is unbounded.

Valid:

```sh
tablatal init books.tbl TITLE:32 AUTHOR:24 STATUS
```

Invalid:

```sh
tablatal init people.tbl NAME AGE:6 COLOR
```

`NAME` is not the final field, so it must have a width.

Also invalid:

```sh
tablatal init people.tbl NAME:4 AGE:6 COLOR
```

The width for a fixed-width field must be greater than the field name length so that the header can include spacing between fields.

## Viewing the schema

```sh
tablatal schema people.tbl
```

Example output:

```txt
FIELD            OFFSET   WIDTH
NAME             0        8
AGE              8        6
COLOR            14       unbounded
```

## Adding rows

```sh
tablatal add people.tbl NAME=Erica AGE=12 COLOR=Opal
tablatal add people.tbl NAME=Alex AGE=23 COLOR=Cyan
tablatal add people.tbl NAME=Nike AGE=34 COLOR=Red
```

Result:

```txt
NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Cyan
Nike    34    Red
```

Missing fields are written as empty values:

```sh
tablatal add people.tbl NAME=Ruca COLOR=Grey
```

Unknown fields are rejected:

```sh
tablatal add people.tbl NAME=Ruca MOOD=Happy
```

## Strict field widths

Fixed-width fields reject values that are too long.

Given:

```sh
tablatal init people.tbl NAME:8 AGE:6 COLOR
```

this fails:

```sh
tablatal add people.tbl NAME=Alexandria AGE=23 COLOR=Cyan
```

because `Alexandria` is longer than the fixed width of the `NAME` field.

The final field is unbounded, so this is valid:

```sh
tablatal add people.tbl NAME=Long AGE=99 COLOR=This-is-a-long-unbounded-final-field
```

## Listing rows

Print the table as-is:

```sh
tablatal list people.tbl
```

Example:

```txt
NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Cyan
Nike    34    Red
```

Print with display-only row numbers:

```sh
tablatal list people.tbl -n
```

or:

```sh
tablatal list people.tbl --number
```

Example:

```txt
ROW  NAME    AGE   COLOR
1    Erica   12    Opal
2    Alex    23    Cyan
3    Nike    34    Red
```

Row numbers are not stored in the file. They are only shown by the CLI to make `get`, `set`, and `delete` easier to use.

## Getting one row

Rows are addressed by 1-based data row number. The header is not row `1`; the first row after the header is row `1`.

```sh
tablatal get people.tbl 2
```

Example output:

```txt
NAME    AGE   COLOR
Alex    23    Cyan
```

Show the row number too:

```sh
tablatal get people.tbl 2 -n
```

Example:

```txt
ROW  NAME    AGE   COLOR
2    Alex    23    Cyan
```

## Finding rows

`find` performs an exact match against a field value.

```sh
tablatal find people.tbl COLOR=Cyan
```

Example output:

```txt
NAME    AGE   COLOR
Alex    23    Cyan
```

Find with row numbers:

```sh
tablatal find people.tbl COLOR=Cyan -n
```

Example:

```txt
ROW  NAME    AGE   COLOR
2    Alex    23    Cyan
```

If no rows match, `find` exits successfully and prints no output.

## Updating rows

Use `set` with a row number and one or more `field=value` assignments.

```sh
tablatal set people.tbl 2 COLOR=Blue
```

Before:

```txt
NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Cyan
Nike    34    Red
```

After:

```txt
NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Blue
Nike    34    Red
```

Update multiple fields:

```sh
tablatal set people.tbl 1 AGE=13 COLOR=Green
```

Unknown fields are rejected. Values that exceed fixed-width fields are rejected, and the file is left unchanged.

## Deleting rows

Delete a row by 1-based data row number:

```sh
tablatal delete people.tbl 2
```

Before:

```txt
NAME    AGE   COLOR
Erica   12    Opal
Alex    23    Cyan
Nike    34    Red
```

After:

```txt
NAME    AGE   COLOR
Erica   12    Opal
Nike    34    Red
```

## Row numbers

Row numbers are operational handles used by the CLI.

They are available with:

```sh
tablatal list people.tbl -n
tablatal get people.tbl 2 -n
tablatal find people.tbl COLOR=Cyan -n
```

They are not part of the table and are not written to the file.

This keeps the underlying Tablatal file clean and faithful to the plaintext format.

## Version

```sh
tablatal --version
```

or:

```sh
tablatal version
```

Example:

```txt
tablatal 0.1.0
```

## Project structure

```txt
tablatal/
  Makefile
  test.sh
  src/
    main.c
    tbt.h
    tbt_cmd.c
    tbt_file.c
    tbt_init.c
    tbt_pair.c
    tbt_row.c
    tbt_schema.c
```

Suggested responsibilities:

```txt
main.c          command dispatch
tbt_init.c      init/list command
tbt_cmd.c       row-oriented commands: add/get/find/set/delete
tbt_file.c      reading/writing files
tbt_schema.c    parsing/printing schema
tbt_pair.c      field=value parsing and row-number parsing
tbt_row.c       row formatting and field extraction
```

## Design choices

### One table per file

`tablatal` intentionally treats one file as one table. It does not implement a multi-table database format.

### Plaintext first

The table remains readable and editable in a normal text editor.

### No hidden metadata

The file does not store hidden schema metadata, row IDs, indexes, or configuration blocks.

### Final field is unbounded

The width of the final field cannot be inferred from the header without relying on trailing spaces, which many editors remove. To keep the format robust, the final field is unbounded.

### Strict fixed-width fields

All non-final fields have strict widths. Values that do not fit are rejected instead of truncated.

### Display-only row numbers

Row numbers are shown by the CLI when requested, but they are not written to the file.

## Limitations

`tablatal` is intentionally small and not meant as a full database replacement.
