#ifndef NC_OPTIONS_HEADER
#define NC_OPTIONS_HEADER

enum nc_option_type {
    NC_OPT_INCREMENT,
    NC_OPT_DECREMENT,
    NC_OPT_ENUM,
    NC_OPT_STRING,
    NC_OPT_STRING_LIST,
};

struct nc_option {
    //  Option names
    char *longname;
    char shortname;
    char *arg0name;

    //  Parsing specification
    enum nc_option_type type;
    int option_position;  // offsetof() where to store the value
    void *pointer;  // type specific pointer

    //  Group and description for --help
    char *group;
    char *description;
};


void nc_parse_options(struct nc_option *options, void *target);


#endif  // NC_OPTIONS_HEADER
