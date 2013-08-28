#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/bus.h>
#include <nanomsg/pair.h>
#include <nanomsg/survey.h>
#include <nanomsg/reqrep.h>

#include "options.h"

enum echo_format {
    NC_NO_ECHO,
    NC_ECHO_RAW,
    NC_ECHO_ASCII,
    NC_ECHO_QUOTED,
    NC_ECHO_MSGPACK
};

typedef struct nc_options {
    // Global options
    int verbose;

    // Socket options
    int socket_type;
    struct nc_string_list bind_addresses;
    struct nc_string_list connect_addresses;
    float timeout;
    struct nc_string_list subscriptions;

    // Data options
    struct nc_blob data_to_send;

    // Echo options
    enum echo_format echo_format;
} nc_options_t;

//  Constants to get address of in option declaration
static const int nn_push = NN_PUSH;
static const int nn_pull = NN_PULL;
static const int nn_pub = NN_PUB;
static const int nn_sub = NN_SUB;
static const int nn_req = NN_REQ;
static const int nn_rep = NN_REP;
static const int nn_bus = NN_BUS;
static const int nn_pair = NN_PAIR;
static const int nn_surveyor = NN_SURVEYOR;
static const int nn_respondent = NN_RESPONDENT;


struct nc_enum_item socket_types[] = {
    {"PUSH", NN_PUSH},
    {"PULL", NN_PULL},
    {"PUB", NN_PUB},
    {"SUB", NN_SUB},
    {"REQ", NN_REQ},
    {"REP", NN_REP},
    {"BUS", NN_BUS},
    {"PAIR", NN_PAIR},
    {"SURVEYOR", NN_SURVEYOR},
    {"RESPONDENT", NN_RESPONDENT},
    {NULL, 0},
};


//  Constants to get address of in option declaration
static const int nc_echo_raw = NC_ECHO_RAW;
static const int nc_echo_ascii = NC_ECHO_ASCII;
static const int nc_echo_quoted = NC_ECHO_QUOTED;
static const int nc_echo_msgpack = NC_ECHO_MSGPACK;

struct nc_enum_item echo_formats[] = {
    {"no", NC_NO_ECHO},
    {"raw", NC_ECHO_RAW},
    {"ascii", NC_ECHO_ASCII},
    {"quoted", NC_ECHO_QUOTED},
    {"msgpack", NC_ECHO_MSGPACK},
    {NULL, 0},
};

//  Constants for conflict masks
#define NC_MASK_SOCK 1
#define NC_MASK_WRITEABLE 2
#define NC_MASK_READABLE 4
#define NC_MASK_SOCK_SUB 8
#define NC_MASK_DATA 16
#define NC_NO_PROVIDES 0
#define NC_NO_CONFLICTS 0
#define NC_NO_REQUIRES 0
#define NC_MASK_SOCK_WRITEABLE (NC_MASK_SOCK | NC_MASK_WRITEABLE)
#define NC_MASK_SOCK_READABLE (NC_MASK_SOCK | NC_MASK_READABLE)
#define NC_MASK_SOCK_READWRITE  (NC_MASK_SOCK_WRITEABLE|NC_MASK_SOCK_READABLE)

