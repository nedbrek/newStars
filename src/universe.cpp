//
// C++ Implementation: universe
//
// Copyright: See COPYING file that comes with this distribution
//
#include "universe.h"
#include "classDef.h"
#include "utils.h"
#include "race.h"

#include <sstream>
#include <cassert>
#include <iostream>

//read in the universe data from the root XML structure
unsigned Universe::maxTechLevel = 26;
unsigned Universe::techKickerPerLevel = 10;

static const unsigned MAXSHIPBASEHULLS = 1024;

// return false if there is anything present with 'rad' of 'y'
// in the present matrix
static bool scan(unsigned y, unsigned rad, bool **present)
{
   assert( y >= rad );

   for(unsigned cy = y - rad; cy <= y + rad; cy++)
   for(unsigned cx = 0; cx < rad; cx++)
   if( present[cx][cy] )
   {
      return false;
   }
   return true;
}

unsigned rand16(void)
{
	return rand() & 0xffff;
}

unsigned rand32(void)
{
	return (rand16() << 16) | rand16();
}

unsigned Universe::randY(unsigned dy)
{
   const unsigned tmp = height - dy - dy;
   return (rand32() % tmp) + dy;
}

static unsigned getPlanetNormDist(void)
{
	static unsigned disty[99] = {
      1251564, 3337504, 7092197, 10429702, 15853147, 23779721, 29620355,
		36295365, 44221939, 55486019, 69670416, 81768872, 93867328, 99290773,
		108051724, 118898615, 128493942, 136003328, 146850219, 156445546,
		166458061, 176887764, 189403408, 205256557, 216520637, 232373786,
		240717549, 256153510, 270337907, 283687927, 293700442, 306633274,
		318731730, 328744245, 339591136, 347517710, 362119295, 374634939,
		382978702, 397163099, 410095931, 419274070, 434710031, 444722546,
		452231932, 464330388, 478097597, 487692924, 497705439, 506883578,
		518564846, 531497678, 541510193, 551939896, 565289916, 577805560,
		586149323, 595744650, 606591541, 617438432, 629536888, 642886908,
		652899423, 660408809, 673341641, 687108850, 697955741, 709219821,
		722569841, 732165168, 740508931, 751355822, 763454278, 773049605,
		783896496, 794326199, 800584021, 816437170, 825198121, 839799706,
		853566915, 867751312, 879015392, 888193531, 899040422, 911556066,
		920317017, 932832661, 939507671, 947434245, 959115513, 968710840,
		976220226, 985815553, 989987434, 992907751, 995410879, 999582760,
		999999948
	};
	const unsigned totalCt = 999999948;

	unsigned rnd = rand32();
	while( rnd >= totalCt )
		rnd = rand32();

	unsigned idx = 0;
	while( idx < 99 && rnd >= disty[idx] )
		++idx;

	return idx + 1;
}

void Universe::startNew(const char *rootPath, const GenParms &p,
    std::vector<class Planet*> *stars)
{
	gameYear   = 2400;
	masterCopy = true;
	width  = p.width;
	height = p.height;

	size_t sz = 0;
	std::vector<std::string> nameList;
	std::string nameFileName = rootPath;
	nameFileName += "/names.txt";
	FILE *nameFile = fopen(nameFileName.c_str(), "rt");
	char buf[512];
	fgets(buf, 512, nameFile);
	while( !feof(nameFile) )
	{
		size_t slen = strlen(buf);
		if( buf[slen-1] == 10 )
			buf[slen-1] = 0;
		nameList.push_back(buf);
		sz++;

		fgets(buf, 512, nameFile);
	}

   bool **present = new bool*[p.rad+1];
   for(unsigned i = 0; i < p.rad + 1; i++)
   {
      present[i] = new bool[height];
      for(unsigned j = 0; j < height; j++)
         present[i][j] = false;
   }

	uint64_t planetOID = 1;
   for(unsigned x = p.deadX; x < width - p.deadX;)
   {
      if( (unsigned(rand16()) % 10000) <= p.density )
      {
         unsigned y = randY(p.deadY);

         unsigned watchDog = 10000;
         while( watchDog && !scan(y, p.rad, present) )
         {
            y = randY(p.deadY);
            --watchDog;
         }

         if( watchDog == 0 )
         {
            printf("Watchdog bombed for x=%d\n", x);
         }
         else
         {
            present[p.rad][y] = true;

				// create the planet
				Planet *pp = new Planet;
				pp->objectId = planetOID;
				++planetOID;

				// pick a name
				const size_t nameIdx = rand32() % sz;
				pp->setName(nameList[nameIdx]);
				// remove that name
				nameList[nameIdx] = nameList[sz-1];
				nameList.pop_back();
				--sz;

				// set the coordinates
				pp->coordinates.set(x, y);

				// set environment
				pp->currentEnvironment.grav = getPlanetNormDist();
				pp->currentEnvironment.temp = getPlanetNormDist();
				pp->currentEnvironment.rad  = (rand16() % 99) + 1;

				pp->originalEnvironment = pp->currentEnvironment;

				// set minerals
				//Ned minerals shift under accBBS
				pp->mineralConcentrations.cargoDetail[_ironium  ].amount = (rand16() % 100) + 1;
				pp->mineralConcentrations.cargoDetail[_germanium].amount = (rand16() % 100) + 1;
				pp->mineralConcentrations.cargoDetail[_boranium ].amount = (rand16() % 100) + 1;

				// add it to the output list
            stars->push_back(pp);
         }
      }

      if( p.u == GenParms::UX )
      {
         x++;
         for(unsigned i = 1; i < p.rad+1; i++)
            memcpy(present[i-1], present[i], sizeof(bool)*height);
         for(unsigned j = 0; j < height; j++)
            present[p.rad][j] = false;
      }
   }

   for(unsigned j = 0; j < p.rad + 1; j++)
   {
      delete[] present[j];
   }
   delete[] present;
}

