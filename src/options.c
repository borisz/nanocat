#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "options.h"

struct nc_parse_context {
    //  Initial state
    struct nc_option *options;
    void *target;
    int argc;
    char **argv;

    //  Current values
    unsigned long mask;
    int args_left;
    char **arg;
    char *data;
    char **last_option_usage;
};

static int nc_has_arg(struct nc_option *opt) {
    switch(opt->type) {
        case NC_OPT_INCREMENT:
        case NC_OPT_DECREMENT:
        case NC_OPT_SET_ENUM:
        case NC_OPT_HELP:
            return 0;
        case NC_OPT_ENUM:
        case NC_OPT_STRING:
        case NC_OPT_BLOB:
        case NC_OPT_FLOAT:
        case NC_OPT_STRING_LIST:
        case NC_OPT_READ_FILE:
            return 1;
    }
    abort();
}

static void nc_print_help(struct nc_parse_context *ctx, FILE *stream) {
    fprintf(stream, "Usage:\n");
}

static void nc_print_option(struct nc_parse_context *ctx, int opt_index,
                            FILE *stream)
{
    char *ousage;
    char *oend;
    int olen;
    struct nc_option *opt;

    opt = &ctx->options[opt_index];
    ousage = ctx->last_option_usage[opt_index];
    if(*ousage == '-') {  // Long option
        oend = strchr(ousage, '=');
        if(!oend) {
            olen = strlen(ousage);
        } else {
            olen = (oend - ousage);
        }
        if(olen != strlen(opt->longname)+2) {
            fprintf(stream, " %.*s[%s] ",
                olen, ousage, opt->longname + (olen-2));
        } else {
            fprintf(stream, " %s ", ousage);
        }
    } else {  // Short option
        fprintf(stream, " -%c (--%s) ",
            *ousage, opt->longname);
    }
}

static void nc_option_error(char *message, struct nc_parse_context *ctx,
                     int opt_index)
{
    fprintf(stderr, "%s: Option", ctx->argv[0]);
    nc_print_option(ctx, opt_index, stderr);
    fprintf(stderr, "%s\n", message);
    exit(1);
}


static void nc_memory_error(struct nc_parse_context *ctx) {
    fprintf(stderr, "%s: Memory error while parsing command-line",
        ctx->argv[0]);
    abort();
}

static void nc_invalid_enum_value(struct nc_parse_context *ctx,
    int opt_index, char *argument)
{
    struct nc_option *opt;
    struct nc_enum_item *items;

    opt = &ctx->options[opt_index];
    items = (struct nc_enum_item *)opt->pointer;
    fprintf(stderr, "%s: Invalid value for", ctx->argv[0]);
    nc_print_option(ctx, opt_index, stderr);
    fprintf(stderr, ". Options are:\n");
    for(;items->name; ++items) {
        fprintf(stderr, "    %s\n", items->name);
    }
    exit(1);
}

static void nc_option_conflict(struct nc_parse_context *ctx,
                              int opt_index) {
    unsigned long mask;
    int i;
    int num_conflicts;
    struct nc_option *opt;

    fprintf(stderr, "%s: Option", ctx->argv[0]);
    nc_print_option(ctx, opt_index, stderr);
    fprintf(stderr, "conflicts with the following options:\n");

    mask = ctx->options[opt_index].conflicts_mask;
    num_conflicts = 0;
    for(i = 0;; ++i) {
        opt = &ctx->options[i];
        if(!opt->longname)
            break;
        if(i == opt_index)
            continue;
        if(ctx->last_option_usage[i] && opt->mask_set & mask) {
            num_conflicts += 1;
            fprintf(stderr, "   ");
            nc_print_option(ctx, i, stderr);
            fprintf(stderr, "\n");
        }
    }
    if(!num_conflicts) {
        fprintf(stderr, "   ");
        nc_print_option(ctx, opt_index, stderr);
        fprintf(stderr, "\n");
    }
    exit(1);
}

