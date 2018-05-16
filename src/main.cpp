#include "universe.h"
#include "gen.h"
#include "newStars.h"
#include "opt.h"
#include "actionDispatcher.h"

#include <iostream>
#include <fstream>

// test race wizard code
int go(void);

// construct game
int main(int argc, const char *argv[])
{
	mxmlSetWrapMargin(200);

   GameData gameData;
   // initialize static names
   initNameArrays();

   // get command line options
   options options;
   int i;
	try
	{
   	i = parse_options(&options, argc, argv);
	}
	catch(...)
	{
		i = argc;
	}

   if( options.opt_test_races )
   {
      go();
      starsExit("", EXIT_SUCCESS);
   }
   //else

   // check for error in arguments
   if( argv[i] == '\0' )
   {
      const char *progname = (argv[0] && *(argv[0]) != '\0') ?
                               argv[0] : "newStars";

      cerr << "Usage: " << progname << "[-cgnpr] TURNFILENAME" << endl
           << STR_HELP << endl;
      starsExit("", -1);
   }

   // get universe tree
   mxml_node_t *tree = getTree(argv[i]);
   if( !tree )
   {
      cerr << "Unable to read Universe XML file!\n";
      starsExit("", -1);
   }

   unsigned newLen = std::string(argv[0]).find_last_of("/\\");
   char *rootPath = new char[newLen+1];
   strncpy(rootPath, argv[0], newLen);
   rootPath[newLen] = 0;
   //printf("RootPath: '%s'\n", rootPath);

	if( options.opt_newuniverse )
	{
		UnivParms up;
		up.parseXMLtree(tree);

	   if( options.opt_print )
		{
			std::string str;
			up.dumpXMLToString(str);
			cout << str;
	      starsExit("", EXIT_SUCCESS);
		}

		gameData.createFromParms(rootPath, up);

		ActionDispatcher ad(&gameData);

		// do initial pen scans
		ad.calcScan();

		generateFiles(gameData, ".");

		starsExit("", EXIT_SUCCESS);
	}

   // grab components
// if( !gameData.parseTrees(tree, !options.opt_regenerate_files, "c:\\newStars\\newStars\\src\\"))//rootPath) )
   if( !gameData.parseTrees(tree, !options.opt_regenerate_files, rootPath) )
      starsExit("", -1);
   delete rootPath;

   if( !gameData.hasPlanets() )
   {
      cerr << "Error: No planetary data located!\n";
      starsExit("", -1);
   }

   gameData.addDefaults();

   // print universe back out and exit (debug)
   if( options.opt_print )
   {
      gameData.dumpXML();
      starsExit("", EXIT_SUCCESS);
   }
   //else

   // generate a new turn
   if( options.opt_generate )
   {
      gameData.playerMessageList.clear();
      ActionDispatcher aDispatchEngine(&gameData);

      aDispatchEngine.dispatchAll();
   }

   if( options.opt_regenerate_files || options.opt_generate )
   {
		generateFiles(gameData, ".");

	  char outTemp[256];
      sprintf(outTemp, "Success!  Generated files for game year: %i ",
          gameData.theUniverse->gameYear);
      starsExit(outTemp, EXIT_SUCCESS);
   }
   //else

   // print component list (debug)
   if( options.opt_component_list )
   {
      string tempString = "";
      gameData.technicalComponentsToString(tempString);
      cout << tempString;
      gameData.hullListToString(tempString);
      cout << tempString;
   }
   starsExit("Success (No operation specified -- did nothing)", EXIT_SUCCESS);
   return 0;
}

