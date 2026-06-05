#include "dotenv.h"
#include "nob.h"
#include "path.h"

bool parse_dotenv(Variables *variables, const Path folder) {
    if (get_path_type(folder) != PT_DIR)
        return false;
    String_Builder sb = {0};
    String_Builder filepath = sb_from_path_with_file(folder, sv_from_cstr(".env"));
    if (!read_entire_file(filepath.data, &sb)) {
        sb_free(sb);
        sb_free(filepath);
        return false;
    }
    String_View sv = sb_to_sv(sb);
    while (sv.count > 0) {
        String_View line = sv_chop_by_delim(&sv, '\n');
        line = sv_trim(line);

        if ((line.count == 0) ||
            (line.count > 0 && line.data[0] == '#') ||
            (line.count > 1 && line.data[0] == '/' && line.data[1] == '/'))
            continue;

        const String_View key = sv_chop_by_delim(&line, '=');
        if (line.count == 0) {
            nob_log(NOB_WARNING, "Missing value for key: %.*s", (int) key.count, key.data);
            continue;
        }

        const KeyValuePair *found = nullptr;
        for (size_t i = 0; i < variables->count; ++i) {
            if (sv_eq(variables->items[i].key, key)) {
                found = &variables->items[i];
                break;
            }
        }

        if (found) {
            nob_log(NOB_WARNING, "Duplicate key: %.*s with value: %.*s from %s\n\tUsing value: %.*s from %.*s",
                    (int) key.count, key.data, (int) line.count, line.data, filepath.data, (int) found->value.count,
                    found->value.data, (int) found->path.count, found->path.data);
            continue;
        }

        sv_chop_prefix(&line, sv_from_cstr("\""));
        sv_chop_suffix(&line, sv_from_cstr("\""));

        const KeyValuePair kv = {key, line, sv_from_cstr(filepath.data)};

        da_append(variables, kv);
    }
    // Keep the backing buffers alive: kv.key/value point into `sb`, kv.path
    // into `filepath`. vars_free releases them.
    da_append(&variables->backings, sb.items);
    da_append(&variables->backings, filepath.items);
    return true;
}

void vars_free(Variables *variables) {
    for (size_t i = 0; i < variables->backings.count; ++i)
        free(variables->backings.items[i]);
    da_free(variables->backings);
    da_free(*variables);
    *variables = (Variables) {0};
}