static void nc_process_option(struct nc_parse_context *ctx,
                              int opt_index, char *argument) {
    struct nc_option *opt;
    struct nc_enum_item *items;
    char *endptr;
    struct nc_string_list *lst;
    struct nc_blob *blob;
    FILE *file;
    char *data;
    size_t data_len;
    size_t data_buf;
    int bytes_read;

    opt = &ctx->options[opt_index];
    if(ctx->mask & opt->conflicts_mask) {
        nc_option_conflict(ctx, opt_index);
    }
    ctx->mask |= opt->mask_set;

    switch(opt->type) {
        case NC_OPT_HELP:
            nc_print_help(ctx, stdout);
            exit(0);
            return;
        case NC_OPT_INCREMENT:
            *(int *)(((char *)ctx->target) + opt->offset) += 1;
            return;
        case NC_OPT_DECREMENT:
            *(int *)(((char *)ctx->target) + opt->offset) -= 1;
            return;
        case NC_OPT_ENUM:
            items = (struct nc_enum_item *)opt->pointer;
            for(;items->name; ++items) {
                if(!strcmp(items->name, argument)) {
                    *(int *)(((char *)ctx->target) + opt->offset) = \
                        items->value;
                    return;
                }
            }
            nc_invalid_enum_value(ctx, opt_index, argument);
            return;
        case NC_OPT_SET_ENUM:
            *(int *)(((char *)ctx->target) + opt->offset) = \
                *(int *)(opt->pointer);
            return;
        case NC_OPT_STRING:
            *(char **)(((char *)ctx->target) + opt->offset) = argument;
            return;
        case NC_OPT_BLOB:
            blob = (struct nc_blob *)(((char *)ctx->target) + opt->offset);
            blob->data = argument;
            blob->length = strlen(argument);
            return;
        case NC_OPT_FLOAT:
            *(float *)(((char *)ctx->target) + opt->offset) = strtof(argument,
                &endptr);
            if(endptr == argument || *endptr != 0) {
                nc_option_error("requires float point argument",
                                ctx, opt_index);
            }
            return;
        case NC_OPT_STRING_LIST:
            lst = (struct nc_string_list *)(
                ((char *)ctx->target) + opt->offset);
            if(lst->items) {
                lst->num += 1;
                lst->items = realloc(lst->items, sizeof(char *)*lst->num);
            } else {
                lst->items = malloc(sizeof(char *));
                lst->num = 1;
            }
            if(!lst->items) {
                nc_memory_error(ctx);
            }
            lst->items[lst->num-1] = argument;
            return;
        case NC_OPT_READ_FILE:
            if(!strcmp(argument, "-")) {
                file = stdin;
            } else {
                file = fopen(argument, "r");
                if(!file) {
                    fprintf(stderr, "Error opening file ``%s'': %s\n",
                        argument, strerror(errno));
                    exit(2);
                }
            }
            data = malloc(4096);
            if(!data)
                nc_memory_error(ctx);
            data_len = 0;
            data_buf = 4096;
            for(;;) {
                bytes_read = fread(data + data_len, 1, data_buf - data_len,
                                   file);
                printf("DATALEN %d\n", bytes_read);
                data_len += bytes_read;
                if(feof(file))
                    break;
                if(data_buf - data_len < 1024) {
                    if(data_buf < (1 << 20)) {
                        data_buf *= 2;  // grow twice until not too big
                    } else {
                        data_buf += 1 << 20;  // grow 1 Mb each time
                    }
                    data = realloc(data, data_buf);
                    if(!data)
                        nc_memory_error(ctx);
                }
            }
            if(data_len != data_buf) {
                data = realloc(data, data_len);
                assert(data);
            }
            if(ferror(file)) {
                fprintf(stderr, "Error reading file ``%s'': %s\n",
                    argument, strerror(errno));
                exit(2);
            }
            if(file != stdin) {
                fclose(file);
            }
            blob = (struct nc_blob *)(((char *)ctx->target) + opt->offset);
            blob->data = data;
            blob->length = data_len;
            return;
    }
    abort();
}

static void nc_parse_arg0(struct nc_parse_context *ctx) {
    int i;
    struct nc_option *opt;

    for(i = 0;; ++i) {
        opt = &ctx->options[i];
        if(!opt->longname)
            return;
        if(opt->arg0name && !strcmp(ctx->argv[0], opt->arg0name)) {
            assert(!nc_has_arg(opt));
            nc_process_option(ctx, i, NULL);
        }
    }
}


static void nc_error_ambiguous_option(struct nc_parse_context *ctx) {
    struct nc_option *opt;
    char *a, *b;
    char *arg;

    arg = ctx->data+2;
    fprintf(stderr, "%s: Ambiguous option ``%s'':\n", ctx->argv[0], ctx->data);
    for(opt = ctx->options; opt->longname; ++opt) {
        for(a = opt->longname, b = arg; ; ++a, ++b) {
            if(*b == 0 || *b == '=') {  // End of option on command-line
                fprintf(stderr, "    %s\n", opt->longname);
                break;
            } else if(*b != *a) {
                break;
            }
        }
    }
    exit(1);
}

static void nc_error_unknown_long_option(struct nc_parse_context *ctx) {
    fprintf(stderr, "%s: Unknown option ``%s''\n", ctx->argv[0], ctx->data);
    exit(1);
}

