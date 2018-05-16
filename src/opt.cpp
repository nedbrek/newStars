#include "opt.h"
#include <iostream>
#include <cstring>
#include <string>
#include <stdexcept>

#ifndef STR_ERR_UNKNOWN_LONG_OPT
# define STR_ERR_UNKNOWN_LONG_OPT \
  ::std::string ("unrecognized option `--").append (option).append (1, '\'')
#endif

#ifndef STR_ERR_LONG_OPT_AMBIGUOUS
# define STR_ERR_LONG_OPT_AMBIGUOUS \
  ::std::string ("option `--").append (option).append ("' is ambiguous")
#endif

#ifndef STR_ERR_MISSING_ARG_LONG
# define STR_ERR_MISSING_ARG_LONG \
  ::std::string ("option `--").append (option).append ("' requires an argument")
#endif

#ifndef STR_ERR_UNEXPEC_ARG_LONG
# define STR_ERR_UNEXPEC_ARG_LONG \
  ::std::string ("option `--").append (option).append ("' doesn't allow an argument")
#endif

#ifndef STR_ERR_UNKNOWN_SHORT_OPT
# define STR_ERR_UNKNOWN_SHORT_OPT \
  ::std::string ("unrecognized option `-").append (1, *option).append (1, '\'')
#endif

#ifndef STR_ERR_MISSING_ARG_SHORT
# define STR_ERR_MISSING_ARG_SHORT \
  ::std::string ("option `-").append (1, *option).append ("' requires an argument")
#endif

/* Parse command line options.  Return index of first non-option argument.
   Throw a ::std::runtime_error exception if an error is encountered.  */
int parse_options (options *options, int argc, const char **argv)
{
    static const char *const optstr__component_list   = "component-list";
    static const char *const optstr__generate         = "generate";
    static const char *const optstr__newuniverse      = "newuniverse";
    static const char *const optstr__print            = "print";
    static const char *const optstr__regenerate_files = "regenerate-files";
    static const char *const optstr__test_races       = "test-races";

    options->opt_print            = 0;
    options->opt_generate         = 0;
    options->opt_newuniverse      = 0;
    options->opt_component_list   = 0;
    options->opt_regenerate_files = 0;
    options->opt_test_races       = 0;

	int i = 0;
	while( ++i < argc )
    {
        const char *option = argv[i];
        if( option[0] != '-' )
            return i;
        //else

		++option; // skip the dash

		if( *option == '\0' ) // -<nothing>, odd
            return i;
        //else

		if( *option == '-' ) // --<long_option>[=<value>]
        {
            ++option; // skip the second dash

			// find the '='
			const char *argument = strchr(option, '=');
            if( argument == option )
                throw ::std::runtime_error(STR_ERR_UNKNOWN_LONG_OPT); // oops
            //else

            size_t option_len;
			if( argument == NULL ) // if no '='
                option_len = strlen(option);
            else
			{
                option_len = argument - option;
				++argument;
			}

            switch( *option )
            {
            case '\0':
                return i + 1;

            case 'c': // component-list
                if( strncmp(option + 1, optstr__component_list + 1, option_len - 1) != 0 )
	                throw ::std::runtime_error (STR_ERR_UNKNOWN_LONG_OPT);
				//else

                if( argument != NULL )
                {
                    option = optstr__component_list;
                    throw ::std::runtime_error(STR_ERR_UNEXPEC_ARG_LONG);
                }
                options->opt_component_list = 1;
                break;

            case 'g':
                if( strncmp (option + 1, optstr__generate + 1, option_len - 1) == 0 )
                {
                    if (argument != 0)
                    {
                        option = optstr__generate;
                        throw ::std::runtime_error (STR_ERR_UNEXPEC_ARG_LONG);
                    }
                    options->opt_generate = 1;
                    break;
                }
                throw ::std::runtime_error (STR_ERR_UNKNOWN_LONG_OPT);
            case 'n':
                if( strncmp (option + 1, optstr__newuniverse + 1, option_len - 1) == 0 )
                {
                    if (argument != 0)
                    {
                        option = optstr__newuniverse;
                        throw ::std::runtime_error (STR_ERR_UNEXPEC_ARG_LONG);
                    }
                    options->opt_newuniverse = 1;
                    break;
                }
                throw ::std::runtime_error (STR_ERR_UNKNOWN_LONG_OPT);
            case 'p':
                if( strncmp (option + 1, optstr__print + 1, option_len - 1) == 0 )
                {
                    if (argument != 0)
                    {
                        option = optstr__print;
                        throw ::std::runtime_error (STR_ERR_UNEXPEC_ARG_LONG);
                    }
                    options->opt_print = 1;
                    break;
                }
            case 'r':
                if( strncmp (option + 1, optstr__print + 1, option_len - 1) == 0 )
                {
                    if (argument != 0)
                    {
                        option = optstr__regenerate_files;
                        throw ::std::runtime_error (STR_ERR_UNEXPEC_ARG_LONG);
                    }
                    options->opt_regenerate_files = 1;
                    break;
                }
			case 't':
               if( strncmp (option + 1, optstr__print + 1, option_len - 1) == 0 )
                {
                    if (argument != 0)
                    {
						option = optstr__test_races;
                        throw ::std::runtime_error (STR_ERR_UNEXPEC_ARG_LONG);
                    }
					options->opt_test_races = 1;
                    break;
                }

            default:
                throw ::std::runtime_error (STR_ERR_UNKNOWN_LONG_OPT);
            }
        }
        else
            do
            {
                switch (*option)
                {
                case 'c':
                    options->opt_component_list = 1;
                    break;
                case 'g':
                    options->opt_generate = 1;
                    break;
                case 'n':
                    options->opt_newuniverse = 1;
                    break;
                case 'p':
                    options->opt_print = 1;
                    break;
                case 'r':
                    options->opt_regenerate_files = 1;
                    break;
                case 't':
                    options->opt_test_races = 1;
                    break;
                default:
                    throw ::std::runtime_error (STR_ERR_UNKNOWN_SHORT_OPT);
                }
            }
            while (*++option != '\0');
    }
    return i;
}

