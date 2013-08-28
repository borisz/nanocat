#ifndef NC_OPTIONS_HEADER
#define NC_OPTIONS_HEADER

enum nc_option_type {
    NC_OPT_HELP,
    NC_OPT_INCREMENT,
    NC_OPT_DECREMENT,
    NC_OPT_ENUM,
    NC_OPT_SET_ENUM,
    NC_OPT_STRING,
    NC_OPT_BLOB,
    NC_OPT_FLOAT,
    NC_OPT_STRING_LIST,
    NC_OPT_READ_FILE
};

struct nc_option {
    //  Option names
    char *longname;
    char shortname;
    char *arg0name;

    //  Parsing specification
    enum nc_option_type type;
    int offset;  // offsetof() where to store the value
    const void *pointer;  // type specific pointer

    //  Conflict mask for options
    unsigned long mask_set;
    unsigned long conflicts_mask;
    unsigned long requires_mask;

    //  Group and description for --help
    char *group;
    char *metavar;
    char *description;
};

struct nc_enum_item {
    char *name;
    int value;
};

struct nc_string_list {
    char **items;
    int num;
};

struct nc_blob {
    char *data;
    int length;
};


void nc_parse_options(struct nc_option *options, unsigned long requires,
    void *target, int argc, char **argv);


#endif  // NC_OPTIONS_HEADER
