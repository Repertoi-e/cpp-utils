#pragma once

#include "../memory/dynamic_array.h"
#include "../string/string.h"

#include "file_path.h"

// #TODO Permissions, copy, move, create links

CPPU_BEGIN_NAMESPACE

struct Local_File_Path {
    // Platform dependend, don't use unless you know what you are doing
    mutable void *FileInfo;
    // Platform dependend, don't use unless you know what you are doing
    mutable void *LinkInfo;

    Local_File_Path(string const &path) {}
    ~Local_File_Path();
};

b32 exists(Local_File_Path const &path);
b32 is_file(Local_File_Path const &path);
b32 is_dir(Local_File_Path const &path);
b32 is_symbolic_link(Local_File_Path const &path);

typedef void (*Visit_Func)(Local_File_Path path);
void visit_entries(Local_File_Path const &path, Visit_Func function);

size_t file_size(Local_File_Path const &path);
u32 last_access_time(Local_File_Path const &path);
u32 last_write_time(Local_File_Path const &path);

b32 remove(Local_File_Path const &path);
b32 rename(Local_File_Path const &path, string const &name);

CPPU_END_NAMESPACE