#include "race.h"
#include "utils.h"
#include "gen.h"
#include <iostream>
#include <sstream>

#ifdef MSVC
inline int strcasecmp(const char *l, const char *r)
{
	return _stricmp(l, r);
}
#endif

const char *const UnivParms::udNames[UD_MAX+1] = {
	"SPARSE", "NORMAL", "DENSE", "PACKED", 0
};

const char *const UnivParms::upNames[UP_MAX+1] = {
	"CLOSE", "MODERATE", "FARTHER", "DISTANT", 0
};

const char *const UnivParms::usNames[US_MAX+1] = {
	"TINY", "SMALL", "MEDIUM", "LARGE", "HUGE", 0
};

static const char *const boolNames[3] = {"false", "true", 0};

UnivParms::UnivParms(void)
{
   universeName_ = "Barefoot Jaywalk";
   density_      = SPARSE;
   positions_    = CLOSE;
   size_         = TINY;

   max_minerals_  = false;
   slow_tech_     = false;
   accelerated_   = false;
   random_        = false;
   alliances_     = false;
   pubPlayScore_  = false;
   clumping_      = false;
}

void UnivParms::dumpXMLToString(std::string &str)
{
	std::ostringstream os;

	os << "<?xml version=\"1.0\"?>" << std::endl;
	os << "<NSTARS_NEW>" << std::endl;

	os << "<GAME_NAME>" << universeName_ << "</GAME_NAME>" << std::endl;
	os << "<BASENAME>"  << baseName_     << "</BASENAME>"  << std::endl;
	os << "<DENSITY>" << udNames[density_] << "</DENSITY>" << std::endl;
	os << "<PLAYER_POSITIONS>" << upNames[positions_] << "</PLAYER_POSITIONS>"
	    << std::endl;
	os << "<SIZE>" << usNames[size_] << "</SIZE>" << std::endl;

	os << "<MAX_MINERALS>" << boolNames[max_minerals_ ? 1 : 0]
	    << "</MAX_MINERALS>" << std::endl;
	os << "<SLOW_TECH>" << boolNames[slow_tech_ ? 1 : 0]
	    << "</SLOW_TECH>" << std::endl;
	os << "<ACCEL_START>" << boolNames[accelerated_ ? 1 : 0]
	    << "</ACCEL_START>" << std::endl;
	os << "<RANDOM_EVENTS>" << boolNames[random_ ? 1 : 0]
	    << "</RANDOM_EVENTS>" << std::endl;
	os << "<CP_ALLIANCE>" << boolNames[alliances_ ? 1 : 0]
	    << "</CP_ALLIANCE>" << std::endl;
	os << "<PUB_SCORES>" << boolNames[pubPlayScore_ ? 1 : 0]
	    << "</PUB_SCORES>" << std::endl;
	os << "<STAR_CLUMPS>" << boolNames[clumping_ ? 1 : 0]
	    << "</STAR_CLUMPS>" << std::endl;

	os << "</NSTARS_NEW>" << std::endl;

	str = os.str();
}

class XMLparseError
{
};

unsigned unmap(const char *const aryNames[], const char *val) throw(XMLparseError)
{
	for(unsigned i = 0; aryNames[i] != NULL; i++)
		if( !strcasecmp(aryNames[i], val) )
			return i;

	throw new XMLparseError;
	return unsigned(-1);
}

unsigned getValFromXML(mxml_node_t *rnode, const char *token,
    const char *const aryNames[]) throw(XMLparseError)
{
	mxml_node_t *node = mxmlFindElement(rnode, rnode, token, NULL, NULL,
	    MXML_DESCEND_FIRST);
	if( !node )
	{
		std::cerr << "Error parsing universe: " << token << " token not found\n";
		throw new XMLparseError;
	}
	return unmap(aryNames, node->child->value.text.string);
}

bool UnivParms::parseXMLtree(mxml_node_s *tree)
{
	mxml_node_t *rnode; // root node
	rnode = mxmlFindElement(tree, tree, "NSTARS_NEW", NULL, NULL, MXML_DESCEND_FIRST);
	if( !rnode )
	{
		std::cerr << "Error parsing universe: NSTARS_NEW token not found\n";
		return false;
	}

	mxml_node_t *node;
	node = mxmlFindElement(rnode, rnode, "GAME_NAME", NULL, NULL, MXML_DESCEND_FIRST);
	if( !node )
	{
		std::cerr << "Error parsing universe: GAME_NAME token not found\n";
		return false;
	}
	//universeName_ = node->child->value.text.string;
	assembleNodeArray(node->child, universeName_);

	node = mxmlFindElement(rnode, rnode, "BASENAME", NULL, NULL, MXML_DESCEND_FIRST);
	if( !node )
	{
		std::cerr << "Error parsing universe: BASENAME token not found\n";
		return false;
	}
	assembleNodeArray(node->child, baseName_);

	try
	{
		density_      = UniverseDensity  (getValFromXML(rnode, "DENSITY",          udNames));
		positions_    = UniversePositions(getValFromXML(rnode, "PLAYER_POSITIONS", upNames));
		size_         = UniverseSize     (getValFromXML(rnode, "SIZE",             usNames));

		max_minerals_ = getValFromXML(rnode, "MAX_MINERALS",     boolNames) == 1;
		slow_tech_    = getValFromXML(rnode, "SLOW_TECH",        boolNames) == 1;
		accelerated_  = getValFromXML(rnode, "ACCEL_START",      boolNames) == 1;
		random_       = getValFromXML(rnode, "RANDOM_EVENTS",    boolNames) == 1;
		alliances_    = getValFromXML(rnode, "CP_ALLIANCE",      boolNames) == 1;
		clumping_     = getValFromXML(rnode, "STAR_CLUMPS",      boolNames) == 1;

		mxml_node_t *raceListNode = mxmlFindElement(rnode, rnode, "RACELIST", NULL, NULL,MXML_DESCEND);
		mxml_node_t *node = mxmlFindElement(raceListNode, raceListNode, "RACE", NULL, NULL,
		                                    MXML_DESCEND);
		while( node )
		{
			Race *r = new Race;
			r->initRace(node);

			races_.push_back(r);

			node = mxmlFindElement(node, raceListNode, "RACE", NULL, NULL,
		                          MXML_NO_DESCEND);
		}
	}
	catch(XMLparseError *parseErr)
	{
		delete parseErr;
		return false;
	}

	return true;
}

