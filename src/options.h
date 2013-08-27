#ifndef NC_OPTIONS_HEADER
#define NC_OPTIONS_HEADER

enum nc_option_type {
    NC_OPT_INCREMENT,
    NC_OPT_DECREMENT,
};

struct nc_option {
    char *longname;
    char shortname;
    enum nc_option_type type;
    int option_position;
    char *group;
    char *description;
};


void nc_parse_options(struct nc_option *options, void *target);


#endif  // NC_OPTIONS_HEADER
