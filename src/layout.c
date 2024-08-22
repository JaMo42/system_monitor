#include "layout.h"
#include "config.h"
#include "vector.h"

static VECTOR(LayoutString) layouts = NULL;

static const LayoutString DEFAULT = {
    .string = "(rows 33% c[2] (cols (rows m[1] n[0]) p[3]))",
    .source_name = "<builtin layout>",
    .name = "default",
};

static const LayoutString FULL = {
    .string = "(rows 40% c[2] (cols (rows (cols m[1] t[1]) (cols n[0] d[0])) "
              "(rows 100% p[3] b[2])))",
    .source_name = "<builtin full layout>",
    .name = "full",
};

void
AddNamedLayout(const char *name, const char *string) {
    LayoutString layout = {string, name, name};
    vector_push(layouts, layout);
}

void
CleanLayouts(void) {
    vector_free(layouts);
    layouts = NULL;
}

static const LayoutString *
GetLayout(const char *name) {
    for (size_t i = 0; i < vector_size(layouts); ++i) {
        if (!strcmp(layouts[i].source_name, name)) {
            return &layouts[i];
        }
    }
    if (strcmp(name, "default") == 0) {
        return &DEFAULT;
    } else if (strcmp(name, "full") == 0) {
        return &FULL;
    }
    return NULL;
}

static const LayoutString *
GetLayoutRepeated(const char *string) {
    const LayoutString *layout = GetLayout(string), *ok = NULL;
    VECTOR(const LayoutString *) seen = NULL;
    while (layout) {
        ok = layout;
        vector_push(seen, layout);
        layout = GetLayout(layout->string);
        for (size_t i = 0; i < vector_size(seen); ++i) {
            if (seen[i] == layout) {
                fprintf(
                    stderr,
                    "\x1b[1;31merror:\x1b[m recursive layout: %s\n",
                    string
                );
                vector_free(seen);
                exit(1);
            }
        }
    }
    vector_free(seen);
    return ok;
}

LayoutString
ResolveLayout(const char *layout_arg) {
    const char *string = NULL;
    const char *source = NULL;
    if (layout_arg == NULL || *layout_arg == '\0') {
        Config_Read_Value *v;
        if (HaveConfig() && (v = ConfigGet("sm", "layout"))) {
            string = v->as_string();
            source = "sm.ini";
        } else {
            string = "default";
        }
    } else {
        string = layout_arg;
        source = "-l";
    }
    const LayoutString *named = GetLayoutRepeated(string);
    if (named) {
        return *named;
    }
    return (LayoutString){
        .string = string,
        .source_name = source,
        // We don't care about the name at this point
        .name = NULL,
    };
}
