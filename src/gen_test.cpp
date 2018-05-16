#include "mxml.h"
#include "gen.h"
#include <string>
#include <iostream>
#include <fstream>

extern mxml_node_t* getTree(const char *argv);

int main(int argc, char **argv)
{
   UnivParms up; // default universe

	if( argc > 1 && argv[1][1] == 'r' )
	{
		if( argc < 3 )
		{
			std::cout << "Usage: " << argv[0] << " [-r <filename>]" << std::endl;
			return 1;
		}

		// overwrite from file
		mxml_node_t *tree = getTree(argv[2]);
		if( !tree )
		{
			std::cout << "Unable to parse file: " << argv[2] << std::endl;
			return 2;
		}
		up.parseXMLtree(tree);
	}

	std::string ostr;
	up.dumpXMLToString(ostr);
	std::cout << ostr << std::endl;

	return 0;
}

