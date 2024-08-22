#pragma once

typedef struct {
    const char *string;
    const char *source_name;
    const char *name;
} LayoutString;

/** Add a named layout to the list of available layouts. */
void AddNamedLayout(const char *name, const char *string);

/** Free the list of available layouts. */
void CleanLayouts(void);

/** Determine the layout string to use from the configuration and command line
    argument. */
LayoutString ResolveLayout(const char *layout_arg);