static void nc_error_invalid_argument(struct nc_parse_context *ctx) {
    fprintf(stderr, "%s: Invalid argument ``%s''\n", ctx->argv[0], ctx->data);
    exit(1);
}

static void nc_error_unknown_short_option(struct nc_parse_context *ctx) {
    fprintf(stderr, "%s: Unknown option ``-%c''\n", ctx->argv[0], *ctx->data);
    exit(1);
}

static int nc_get_arg(struct nc_parse_context *ctx) {
    if(!ctx->args_left)
        return 0;
    ctx->args_left -= 1;
    ctx->arg += 1;
    ctx->data = *ctx->arg;
    return 1;
}

static void nc_parse_long_option(struct nc_parse_context *ctx) {
    struct nc_option *opt;
    char *a, *b;
    int longest_prefix;
    int cur_prefix;
    int best_match;
    char *arg;
    int i;

    arg = ctx->data+2;
    longest_prefix = 0;
    best_match = -1;
    for(i = 0;; ++i) {
        opt = &ctx->options[i];
        if(!opt->longname)
            break;
        for(a = opt->longname, b = arg;; ++a, ++b) {
            if(*b == 0 || *b == '=') {  // End of option on command-line
                cur_prefix = a - opt->longname;
                if(!*a) {  // Matches end of option name
                    best_match = i;
                    longest_prefix = cur_prefix;
                    break;
                }
                if(cur_prefix == longest_prefix) {
                    best_match = -1;  // Ambiguity
                } else if(cur_prefix > longest_prefix) {
                    best_match = i;
                    longest_prefix = cur_prefix;
                }
                break;
            } else if(*b != *a) {
                break;
            }
        }
    }
    if(best_match >= 0) {
        opt = &ctx->options[best_match];
        ctx->last_option_usage[best_match] = ctx->data;
        if(arg[longest_prefix] == '=') {
            if(nc_has_arg(opt)) {
                nc_process_option(ctx, best_match, arg + longest_prefix + 1);
            } else {
                nc_option_error("does not accept argument", ctx, best_match);
            }
        } else {
            if(nc_has_arg(opt)) {
                if(nc_get_arg(ctx)) {
                    nc_process_option(ctx, best_match, ctx->data);
                } else {
                    nc_option_error("requires an argument", ctx, best_match);
                }
            } else {
                nc_process_option(ctx, best_match, NULL);
            }
        }
    } else if(longest_prefix > 0) {
        nc_error_ambiguous_option(ctx);
    } else {
        nc_error_unknown_long_option(ctx);
    }
}

static void nc_parse_short_option(struct nc_parse_context *ctx) {
    int i;
    struct nc_option *opt;

    for(i = 0;; ++i) {
        opt = &ctx->options[i];
        if(!opt->longname)
            break;
        if(!opt->shortname)
            continue;
        if(opt->shortname == *ctx->data) {
            ctx->last_option_usage[i] = ctx->data;
            if(nc_has_arg(opt)) {
                if(ctx->data[1]) {
                    nc_process_option(ctx, i, ctx->data+1);
                } else {
                    if(nc_get_arg(ctx)) {
                        nc_process_option(ctx, i, ctx->data);
                    } else {
                        nc_option_error("requires an argument", ctx, i);
                    }
                }
                ctx->data = "";  // end of short options anyway
            } else {
                nc_process_option(ctx, i, NULL);
                ctx->data += 1;
            }
            return;
        }
    }
    nc_error_unknown_short_option(ctx);
}


static void nc_parse_arg(struct nc_parse_context *ctx) {
    if(ctx->data[0] == '-') {  // an option
        if(ctx->data[1] == '-') {  // long option
            if(ctx->data[2] == 0) {  // end of options
                return;
            }
            nc_parse_long_option(ctx);
        } else {
            ctx->data += 1;  // Skip minus
            while(*ctx->data) {
                nc_parse_short_option(ctx);
            }
        }
    } else {
        nc_error_invalid_argument(ctx);
    }
}

void nc_parse_options(struct nc_option *options, void *target,
                      int argc, char **argv) {
    struct nc_parse_context ctx;
    int num_options;

    ctx.options = options;
    ctx.target = target;
    ctx.argc = argc;
    ctx.argv = argv;

    nc_parse_arg0(&ctx);

    ctx.args_left = argc - 1;
    ctx.arg = argv;

    for(num_options = 0; ctx.options[num_options].longname; ++num_options);
    ctx.last_option_usage = calloc(sizeof(char *), num_options);
    if(!ctx.last_option_usage)
        nc_memory_error(&ctx);

    while(nc_get_arg(&ctx)) {
        nc_parse_arg(&ctx);
    }

    free(ctx.last_option_usage);

}