struct nc_option nc_options[] = {
    // Generic options
    {"verbose", 'v', NULL,
     NC_OPT_INCREMENT, offsetof(nc_options_t, verbose), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Generic", NULL, "Increase verbosity of the nanocat"},
    {"silent", 'q', NULL,
     NC_OPT_DECREMENT, offsetof(nc_options_t, verbose), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Generic", NULL, "Decrease verbosity of the nanocat"},
    {"help", 'h', NULL,
     NC_OPT_HELP, 0, NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Generic", NULL, "This help text"},

    // Socket types
    {"push", 'p', "nn_push",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_push,
     NC_MASK_SOCK_WRITEABLE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PUSH socket type"},
    {"pull", 'P', "nn_pull",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_pull,
     NC_MASK_SOCK_READABLE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PULL socket type"},
    {"pub", 'S', "nn_pub",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_pub,
     NC_MASK_SOCK_WRITEABLE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PUB socket type"},
    {"sub", 's', "nn_sub",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_sub,
     NC_MASK_SOCK_READABLE|NC_MASK_SOCK_SUB, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_SUB socket type"},
    {"req", 'R', "nn_req",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_req,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_REQ socket type"},
    {"rep", 'r', "nn_rep",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_rep,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_REP socket type"},
    {"surveyor", 'U', "nn_surveyor",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_surveyor,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_SURVEYOR socket type"},
    {"respondent", 'u', "nn_respondent",
     NC_OPT_SET_ENUM, offsetof(nc_options_t, socket_type), &nn_respondent,
     NC_MASK_SOCK_READWRITE, NC_MASK_SOCK, NC_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_RESPONDENT socket type"},

    // Socket Options
    {"bind", 'b' , NULL,
     NC_OPT_STRING_LIST, offsetof(nc_options_t, bind_addresses), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Socket Options", "ADDR", "Bind socket to the address ADDR"},
    {"connect", 'c' , NULL,
     NC_OPT_STRING_LIST, offsetof(nc_options_t, connect_addresses), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_NO_REQUIRES,
     "Socket Options", "ADDR", "Connect socket to the address ADDR"},
    {"recv-timeout", 't', NULL,
     NC_OPT_FLOAT, offsetof(nc_options_t, timeout), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Socket Options", "SEC", "Set timeout for receiving a message"},
    {"send-timeout", 'T', NULL,
     NC_OPT_FLOAT, offsetof(nc_options_t, timeout), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_WRITEABLE,
     "Socket Options", "SEC", "Set timeout for sending a message"},

    // Pattern-specific options
    {"subscribe", 0, NULL,
     NC_OPT_STRING_LIST, offsetof(nc_options_t, subscriptions), NULL,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_SOCK_SUB,
     "SUB Socket Options", "PREFIX", "Subscribe to the prefix PREFIX. "
        "Note: socket will be subscribed to everything (empty prefix) if "
        "no prefixes are specified on the command-line."},

    // Echo Options
    {"format", 'f', NULL,
     NC_OPT_ENUM, offsetof(nc_options_t, echo_format), &echo_formats,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", "FORMAT", "Use echo format FORMAT "
                               "(same as the options below)"},
    {"raw", 0, NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_raw,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Dump message as is "
                           "(Note: no delimiters are printed)"},
    {"ascii", 'L', NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_ascii,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Print ASCII part of message delimited by newline. "
                           "All non-ascii characters replaced by dot."},
    {"quoted", 'Q', NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_quoted,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Print each message on separate line in double "
                           "quotes with C-like character escaping"},
    {"msgpack", 0, NULL,
     NC_OPT_SET_ENUM, offsetof(nc_options_t, echo_format), &nc_echo_msgpack,
     NC_NO_PROVIDES, NC_NO_CONFLICTS, NC_MASK_READABLE,
     "Echo Options", NULL, "Print each message as msgpacked string (raw type)."
                           " This is useful for programmatic parsing."},

    // Input Options
    {"data", 'D', NULL,
     NC_OPT_BLOB, offsetof(nc_options_t, data_to_send), &echo_formats,
     NC_MASK_DATA, NC_MASK_DATA, NC_MASK_WRITEABLE,
     "Data Options", "DATA", "Send DATA to the socket and quit for "
     "PUB, PUSH, PAIR socket. Use DATA to reply for REP or RESPONDENT socket. "
     "Send DATA as request for REQ or SURVEYOR socket. "},
    {"file", 'F', NULL,
     NC_OPT_READ_FILE, offsetof(nc_options_t, data_to_send), &echo_formats,
     NC_MASK_DATA, NC_MASK_DATA, NC_MASK_WRITEABLE,
     "Data Options", "PATH", "Same as --data but get data from file PATH"},

    // Sentinel
    {NULL}
    };


int main(int argc, char **argv) {
    nc_options_t options = {
        verbose: 0,
        socket_type: 0,
        bind_addresses: {NULL, 0},
        connect_addresses: {NULL, 0},
        timeout: -1.f,
        subscriptions: {NULL, 0},
        data_to_send: {NULL, 0},
        echo_format: NC_NO_ECHO
        };
    nc_parse_options(nc_options, NC_MASK_SOCK,
        &options, argc, argv);
    printf("VERBOSITY %d, timeout %f, sock %d, fmt %d, data [%d]``%.*s''\n",
        options.verbose, options.timeout,
        options.socket_type, options.echo_format,
        options.data_to_send.length, options.data_to_send.length,
        options.data_to_send.data);

}
