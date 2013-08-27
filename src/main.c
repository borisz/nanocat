#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include "options.h"

typedef struct nc_options {
    int verbose;
    int socket_type;
} nc_options_t;


struct nc_option nc_options[] = {
    // Generic options
    {"verbose", 'v', NULL, NC_OPT_INCREMENT, offsetof(nc_options_t, verbose),
     "Generic", "Increase verbosity of the nanocat"},
    // Socket types
    {"push", 'p', NULL, NC_OPT_SET, offsetof(nc_options_t, socket_type),
     "Socket Types", "Use PUSH socket type"},
    // Sentinel
    {NULL}
    };


int main(int argc, char **argv) {
    nc_options_t options;
    nc_parse_options(nc_options, &options);
}
