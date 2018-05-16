#ifndef OPT__H
#define OPT__H

struct options
{
    // Set to 1 if option --print (-p) has been specified
unsigned int opt_print :
    1;

    // Set to 1 if option --generate (-g) has been specified
unsigned int opt_generate :
    1;

    // Set to 1 if option --newuniverse (-n) has been specified.
unsigned int opt_newuniverse :
    1;

    // Set to 1 if option --component-list (-c) has been specified.
unsigned int opt_component_list :
    1;

	//Set to 1 if option --regenerate-files (-r) has been specified/
unsigned int opt_regenerate_files :
	1;

	//Set to 1 if option --test-races (-t) has been specified/
unsigned int opt_test_races:
	1;
};

// Parse command line options.  Return index of first non-option argument.
// Throw a ::std::runtime_error exception if an error is encountered.
extern int parse_options (options *options, int argc, const char *argv[]);

#define STR_HELP_PRINT \
  "  -p, --print             Print the input data only\n"

#define STR_HELP_GENERATE \
  "  -g, --generate          Generate the turn\n"

#define STR_HELP_NEWUNIVERSE \
  "  -n, --newuniverse       generate a new detailed universe from the planet\n" \
  "                            list input\n"

#define STR_HELP_COMPONENT_LIST \
  "  -c, --component-list    Print the list of components that players can build\n" \
  "                            into hulls\n"

#define STR_HELP_REGENERATE_FILES \
  "  -r, --regenerate-files    Regenerates the universe and player files for the\n" \
  "                            turn file passed in the command line\n"

#define STR_HELP_TEST_RACES\
  "  -t, --test-races	Runs race testing\n"

#define STR_HELP \
  STR_HELP_PRINT \
  STR_HELP_GENERATE \
  STR_HELP_NEWUNIVERSE \
  STR_HELP_COMPONENT_LIST \
  STR_HELP_REGENERATE_FILES\
  STR_HELP_TEST_RACES

#endif