bool Universe::parseUniverseData(mxml_node_t *tree)
{
	slowTech_ = false; //Ned, parse

	mxml_node_t *node;
	mxml_node_t *rnode;
	rnode = mxmlFindElement(tree,tree,"NSTARS_UNIV",NULL,NULL,MXML_DESCEND_FIRST);
	if( !rnode )
	{
		cerr << "Error parsing universe: NSTARS_UNIV token not found\n";
		return false;
	}

	height = atoi(mxmlElementGetAttr(rnode,"Y"));
	width  = atoi(mxmlElementGetAttr(rnode,"X"));

	node = mxmlFindElement(rnode,rnode,"GAME_YEAR",NULL,NULL,MXML_DESCEND_FIRST);
	if( !node )
	{
		cerr << "Error parsing universe: GAME_YEAR token not found\n";
		return false;
	}

	gameYear = atoi(node->child->value.text.string);

	node = mxmlFindElement(rnode,rnode,"TECH_COSTS",NULL,NULL,MXML_DESCEND_FIRST);
	if( !node )
	{
		cerr << "Error parsing universe: TECH_COSTS token not found\n";
		return false;
	}

	unsigned *tempArray = (unsigned *)malloc(sizeof(unsigned)*maxTechLevel);
	int numRead = assembleNodeArray(node->child,tempArray,maxTechLevel);
	if( unsigned(numRead) != maxTechLevel )
	{
		cerr << "Error, incomplete techcost list: "<< endl;
		cerr << "Expecting "<<maxTechLevel<<" entries, found " << numRead << endl;
		return false;
	}
	for(unsigned i = 0; i < maxTechLevel; ++i)
		techCost.push_back(tempArray[i]);
	parseXML_noFail(rnode, "COMPONENTFILENAME",  componentFileName);
	parseXML_noFail(rnode, "HULLFILENAME",       hullFileName);
	parseXML_noFail(rnode, "PLAYERFILEBASENAME", playerFileBaseName);
	parseXML_noFail(rnode, "NUMBER_OF_PLAYERS",  numPlayers);
	parseXML_noFail(rnode, "MASTER_COPY",        masterCopy);
	if( componentFileName  == "") componentFileName = "newStarsComponents.xml";
	if(   hullFileName     == "" ) hullFileName = "newStarsHulls.xml";
	if( playerFileBaseName == "" ) playerFileBaseName = "untitledNewStarsGame";

	return true;
}

// return the universe postlude to close off the entire XML structure
void Universe::xmlPostlude(string &theString) const
{
    theString = "</NSTARS_UNIV>\n";
}

// first part of the game file, with the universe headers and details
void Universe::xmlPrelude(string &theString) const
{
	//Ned, clear?
	theString = "";

	//Ned, slowTech_

	 std::ostringstream os;
	 // XML version
	 os << "<?xml version=\"1.0\"?>" << std::endl;

	 // main wrapper, with width and height
	 os << "<NSTARS_UNIV DIMENSIONS=\"2\" X=\"" << width << "\" ";
	 os <<                               "Y=\"" << height << "\">" << std::endl;

	 // universe header, defunct
	 os << "<UNIV_HDR>" << std::endl;
	 os << "<STAR_COUNT>" << 2 << "</STAR_COUNT>" << std::endl;
	 os << "</UNIV_HDR>" << std::endl;

	 // game year
	 os << "<GAME_YEAR>" << gameYear << "</GAME_YEAR>" << std::endl;

	 // tech costs
	 std::string tempString;
    vectorToString(tempString, techCost);
	 os << "<TECH_COSTS>" << tempString << "</TECH_COSTS>" << std::endl;

	 // helper file refs
	 os <<  "<COMPONENTFILENAME>" <<  componentFileName <<  "</COMPONENTFILENAME>" << std::endl;
	 os <<       "<HULLFILENAME>" <<       hullFileName <<       "</HULLFILENAME>" << std::endl;
	 os << "<PLAYERFILEBASENAME>" << playerFileBaseName << "</PLAYERFILEBASENAME>" << std::endl;

	 // is this the master copy
	 os << "<MASTER_COPY>";
	 os << (masterCopy ? '1' : '0');
	 os << "</MASTER_COPY>" << std::endl;

	 os << "<NUMBER_OF_PLAYERS>" << numPlayers << "</NUMBER_OF_PLAYERS>" << std::endl;

	 theString += os.str();
}

