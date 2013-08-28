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

static void nc_argument_error(char *message, struct nc_parse_context *ctx,
                     struct nc_option *opt) {
    fprintf(stderr, "Option ``--%s'' %s\n", opt->longname, message);
    exit(1);
}

static void nc_option_error(char *message, struct nc_parse_context *ctx,
                     struct nc_option *opt) {
    fprintf(stderr, "Option ``--%s'' %s\n", opt->longname, message);
    exit(1);
}

static void nc_memory_error() {
    fprintf(stderr, "Memory error while parsing comand-line");
    abort();
}

static void nc_invalid_enum_value(struct nc_parse_context *ctx,
    struct nc_option *opt, char *argument, struct nc_enum_item *items)
{
    fprintf(stderr, "Invalid value for ``--%s``. Options are:\n",
        opt->longname);
    for(;items->name; ++items) {
        fprintf(stderr, "    %s\n", items->name);
    }
    exit(1);
}

static void nc_process_option(struct nc_parse_context *ctx,
                              struct nc_option *opt, char *argument) {
    struct nc_enum_item *items;
    char *endptr;
    struct nc_string_list *lst;
    struct nc_blob *blob;
    FILE *file;
    char *data;
    size_t data_len;
    size_t data_buf;
    int bytes_read;
    printf("PARSED ``%s'' with arg ``%s''\n", opt->longname, argument);

    if(ctx->mask & opt->conflicts_mask) {
        nc_option_error("conflicts with previous option", ctx, opt);
    }
    ctx->mask |= opt->mask_set;

    switch(opt->type) {
        case NC_OPT_HELP:
            nc_print_help(ctx, stdout);
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
            nc_invalid_enum_value(ctx, opt, argument,
                 (struct nc_enum_item *)opt->pointer);
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
                nc_argument_error("requires float point argument", ctx, opt);
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
                nc_memory_error();
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
                nc_memory_error();
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
                        nc_memory_error();
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
    struct nc_option *opt;
    for(opt = ctx->options; opt->longname; ++opt) {
        if(opt->arg0name && !strcmp(ctx->argv[0], opt->arg0name)) {
            assert(!nc_has_arg(opt));
            nc_process_option(ctx, opt, NULL);
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
    fprintf(stderr, "Unknown option ``%s''\n", ctx->data);
    exit(1);
}

static void nc_error_invalid_argument(struct nc_parse_context *ctx) {
    fprintf(stderr, "Invalid argument ``%s''\n", ctx->data);
    exit(1);
}

static void nc_error_unknown_short_option(struct nc_parse_context *ctx) {
    fprintf(stderr, "Unknown option ``-%c''\n", *ctx->data);
    exit(1);
}

static void nc_error_no_argument_needed(struct nc_parse_context *ctx,
                                        struct nc_option *opt) {
    fprintf(stderr, "Option ``--%s'' doesn't accept argument\n",
        opt->longname);
    exit(1);
}

static void nc_error_option_requires_argument(struct nc_parse_context *ctx,
    struct nc_option *opt, int is_long) {
    if(is_long) {
        fprintf(stderr, "Option ``--%s'' requires an argument\n",
            opt->longname);
    } else {
        fprintf(stderr, "Option ``-%c'' requires an argument\n",
            opt->shortname);
    }
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
    struct nc_option *best_match;
    char *a, *b;
    int longest_prefix;
    int cur_prefix;
    char *arg;

    arg = ctx->data+2;
    longest_prefix = 0;
    best_match = NULL;
    for(opt = ctx->options; opt->longname; ++opt) {
        for(a = opt->longname, b = arg;; ++a, ++b) {
            if(*b == 0 || *b == '=') {  // End of option on command-line
                cur_prefix = a - opt->longname;
                if(!*a) {  // Matches end of option name
                    best_match = opt;
                    longest_prefix = cur_prefix;
                    break;
                }
                if(cur_prefix == longest_prefix) {
                    best_match = NULL;  // Ambiguity
                } else if(cur_prefix > longest_prefix) {
                    best_match = opt;
                    longest_prefix = cur_prefix;
                }
                break;
            } else if(*b != *a) {
                break;
            }
        }
    }
    if(best_match) {
        if(arg[longest_prefix] == '=') {
            if(nc_has_arg(best_match)) {
                nc_process_option(ctx, best_match, arg + longest_prefix + 1);
            } else {
                nc_error_no_argument_needed(ctx, best_match);
            }
        } else {
            if(nc_has_arg(best_match)) {
                if(nc_get_arg(ctx)) {
                    nc_process_option(ctx, best_match, ctx->data);
                } else {
                    nc_error_option_requires_argument(ctx, best_match, 1);
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
    struct nc_option *opt;

    for(opt = ctx->options; opt->longname; ++opt) {
        if(!opt->shortname)
            continue;
        if(opt->shortname == *ctx->data) {
            if(nc_has_arg(opt)) {
                if(ctx->data[1]) {
                    nc_process_option(ctx, opt, ctx->data+1);
                } else {
                    if(nc_get_arg(ctx)) {
                        nc_process_option(ctx, opt, ctx->data);
                    } else {
                        nc_error_option_requires_argument(ctx, opt, 0);
                    }
                }
                ctx->data = "";  // end of short options anyway
            } else {
                nc_process_option(ctx, opt, NULL);
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

    ctx.options = options;
    ctx.target = target;
    ctx.argc = argc;
    ctx.argv = argv;

    nc_parse_arg0(&ctx);

    ctx.args_left = argc - 1;
    ctx.arg = argv;

    while(nc_get_arg(&ctx)) {
        nc_parse_arg(&ctx);
    }

}
