#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "options.h"

struct nc_parse_context {
    //  Initial state
    struct nc_option *options;
    void *target;
    int argc;
    char **argv;

    //  Current values
    int args_left;
    char **arg;
    char *data;
};

static int nc_has_arg(struct nc_option *opt) {
    switch(opt->type) {
        case NC_OPT_INCREMENT:
        case NC_OPT_DECREMENT:
        case NC_OPT_SET_ENUM:
            return 0;
        case NC_OPT_ENUM:
        case NC_OPT_STRING:
        case NC_OPT_FLOAT:
        case NC_OPT_STRING_LIST:
        case NC_OPT_READ_FILE:
            return 1;
    }
    abort();
}

static void nc_process_option(struct nc_parse_context *ctx,
                              struct nc_option *opt, char *argument) {
    printf("PARSED ``%s'' with arg ``%s''\n", opt->longname, argument);
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
