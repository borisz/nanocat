#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include "options.h"

typedef struct nc_options {
    int verbose;
} nc_options_t;


struct nc_option nc_options[] = {
    // Generic options
    {"verbose", 'v', NC_OPT_INCREMENT, offsetof(nc_options_t, verbose),
     "Generic", "increase verbosity of the nanocat"},
    // Sentinel
    {NULL}
    };


int main(int argc, char **argv) {
    nc_options_t options;
    nc_parse_options(nc_options, &options);
}
