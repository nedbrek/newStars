#ifdef MSVC
#pragma warning(disable: 4786)
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "battle.h"
#include "gen.h"
#include "universe.h"
#include "newStars.h"
#include "planet.h"
#include "race.h"
#include "utils.h"
#include "reconcile.h"

#include <sstream>
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

#if defined(MSVC) && !defined(MSVC8)
std::ostream& operator<<(std::ostream &o, uint64_t i)
{
	o << unsigned(i / 1000000000) << unsigned(i % 10000000000);
	return o;
}
#endif

std::string ColonizerModuleComponent::ColonizerDescriptions[_numberOfColonizerTypes];

std::string structuralElementCategoryStrings[numberOf_ELEMENT];
std::string structuralElementCategoryCodes  [numberOf_ELEMENT];

std::string mineTypeArray[_numberOfMineTypes];

std::map<string, int>      structuralElementCodeMap;
std::map<string, unsigned> orderNameMap;
std::map<string, unsigned> seElementNameMap;

uint64_t GameObject::nextObjectId;

uint64_t formXYhash(unsigned x, unsigned y)
{
   uint64_t tmp64 = x;
   tmp64 <<= 32;
   tmp64 += y;
   return tmp64;
}

void generateFiles(const GameData &gameData, const char *path)
{
	std::string gameString;
	gameData.theUniverse->masterCopy = true;
	gameData.dumpXMLToString(gameString);
	std::string fName = path;
	fName += '/';
	fName += gameData.theUniverse->playerFileBaseName;

	fName += "_master.xml";
	std::ofstream fileOut;
	fileOut.open(fName.c_str(),ios_base::out | ios_base::trunc);
	if( !fileOut.good() )
	{
		cerr << "Error when opening file: " << fName << " for output\n";
		starsExit("main.cpp",-1);
	}
	// XML version
	fileOut << "<?xml version=\"1.0\"?>" << std::endl;
	fileOut << gameString;
	fileOut.close();

	for(unsigned i = 0; i < gameData.playerList.size(); i++)
		gameData.dumpPlayerFile(gameData.playerList[i]);
}

/* The assembleNodeArray methods take a child leaf, and return all the data associated
with the XML child and return it in a variety of formats.

For example, an XML line:
<MYDATA>1 2 3 4 5 6</MYDATA>

would be found using:
  mxml_node_t *node = mxmlFindElement(tree,tree,"MYDATA",NULL,NULL,MXML_DESCEND);

The data starts in the first child node -- eg node->child

A call to:
  string myString;
  assembleNodeArray(node->child,myString);

Would store: "1 2 3 4 5 6" in the string myString;

A call to:
  string myStringArray[6];
  int num = assembleNodeArray(node->child,myStringArray,6);

Would return 6, and store the strings "1" "2" "3" "4" "5" "6" in the array in myStringArray[0]..[5] respectively

A call to:
  int myIntArray[6];
  int num = assembleNodeArray(node->child,myIntArray,6);

Would return 6, and store the ints 1 2 3 4 5 6 in the array myIntArray[0]..[5] respectively.

Return value indicates the number of elements returned in the array, or -1 on overflow.
*/
void assembleNodeArray(mxml_node_t *node, string &theString)
{
	theString = "";

	string tempString;
	while( node )
	{
		tempString = node->value.text.string;
		theString += tempString;

		node = node->next;
		if( node )
			theString += " ";
	}
}

// returns number of values found
int assembleNodeArray(mxml_node_t *node, string theArray[], int size)
{
	if( !node )
		return 0;

	mxml_node_t *child = node;
	int counter = 0;
	while( child )
	{
		if( counter == size )
		{
			cerr << "Error, array overflow in assembleNodeEntry\n";
			return -1;
		}

		theArray[counter] = child->value.text.string;
		counter++;

		child = child->next;
	}

	return counter;
}

int assembleNodeArray(mxml_node_t *node, unsigned theArray[],int size)
{
	if( !node )
		return 0;

	mxml_node_t *child = node;
	int counter = 0;
	while( child )
	{
		if( counter == size )
		{
			cerr << "Error, array overflow in assembleNodeEntry\n";
			return -1;
		}

		theArray[counter] = (unsigned)atoi(child->value.text.string);
		counter++;

		child = child->next;
	}

	return counter;
}

int assembleNodeArray(mxml_node_t *node, int theArray[], int size)
{
	if( !node )
		return 0;

	mxml_node_t *child = node;
	int counter = 0;
	while( child )
	{
		if( counter == size )
		{
			cerr << "Error, array overflow in assembleNodeEntry\n";
			return -1;
		}

		theArray[counter] = atoi(child->value.text.string);
		counter++;

		child = child->next;
	}

	return counter;
}

//----------------------------------------------------------------------------
EnemyPlayer::EnemyPlayer(void):
	singularName("Error"), pluralName("Errors"),
	ownerID(unsigned(-1))
{
}

EnemyPlayer::EnemyPlayer(const Race &r):
	singularName(r.singularName), pluralName(r.pluralName),
	ownerID(r.ownerID)
{
}

bool EnemyPlayer::parseXML(mxml_node_t *tree)
{
	mxml_node_t *child = mxmlFindElement(tree,tree,"SINGULARNAME",NULL,NULL,MXML_DESCEND);
	if( !child )
	{
	    std::cerr << "Error, could not parse race name\n";
	    return false;
	}
	child = child->child;
	assembleNodeArray(child, singularName);

	child = mxmlFindElement(tree,tree,"PLURALNAME",NULL,NULL,MXML_DESCEND);
	if( !child )
	{
	    std::cerr << "Error, could not parse race name\n";
	    return false;
	}
	child = child->child;
	assembleNodeArray(child,pluralName);

	child = mxmlFindElement(tree,tree,"OWNERID",NULL,NULL,MXML_DESCEND);
	if (child == NULL)
	{
	    std::cerr << "Error, could not parse ownerID for race:"<<singularName<<"\n";
	    return false;
	}
	child = child->child;
	ownerID = atoi(child->value.text.string);

	return true;
}

void EnemyPlayer::toXMLString(std::string &theString) const
{
	std::ostringstream os;
	os << "<ENEMY_PLAYER>\n";
	os << "<SINGULARNAME>" << singularName << "</SINGULARNAME>\n";
	os << "<PLURALNAME>"   <<   pluralName << "</PLURALNAME>\n";
	os << "<OWNERID>" << ownerID << "</OWNERID>\n";
	os << "</ENEMY_PLAYER>\n";

	theString = os.str();
}

//----------------------------------------------------------------------------
void Location::moveToward(const Location &l, unsigned range)
{
    unsigned dist = distanceFrom(l);

    // we can reach the destination this turn
    if( range >= dist )
    {
        *this = l;
        return;
    }
    // else can't make it the whole way, calculate new location

    double deltaX = double(l.getXDouble()) - xLocation;
    double deltaY = double(l.getYdouble()) - yLocation;
    double  ratio = double(range)/dist;
    //cout <<"Range of fleet at current speed = "<<range<<endl;
    //cout <<"Distance to fleet destination = "<<dist<<endl;
    //cout <<"DeltaX = "<<deltaX<<" DeltaY = "<<deltaY<<" Ratio = "<<ratio<<endl;
    deltaX *= ratio;
    deltaY *= ratio;
    //cout <<"MotionX = "<<deltaX<<" MotionY = "<<deltaY<<endl;

    /*
    if (deltaX>0)
     deltaX = ceil(deltaX);
    else deltaX = floor(deltaX);

    if (deltaY>0)
    deltaY = ceil(deltaY);
    else deltaY = floor(deltaY);
    */
    xLocation += deltaX;
    yLocation += deltaY;
    //cout <<"Final coord "<<xLocation<<","<<yLocation<<endl;
}

const double PI = 4.0 * atan(1.0);

double Location::makeAngleTo(const Location &l) const
{
	double deltaX = l.xLocation - xLocation;
	double deltaY = l.yLocation - yLocation;
	if( deltaX == 0.0 )
		return PI / 2.0;

	return atan2(deltaY, deltaX);
}

double Location::actualDistanceFrom(const Location &where) const
{
	double deltaX = double(xLocation) - where.xLocation;
	double deltaY = double(yLocation) - where.yLocation;
	//cout << "distanceFrom: deltax = " << deltaX << "deltay = " << deltaY << std::endl;

	double temp = (deltaX * deltaX) + (deltaY * deltaY);
	//cout << "distanceFrom: temp = " << temp << std::endl;
	temp = sqrt(temp);
	//cout << "distanceFrom: temp = " << temp << std::endl;
	return temp;
}

// use pythagoras: d^2 = x^2 + y^2
unsigned Location::distanceFrom(const Location &where) const
{
	return unsigned(actualDistanceFrom(where));
}

bool Location::operator<(const Location &rhs) const
{
	return ceil(xLocation) < ceil(rhs.xLocation) ||
	       ceil(yLocation) < ceil(rhs.yLocation);
}

bool Location::operator==(const Location &rhs) const
{
	return ceil(xLocation) == ceil(rhs.xLocation) &&
	       ceil(yLocation) == ceil(rhs.yLocation);
}

bool Location::operator!=(const Location &rhs) const
{
	return !(*this == rhs);
}

//----------------------------------------------------------------------------
CargoManifest CargoManifest::operator+(const CargoManifest &param)
{
	CargoManifest temp;
	for(unsigned i = 0; i < _numCargoTypes; i++)
	{
		temp.cargoDetail[i].isInfinite = cargoDetail[i].isInfinite;
		if( cargoDetail[i].isInfinite )
			temp.cargoDetail[i].amount = 0;
		else
			temp.cargoDetail[i].amount = cargoDetail[i].amount + param.cargoDetail[i].amount;
	}
	return temp;
}

CargoManifest CargoManifest::operator*(unsigned param)
{
	CargoManifest temp;
	for(unsigned i = 0; i < _numCargoTypes; ++i)
	{
		temp.cargoDetail[i].isInfinite = cargoDetail[i].isInfinite;

		if( cargoDetail[i].isInfinite )
			temp.cargoDetail[i].amount = 0;
		else
			temp.cargoDetail[i].amount = cargoDetail[i].amount * param;
	}
	return temp;
}

CargoManifest CargoManifest::operator/(unsigned param)
{
	CargoManifest temp;
	for(int i = 0; i < _numCargoTypes; i++)
	{
		temp.cargoDetail[i].isInfinite = cargoDetail[i].isInfinite;
		if( cargoDetail[i].isInfinite )
			temp.cargoDetail[i].amount = 0;
		else
			temp.cargoDetail[i].amount = cargoDetail[i].amount / param;
	}
	return temp;
}

CargoManifest CargoManifest::operator-(const CargoManifest &param)
{
	CargoManifest temp;
	for(int i = 0; i < _numCargoTypes; i++)
	{
		if( param.cargoDetail[i].amount > cargoDetail[i].amount )
		{
			// trying to subtract more than we had
			cerr << "Fatal Error, attempted to subtract more cargo than available\n";
			starsExit("newstars.cpp", -1);
		}
		temp.cargoDetail[i].amount      = cargoDetail[i].amount - param.cargoDetail[i].amount;
		temp.cargoDetail[i].isInfinite  = cargoDetail[i].isInfinite;
		if( cargoDetail[i].isInfinite )
			temp.cargoDetail[i].amount=0;
	}
	return temp;
}

bool CargoManifest::operator<(const CargoManifest &rhs) const
{
	for(int i = 0; i < _numCargoTypes; i++)
	{
		if( cargoDetail[i].amount >= rhs.cargoDetail[i].amount &&
		    !rhs.cargoDetail[i].isInfinite )
			return false;
	}
	return true;
}

bool CargoManifest::operator==(const CargoManifest &rhs) const
{
	for(int i = 0; i < _numCargoTypes; i++)
	{
		if( cargoDetail[i].amount != rhs.cargoDetail[i].amount &&
		    !(cargoDetail[i].isInfinite && rhs.cargoDetail[i].isInfinite) )
			return false;
	}
	return true;
}

bool CargoManifest::anyGreaterThan(const CargoManifest &rhs) const
{
	for(int i = 0; i < _numCargoTypes; i++)
	{
		if( cargoDetail[i].isInfinite && !rhs.cargoDetail[i].isInfinite )
			return true;

		if( cargoDetail[i].amount > rhs.cargoDetail[i].amount )
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------
// XML data type callback for mxmlLoadFile()...
// Output: Data type
// Input : Element node
mxml_type_t type_cb(mxml_node_t *node)
{
    const char *type; // Type string

    // You can lookup attributes and/or use the element name, hierarchy, etc...

    if( (type = mxmlElementGetAttr(node, "type")) == NULL )
        type = node->value.element.name;

    if( !strcmp(type, "integer") )
        return MXML_INTEGER;

    if( !strcmp(type, "opaque") || !strcmp(type, "pre") )
        return MXML_OPAQUE;

    if( !strcmp(type, "real") )
        return MXML_REAL;

    return MXML_TEXT;
}

mxml_node_t* getTree(const char *argv)
{
	// check for tree on stdin
	if( argv[0] == '<' )
		return mxmlLoadString(NULL, argv, type_cb);

	// look for tree in file
	FILE *fp = fopen(argv, "r");
	if( fp == NULL )
	{
		perror(argv);
		cerr << "Error, could not open file " << argv << "\n";
		return NULL;
	}
	//else

	mxml_node_t *tree = mxmlLoadFile(NULL, fp, type_cb);
	fclose(fp);

	return tree;
}

GameData::GameData(void)
{
	univParms_  = NULL;
	theUniverse = new Universe;
	habRangeMultiplier = 100;
	maxValues.grav  = 100; //8.00 g
	maxValues.temp  = 100; //200 C
	maxValues.rad   = 100; //100 mRad
	minValues.grav  = 0;   //0.12 g
	minValues.temp  = 0;   //-200 C
	minValues.rad   = 0;   //0 mRad

	onePercentValues.grav = (maxValues.grav - minValues.grav) / 100;
	onePercentValues.temp = (maxValues.temp - minValues.temp) / 100;
	onePercentValues.rad  = (maxValues.rad  - minValues.rad ) / 100;
}

// tiny=400; small=800; medium=1200; large=1600; huge=2000
static const unsigned univSizes[5] = {400, 800, 1200, 1600, 2000};
static const    short univDensities[4] = {180, 240, 300, 450};

static const unsigned techcosts[26] = {
	   50,    80,   130,   210,
	  340,   550,   890,  1440,
	 2330,  3770,  6100,  9870,
	13850, 18040, 22440, 27050,
	31870, 36900, 42140, 47590,
	53250, 59120, 65200, 71490,
	77990, 84700
};

Ship* findShipById(const std::vector<Ship*> &slist, uint64_t id)
{
   std::vector<Ship*>::const_iterator k = slist.begin();
   while( k != slist.end() )
   {
      Ship *ship = *k;

      if( ship->objectId == id )
			return ship;

      ++k;
   }
   return NULL;
}

// find a particular component
StructuralElement* GameData::getStructureByName(const std::string &name)
{
	// there is supposed to be a cache for this, try it out
	std::map<std::string, StructuralElement*>::const_iterator mi =
		componentMap.find(name);
	if( mi != componentMap.end() )
		return mi->second;

	// cache isn't built right now, search and update
	for(std::vector<StructuralElement*>::iterator i = componentList.begin();
	    i != componentList.end(); ++i)
	{
		if( (**i).name == name )
		{
			// found it
			componentMap[name] = *i; // fix cache
			return *i;
		}
	}
	// odd, no such component
	return NULL;
}

bool isEngineBetter(const WarpEngineComponent &a,
    const WarpEngineComponent &b)
{
	if( a.maxSafeSpeed > b.maxSafeSpeed )
		return true;

	if( a.isScoop && !b.isScoop )
		return true;

	if( a.battleSpeed > b.battleSpeed )
		return true;

	//Ned? fix fuel consumption
	return false;
}

bool isScannerBetter(const ScanComponent &a, const ScanComponent &b)
{
	if( a.penScanRange > b.penScanRange )
		return true;

	if( a.baseScanRange > b.baseScanRange )
		return true;

	return false;
}

StructuralElement* pickEngine(const PlayerData &pd)
{
	StructuralElement *best = NULL;
	for(std::vector<StructuralElement*>::const_iterator i = pd.cmpAvail_.begin();
	    i != pd.cmpAvail_.end(); ++i)
	{
		if( (**i).engine.isActive )
		{
			// found an engine
			if( !best || isEngineBetter((**i).engine, best->engine) )
			{
				best = *i;
			}
		}
	}
	return best;
}

// when to not pick pen?
StructuralElement* pickScanner(const PlayerData &pd)
{
	StructuralElement *best = NULL;

	for(std::vector<StructuralElement*>::const_iterator i = pd.cmpAvail_.begin();
	    i != pd.cmpAvail_.end(); ++i)
	{
		StructuralElement *sep = *i;
		if( sep->elementType != scanner_ELEMENT )
			continue;
		if( !sep->scanner.isActive )
			continue;

		if( !best || isScannerBetter(sep->scanner, best->scanner) )
		{
			best = sep;
		}
	}

	return best;
}

//Ned? when to not use max range?
//Ned? when to use shield sapper?
//Ned? when to use gatling?
//Ned? what about initiative games?
bool isBeamWeaponBetter(const BeamWeaponComponent &l,
    const BeamWeaponComponent &r)
{
	// check range
	if( l.range > r.range )
		return true;
	if( l.range < r.range )
		return false;

	if( l.damageFactors[0] > r.damageFactors[0] )
		return true;

	return false;
}

bool isTorpBetter(const MissleWeaponComponent &l,
    const MissleWeaponComponent &r)
{
	//Ned, might want range for a special sniper/speed scenario...
	// check cap, always want cap over any other
	if( l.isCapitalShipMissle && !r.isCapitalShipMissle )
		return true;
	if( r.isCapitalShipMissle && !l.isCapitalShipMissle )
		return false;

	if( l.damage > r.damage )
		return true;
	if( r.damage > l.damage )
		return false;

	if( l.range > r.range )
		return true;
	if( r.range > l.range )
		return false;

	return  l.accuracy > r.accuracy;
}

StructuralElement* pickBeamWeapon(const PlayerData &pd)
{
	StructuralElement *best = NULL;

	for(std::vector<StructuralElement*>::const_iterator i = pd.cmpAvail_.begin();
	    i != pd.cmpAvail_.end(); ++i)
	{
		if( (**i).beam.isActive )
		{
			if( !best || isBeamWeaponBetter((**i).beam, best->beam) )
			{
				best = *i;
			}
		}
	}
	return best;
}

StructuralElement* pickTorpedo(const PlayerData &pd)
{
	StructuralElement *best = NULL;

	for(std::vector<StructuralElement*>::const_iterator i = pd.cmpAvail_.begin();
	    i != pd.cmpAvail_.end(); ++i)
	{
		if( (**i).missle.isActive )
		{
			if( !best || isTorpBetter((**i).missle, best->missle) )
			{
				best = *i;
			}
		}
	}
	return best;
}

StructuralElement* pickShield(const PlayerData &pd)
{
	StructuralElement *best = NULL;

	for(std::vector<StructuralElement*>::const_iterator i = pd.cmpAvail_.begin();
	    i != pd.cmpAvail_.end(); ++i)
	{
		StructuralElement *sep = *i;
		if( sep->elementType != shield_ELEMENT )
			continue;

		//Ned? how to balance armor + shield?
		//Ned? what about regen shields?
		if( !best || sep->armor + sep->shields >
		    best->armor + best->shields )
		{
			best = sep;
		}
	}
	return best;
}

StructuralElement* pickArmor(const PlayerData &pd)
{
	StructuralElement *best = NULL;

	for(std::vector<StructuralElement*>::const_iterator i = pd.cmpAvail_.begin();
	    i != pd.cmpAvail_.end(); ++i)
	{
		StructuralElement *sep = *i;
		if( sep->elementType != armor_ELEMENT )
			continue;

		//Ned? how to balance armor + shield?
		//Ned? what about regen shields?
		if( !best || sep->armor + sep->shields >
		    best->armor + best->shields )
		{
			best = sep;
		}
	}
	return best;
}

//Ned? when to trade mass for rate (ultra miner?)
StructuralElement* pickMiner(const PlayerData &pd)
{
	StructuralElement *best = NULL;

	for(std::vector<StructuralElement*>::const_iterator i = pd.cmpAvail_.begin();
	    i != pd.cmpAvail_.end(); ++i)
	{
		StructuralElement *sep = *i;
		if( sep->elementType != remoteMiner_ELEMENT )
			continue;

		if( !best || sep->remoteMineing > best->remoteMineing )
		{
			best = sep;
		}
	}
	return best;
}

//Ned? chaff?
Ship* makeScoutDesign(bool armed, PlayerData *pd, const std::string &name)
{
	// clone off a scout hull
	Ship *design = new Ship(*(pd->gd_->hullList[scout_hull]));
	design->objectId = IdGen::getInstance()->makeDesign(pd->race->ownerID, pd->objectMap);
	pd->objectMap[design->objectId] = design;

	design->setName(name);
	design->designhullID = design->objectId; // it's a design
	design->ownerID      = pd->race->ownerID; // we own it

	// engine
	design->slotList[0]->numberPopulated = 1;
	design->slotList[0]->structureID     = pickEngine(*pd)->structureID;

	// fuel tank or weapon
	design->slotList[1]->numberPopulated = 1;
	if( armed )
	{
		design->slotList[1]->structureID = pickBeamWeapon(*pd)->structureID;
	}
	else
	{
		design->slotList[1]->structureID =
		    pd->gd_->getStructureByName("Fuel Tank")->structureID;
	}

	// scanner
	design->slotList[2]->numberPopulated = 1;
	design->slotList[2]->structureID = pickScanner(*pd)->structureID;

	design->updateCostAndMass(*pd->gd_);

	return design;
}

Fleet* makeScout(bool armed, PlayerData *pd, const std::string &name)
{
	Ship *design = makeScoutDesign(armed, pd, name);

	// add it to the design lists
	design->quantity = 1;
	pd->designList.push_back(design);
	pd->gd_->playerDesigns.push_back(design);

	// build it into a fleet
	Fleet *ret = new Fleet;
	ret->objectId = IdGen::getInstance()->makeFleet(pd->race->ownerID, false, pd->objectMap);
	pd->objectMap[ret->objectId] = ret;

	std::ostringstream os;
	os << name << ' ' << '#' << ret->objectId;
	ret->setName(os.str());
	ret->ownerID = pd->race->ownerID;

	// add the ship
	Ship *scout = new Ship(*design);
	scout->objectId = ret->objectId;

	scout->setName(name);
	scout->quantity = 1;

	ret->shipRoster.push_back(scout);

	ret->updateFleetVariables(*pd->gd_);
	ret->cargoList.cargoDetail[_fuel].amount = ret->fuelCapacity;
	return ret;
}

//Ned when to not use the best engine?
//Ned when to switch to small or medium freighter?
//Ned when to use privateer?
Fleet* makeColonyShip(PlayerData *pd, const std::string &name)
{
	HullTypes ht = colonizer_hull;
	if( pd->race->prtVector[hyperExpansionist_PRT] == 1 )
		ht = miniColonizer_hull; // HE uses mini-colony

	// clone off a colony ship hull
	Ship *design = new Ship(*(pd->gd_->hullList[ht]));
	design->objectId = IdGen::getInstance()->makeDesign(pd->race->ownerID, pd->objectMap);
	pd->objectMap[design->objectId] = design;

	design->setName(name);
	design->designhullID = design->objectId; // it's a design
	design->ownerID      = pd->race->ownerID; // we own it
	design->quantity     = 1;

	// engine
	design->slotList[0]->numberPopulated = 1;
	design->slotList[0]->structureID     = pickEngine(*pd)->structureID;

	// colonization module
	//Ned AR?
	const StructuralElement *colonizer =
	    pd->gd_->getStructureByName("Colonization Module");
	design->slotList[1]->numberPopulated = 1;
	design->slotList[1]->structureID     = colonizer->structureID;

	design->updateCostAndMass(*pd->gd_);

	// add it to the design lists
	pd->designList.push_back(design);
	pd->gd_->playerDesigns.push_back(design);

	// build it into a fleet
	Fleet *ret = new Fleet;
	ret->objectId = IdGen::getInstance()->makeFleet(pd->race->ownerID, false, pd->objectMap);
	pd->objectMap[ret->objectId] = ret;

	std::ostringstream os;
	os << name << ' ' << '#' << ret->objectId;
	ret->setName(os.str());
	ret->ownerID = pd->race->ownerID;

	// add the ship
	Ship *ship = new Ship(*design);
	ship->objectId = ret->objectId;

	ship->setName(name);
	ship->quantity = 1;

	ret->shipRoster.push_back(ship);

	ret->updateFleetVariables(*pd->gd_);
	ret->cargoList.cargoDetail[_fuel].amount = ret->fuelCapacity;
	return ret;
}

// initial build is not what I would build...
// JOAT gets an armored scanning medium freighter (unless con tech 4)
// SS gets a small freighter with shadow shield and cloak (regardless of tech)
Fleet* makeInitialFreighter(PlayerData *pd, const std::string &name)
{
	// only JOAT and SS come here, which are we?
	const bool joatSSbar = pd->race->primaryTrait == jackOfAllTrades_PRT;

	HullTypes ht = mediumFreighter_hull;
	if( !joatSSbar )
		ht = smallFreighter_hull;

	// clone off the freighter ship hull
	Ship *design = new Ship(*(pd->gd_->hullList[ht]));
   design->objectId = IdGen::getInstance()->makeDesign(pd->race->ownerID, pd->objectMap);
   pd->objectMap[design->objectId] = design;
	
	design->setName(name);
	design->designhullID = design->objectId; // it's a design
	design->ownerID      = pd->race->ownerID; // we own it
	design->quantity     = 1;

	// engine
	design->slotList[0]->numberPopulated = design->slotList[0]->maxItems;
	design->slotList[0]->structureID     = pickEngine(*pd)->structureID;

	// slots are the same in small and medium freighter
	// middle slot is armor for JOAT, shield for SS
	design->slotList[1]->numberPopulated = 1;
	design->slotList[1]->structureID = joatSSbar ?
	    pickArmor(*pd)->structureID : pickShield(*pd)->structureID;

	// last slot is scanner for JOAT, cloak for SS
	design->slotList[2]->numberPopulated = 1;
	if( !joatSSbar )
	{
		design->slotList[2]->structureID =
		    pd->gd_->getStructureByName("Transport Cloaking")->structureID;
	}
	else
		design->slotList[2]->structureID = pickScanner(*pd)->structureID;

	design->updateCostAndMass(*pd->gd_);

	// add it to the design lists
	pd->designList.push_back(design);
	pd->gd_->playerDesigns.push_back(design);

	// build it into a fleet
	Fleet *ret = new Fleet;
   ret->objectId = IdGen::getInstance()->makeFleet(pd->race->ownerID, false, pd->objectMap);
   pd->objectMap[ret->objectId] = ret;
	
	std::ostringstream os;
	os << name << ' ' << '#' << ret->objectId;
	ret->setName(os.str());
	ret->ownerID = pd->race->ownerID;

	// add the ship
	Ship *ship = new Ship(*design);
	ship->objectId = ret->objectId;

	ship->setName(name);
	ship->quantity = 1;

	ret->shipRoster.push_back(ship);

	ret->updateFleetVariables(*pd->gd_);
	ret->cargoList.cargoDetail[_fuel].amount = ret->fuelCapacity;

	return ret;
}

// initial destroyer (JOAT + IT) is lame, sorry
// name is always 'Stalwart Defender'
Fleet* makeInitialDestroyer(PlayerData *pd, const std::string &name)
{
	// clone off the ship hull
	Ship *design = new Ship(*(pd->gd_->hullList[destroyer_hull]));
   design->objectId = IdGen::getInstance()->makeDesign(pd->race->ownerID, pd->objectMap);
   pd->objectMap[design->objectId] = design;

	design->setName(name);
	design->designhullID = design->objectId; // it's a design
	design->ownerID      = pd->race->ownerID; // we own it
	design->quantity     = 1;

	// engine
	design->slotList[0]->numberPopulated = design->slotList[0]->maxItems;
	design->slotList[0]->structureID     = pickEngine(*pd)->structureID;

	// mech
	design->slotList[1]->numberPopulated = design->slotList[1]->maxItems;
	design->slotList[1]->structureID     = pd->gd_->getStructureByName("Fuel Tank")->structureID;

	// elect
	design->slotList[2]->numberPopulated = design->slotList[2]->maxItems;
	design->slotList[2]->structureID     = pd->gd_->getStructureByName("Battle Computer")->structureID;

	// armor
	design->slotList[3]->numberPopulated = design->slotList[3]->maxItems;
	design->slotList[3]->structureID     = pickArmor(*pd)->structureID;

	// gp
	design->slotList[4]->numberPopulated = design->slotList[4]->maxItems;
	design->slotList[4]->structureID     = pickScanner(*pd)->structureID;

	// left weapon
	design->slotList[5]->numberPopulated = design->slotList[5]->maxItems;
	design->slotList[5]->structureID     = pickBeamWeapon(*pd)->structureID;

	// right weapon
	design->slotList[6]->numberPopulated = design->slotList[6]->maxItems;
	design->slotList[6]->structureID     = pickTorpedo(*pd)->structureID;

	// done with design
	design->updateCostAndMass(*pd->gd_);

	// add it to the design lists
	pd->designList.push_back(design);
	pd->gd_->playerDesigns.push_back(design);

	// build it into a fleet
	Fleet *ret = new Fleet;
   ret->objectId = IdGen::getInstance()->makeFleet(pd->race->ownerID, false, pd->objectMap);
   pd->objectMap[ret->objectId] = ret;

	std::ostringstream os;
	os << name << ' ' << '#' << ret->objectId;
	ret->setName(os.str());
	ret->ownerID = pd->race->ownerID;

	// add the ship
	Ship *ship = new Ship(*design);
	ship->objectId = ret->objectId;

	ship->setName(name);
	ship->quantity = 1;

	ret->shipRoster.push_back(ship);

	ret->updateFleetVariables(*pd->gd_);
	ret->cargoList.cargoDetail[_fuel].amount = ret->fuelCapacity;

	return ret;
}

// always named "Cotton Picker", not ideal
Fleet* makeInitialJoatMiner(PlayerData *pd, const std::string &name)
{
	// clone off the ship hull
	Ship *design = new Ship(*(pd->gd_->hullList[miniMiner_hull]));
   design->objectId = IdGen::getInstance()->makeDesign(pd->race->ownerID, pd->objectMap);
   pd->objectMap[design->objectId] = design;

	design->setName(name);
	design->designhullID = design->objectId; // it's a design
	design->ownerID      = pd->race->ownerID; // we own it
	design->quantity     = 1;

	// engine
	design->slotList[0]->numberPopulated = design->slotList[0]->maxItems;
	design->slotList[0]->structureID     = pickEngine(*pd)->structureID;

	// mining robots
	StructuralElement *bestBot = pickMiner(*pd);
	design->slotList[1]->numberPopulated = design->slotList[1]->maxItems;
	design->slotList[1]->structureID     = bestBot->structureID;
	design->slotList[2]->numberPopulated = design->slotList[2]->maxItems;
	design->slotList[2]->structureID     = bestBot->structureID;

	// scanner
	design->slotList[3]->numberPopulated = design->slotList[3]->maxItems;
	design->slotList[3]->structureID     = pickScanner(*pd)->structureID;

	// done with design
	design->updateCostAndMass(*pd->gd_);

	// add it to the design lists
	pd->designList.push_back(design);
	pd->gd_->playerDesigns.push_back(design);

	// build it into a fleet
	Fleet *ret = new Fleet;
   ret->objectId = IdGen::getInstance()->makeFleet(pd->race->ownerID, false, pd->objectMap);
   pd->objectMap[ret->objectId] = ret;

	std::ostringstream os;
	os << name << ' ' << '#' << ret->objectId;
	ret->setName(os.str());
	ret->ownerID = pd->race->ownerID;

	// add the ship
	Ship *ship = new Ship(*design);
	ship->objectId = ret->objectId;

	ship->setName(name);
	ship->quantity = 1;

	ret->shipRoster.push_back(ship);

	ret->updateFleetVariables(*pd->gd_);
	ret->cargoList.cargoDetail[_fuel].amount = ret->fuelCapacity;

	return ret;
}

void addHWfleets(PlayerData *pd, std::vector<Fleet*> *hwFleets)
{
	// every race has different starting fleets
	switch( pd->race->primaryTrait )
	{
	case jackOfAllTrades_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Long Range Scout") );
		// armed probe
		hwFleets->push_back( makeScout(true, pd, "Armed Probe") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Santa Maria") );
		// freighter
		if( pd->currentTechLevel.techLevels[construction_TECH] < 4 )
			hwFleets->push_back( makeInitialFreighter(pd, "Teamster") );
		//Ned, else privateer

		// remote miner
		hwFleets->push_back( makeInitialJoatMiner(pd, "Cotton Picker") );

		// destroyer
		hwFleets->push_back( makeInitialDestroyer(pd, "Stalwart Defender") );
		break;

	case alternateReality_PRT:
		//Ned?
		break;

	case interstellarTraveller_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Smaugarian Peeping Tom") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Mayflower") );
		// destroyer
		hwFleets->push_back( makeInitialDestroyer(pd, "Stalwart Defender") );
		//Ned? privateer
		break;

	case packetPhysicist_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Long Range Scout") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Santa Maria") );
		break;

	case spaceDemolition_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Smaugarian Peeping Tom") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Santa Maria") );
		//Ned? minelayer standard
		//Ned? minelayer speed trap
		break;

	case innerStrength_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Smaugarian Peeping Tom") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Santa Maria") );
		break;

	case claimAdjuster_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Smaugarian Peeping Tom") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Santa Maria") );
		//Ned? change of heart
		break;

	case warMonger_PRT:
		// armed scout
		hwFleets->push_back( makeScout(true, pd, "Armed Probe") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Santa Maria") );
		break;

	case superStealth_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Smaugarian Peeping Tom") );
		// colony ship
		hwFleets->push_back( makeColonyShip(pd, "Santa Maria") );
		// small freighter
		hwFleets->push_back( makeInitialFreighter(pd, "Shadow Transport") );
		break;

	case hyperExpansionist_PRT:
		// long range scout
		hwFleets->push_back( makeScout(false, pd, "Smaugarian Peeping Tom") );
		// 3 colony ships
		for(unsigned ct = 0; ct < 3; ++ct)
			hwFleets->push_back( makeColonyShip(pd, "Spore Cloud") );
		break;
	}

	//Ned, add two "Potato Bug" for ARM
}

void GameData::placePlayerHomeworlds(void)
{
	//Ned, real logic please
	const size_t numPlanets = planetList.size();

	Ship *origSB = new Ship(*findShipById(hullList, 2033));
	origSB->setName("Starbase");
	origSB->objectId = 16000;
	const StructuralElement *laserCmp  = getStructureByName("Laser");
	const StructuralElement *moleShCmp = getStructureByName("Mole-skin Shield");

	// first four slots are weapons
	origSB->slotList[0]->numberPopulated = 8;
	origSB->slotList[0]->structureID = laserCmp->structureID;
	origSB->slotList[1]->numberPopulated = 8;
	origSB->slotList[1]->structureID = laserCmp->structureID;
	origSB->slotList[2]->numberPopulated = 8;
	origSB->slotList[2]->structureID = laserCmp->structureID;
	origSB->slotList[3]->numberPopulated = 8;
	origSB->slotList[3]->structureID = laserCmp->structureID;
	// next four are shields
	origSB->slotList[4]->numberPopulated = 8;
	origSB->slotList[4]->structureID = moleShCmp->structureID;
	origSB->slotList[5]->numberPopulated = 8;
	origSB->slotList[5]->structureID = moleShCmp->structureID;
	origSB->slotList[6]->numberPopulated = 8;
	origSB->slotList[6]->structureID = moleShCmp->structureID;
	origSB->slotList[7]->numberPopulated = 8;
	origSB->slotList[7]->structureID = moleShCmp->structureID;

	// all players get the same concentrations
	CargoManifest hwConc;

	// all players get the same starting minerals (plus race points)
	//Ned accel bbs?
	CargoManifest hwMins;
	hwMins.cargoDetail[_ironium  ].amount = 200; //Ned? match
	hwMins.cargoDetail[_germanium].amount = 200;
	hwMins.cargoDetail[_boranium ].amount = 200;

	// foreach player
	for(std::vector<PlayerData*>::iterator i = playerList.begin();
	    i != playerList.end(); ++i)
	{
		const size_t playerIdx = i - playerList.begin();
		PlayerData *pd = *i;

		// give them a starbase
		Ship *mySB = new Ship(*origSB);
		mySB->objectId = IdGen::getInstance()->makeDesign(pd->race->ownerID, pd->objectMap);
		pd->objectMap[mySB->objectId] = mySB;

		mySB->ownerID      = (**i).race->ownerID;
		mySB->designhullID = mySB->objectId;
		mySB->quantity     = 1;
		mySB->updateCostAndMass(*this); // also does armor and shields

		// save it to design list
		playerDesigns.push_back(mySB);
		pd->designList.push_back(mySB);

		// build it
		Fleet *hwSbFleet = new Fleet;
		hwSbFleet->setName("Starbase");
		hwSbFleet->objectId = IdGen::getInstance()->makeFleet(playerIdx+1, false, pd->objectMap);
		pd->objectMap[hwSbFleet->objectId] = hwSbFleet;

		hwSbFleet->ownerID  = mySB->ownerID;
		hwSbFleet->shipRoster.push_back(new Ship(*mySB));
		hwSbFleet->shipRoster[0]->quantity = 1; // give them 1
		hwSbFleet->updateFleetVariables(*this);

		std::vector<Fleet*> hwFleets;
		addHWfleets(*i, &hwFleets);

		int p = rand() % numPlanets;
		for(std::vector<Planet*>::iterator planet = planetList.begin();
			planet != planetList.end(); ++planet)
		{
			Planet *curPlanet = *planet;

			bool myPlanet = false;
			if( planet - planetList.begin() == p )
			{
				if( (**planet).ownerID != 0 )
				{
					++p;
				}
				else
				{
					myPlanet = true;

					if( playerIdx == 0 )
					{
						// first player
						// snapshot the concentrations here
						hwConc = curPlanet->mineralConcentrations;
					}
					else
					{
						// all other players get the same
						curPlanet->mineralConcentrations = hwConc;
					}

					// set homeworld owner id
					curPlanet->ownerID = (**i).race->ownerID;
					//Ned, check for IT and PP
					
					// set homeworld bit
					curPlanet->setIsHomeworld(true);

					// move the starbase to the planet
					hwSbFleet->coordinates = curPlanet->coordinates;

					// move homeworld fleets
					for(size_t fi = 0; fi < hwFleets.size(); ++fi)
					{
						hwFleets[fi]->coordinates = curPlanet->coordinates;

						// add to the fleet lists
						pd->fleetList.push_back(hwFleets[fi]);
						fleetList.push_back(hwFleets[fi]);
					}

					// tell the planet it has a starbase
					curPlanet->starBaseID  = hwSbFleet->objectId;

					// set up initial infrastructure
					curPlanet->infrastructure.factories = 10;
					curPlanet->infrastructure.mines     = 10;
					curPlanet->infrastructure.defenses  = 10;
					curPlanet->infrastructure.scanner   = 1; // true

					//Ned, minerals? everyone seems to have same, plus extra race points
					curPlanet->cargoList = hwMins;

					const bool hasLSP = (**i).race->lrtVector[lowStartingPopulation_LRT] != 0;
					if( univParms_->accelerated_ )
					{
						//Ned HE and IT are different
						unsigned base = hasLSP ? 700 : 950;
						int      off  = hasLSP ?  15 :  14;
						unsigned mul  = hasLSP ?  35 :  50;
						int    growth = (**i).race->growthRate;
						if( growth > off )
						{
							base += (growth - off) * mul;
						}
						curPlanet->cargoList.cargoDetail[_colonists].amount = base;
					}
					else
					{
						if( hasLSP )
							curPlanet->cargoList.cargoDetail[_colonists].amount = 175;
						else
							curPlanet->cargoList.cargoDetail[_colonists].amount = 250;
					}

					// make sure planet is habitable
					curPlanet->currentEnvironment  = (**i).race->optimalEnvironment;
					curPlanet->originalEnvironment = (**i).race->optimalEnvironment;

					curPlanet->currentResourceOutput = (**i).race->getResourceOutput(curPlanet);
					(**i).race->getMaxInfrastructure(curPlanet, curPlanet->maxInfrastructure);
				}
			}

			// players have cloned objects
			Planet *playerView = new Planet(*curPlanet);
			if( !myPlanet )
			{
				playerView->clearPrivateData();
			}
			else
			{
				playerView->yearDataGathered = 2400; // current data

				// add to global list
				fleetList.push_back(hwSbFleet);

				// add a copy to player version
				(**i).fleetList.push_back(new Fleet(*hwSbFleet));
			}

			// put the player's view of the planet on their planet list
			(**i).planetList.push_back(playerView);

		} // foreach planet
	} // foreach player
}

// for a new game, set the player's tech (according to race PRT+LRT)
static void setInitialTech(const Race *rp, PlayerData *pd)
{
	// PRT is a one-hot vector
	size_t prtIdx = size_t(-1);
	for(size_t i = 0; i < rp->prtVector.size(); ++i)
	{
		if( rp->prtVector[i] == 1 )
		{
			prtIdx = i;
			break;
		}
	}

	// PRT sets most of the tech
	switch( prtIdx )
	{
	case jackOfAllTrades_PRT:
	{
		// all tech at 3, except "Start at three" boosts to 4
		for(unsigned j = 0; j < numberOf_TECH; ++j)
		{
			if( rp->expensiveStartAtThree && rp->techCostMultiplier.techLevels[j] == 2 )
			{
				pd->currentTechLevel.techLevels[j] = 4;
			}
			else
			{
				pd->currentTechLevel.techLevels[j] = 3;
			}
		}
	}
		break;

	case alternateReality_PRT: //Ned?
		break;

	case interstellarTraveller_PRT:
		pd->currentTechLevel.techLevels[  propulsion_TECH] = 5;
		pd->currentTechLevel.techLevels[construction_TECH] = 5;
		break;

	case packetPhysicist_PRT: // just energy
		pd->currentTechLevel.techLevels[      energy_TECH] = 4;
		break;

	case spaceDemolition_PRT:
		pd->currentTechLevel.techLevels[  propulsion_TECH] = 2;
		pd->currentTechLevel.techLevels[     biotech_TECH] = 2;
		break;

	case innerStrength_PRT: // no tech!
		break;

	case claimAdjuster_PRT: // a lot of tech
		pd->currentTechLevel.techLevels[     biotech_TECH] = 6;
		pd->currentTechLevel.techLevels[construction_TECH] = 2;
		pd->currentTechLevel.techLevels[      energy_TECH] = 1;
		pd->currentTechLevel.techLevels[     weapons_TECH] = 1;
		pd->currentTechLevel.techLevels[  propulsion_TECH] = 1;
		break;

	case warMonger_PRT:
		pd->currentTechLevel.techLevels[   weapons_TECH] = 6;
		pd->currentTechLevel.techLevels[propulsion_TECH] = 1;
		pd->currentTechLevel.techLevels[    energy_TECH] = 1;
		break;

	case superStealth_PRT: // just electronics 5
		pd->currentTechLevel.techLevels[electronics_TECH] = 5;
		break;

	case hyperExpansionist_PRT: // no tech!
		break;
	}

	// Now do LRTs...

	// IFE gives +1 prop
	if( rp->lrtVector[improvedFuelEfficiency_LRT] )
		++pd->currentTechLevel.techLevels[propulsion_TECH];

	// CE gives +1 prop
	if( rp->lrtVector[cheapEngines_LRT] )
		++pd->currentTechLevel.techLevels[propulsion_TECH];
}

// make a new game from the 'up' definition
void GameData::createFromParms(const char *rootPath, const UnivParms &up)
{
	delete univParms_;
	univParms_ = new UnivParms(up);
	Universe::GenParms gp;
	// square universe
	gp.width   = univSizes[up.size_];
	gp.height  = univSizes[up.size_];
	gp.deadX   = 20;
	gp.deadY   = 20;
	gp.rad     = 16;
	gp.density = univDensities[int(up.density_)] * (gp.height / 100);
	gp.u       = Universe::GenParms::UX;

	// need universe characteristics (and planets)
	theUniverse->startNew(rootPath, gp, &planetList);

	//Ned, not sure why tech costs are in universe...
	for(unsigned i = 0; i < theUniverse->maxTechLevel; i++)
	{
		theUniverse->techCost.push_back(techcosts[i]);
	}

	//Ned? not sure if these should be in universe...
	theUniverse->componentFileName  = "newStarsComponents.xml";
	theUniverse->hullFileName       = "newStarsHulls.xml";
	theUniverse->playerFileBaseName = up.baseName_;

	// create the component list
	std::string cmpFile(theUniverse->componentFileName);
	if( cmpFile[0] != '/' ) // if relative
	{
		cmpFile = rootPath;
		cmpFile += '/';
		cmpFile += theUniverse->componentFileName;
	}
	mxml_node_t *compTree = getTree(cmpFile.c_str());
	StructuralElement::parseTechnicalComponents(
                compTree, componentList);

	// create the players
	for(std::vector<Race*>::const_iterator ri = up.races_.begin();
	    ri != up.races_.end(); ++ri)
	{
		PlayerData *pdp = new PlayerData(this);

		Race *rp = *ri;

		// set player owner id
		rp->ownerID = ri - up.races_.begin() + 1;

		pdp->race = rp; //Ned? should there be a clone? (mem mgmt)
		raceList.push_back(pdp->race); // store to global list

		// give the player initial tech
		setInitialTech(rp, pdp);
		pdp->buildCmpAvail(); // update avail tech cache

		// store the player to global list
		playerList.push_back(pdp);

		rp->updateTerraformAbility(*this);
	}
	//Ned? why is this in universe?
	theUniverse->numPlayers = playerList.size();

	// create the hull list
	std::string hullFile(theUniverse->hullFileName);
	if( hullFile[0] != '/' ) // if relative
	{
		// prepend root path
		hullFile = rootPath;
		hullFile += '/';
		hullFile += theUniverse->hullFileName;
	}
	mxml_node_t *hullTree = getTree(hullFile.c_str());
	parseHulls(hullTree, hullList, *this);

	// let GameData do its thing
   addDefaults();

	for(size_t pi = 0; pi < planetList.size(); ++pi)
	{
		Planet *pl = planetList[pi];

		uint64_t hash = formXYhash(pl->getX(), pl->getY());

		planetMap[hash] = pl;
	}

	// give the players their planets and make it all right
	placePlayerHomeworlds();
}

// handy function to send a string to a player
void GameData::playerMessage(unsigned playerID, unsigned senderID,
    const string &theMessage)
{
	// wrap the string in the PlayerMessage structure
	PlayerMessage *pm = new PlayerMessage();
	pm->playerID = playerID;
	pm->senderID = senderID;
	pm->message  = theMessage;

	// store the message into the main game structure
	playerMessageList.push_back(pm);
}

//---PlayerData---------------------------------------------------------------
PlayerData::PlayerData(GameData *gd): gd_(gd)
{
	race                         = NULL;
	researchNextLowest           = true;
	researchNextSame             = false;
        researchNextID               = 0;
	researchTechHoldback         = 0;
	resourcesAllocatedToResearch = 0;
	researchTechID               = 0;
}

// build the cache of available components
void PlayerData::buildCmpAvail(void)
{
	cmpAvail_.clear();

	// foreach component in the main list
	for(std::vector<StructuralElement*>::iterator i =
	    gd_->componentList.begin();
	    i != gd_->componentList.end(); ++i)
	{
		// if we can build it
		if( (**i).isBuildable(race, currentTechLevel) )
			cmpAvail_.push_back(*i); // cache it
	}
}

const StructuralElement* PlayerData::getBestPlanetScan(void) const
{
	const StructuralElement *ret = NULL;
	for(size_t i = 0; i < cmpAvail_.size(); ++i)
	{
		StructuralElement *sep = cmpAvail_[i];
		if( sep->elementType != planetary_ELEMENT )
			continue;
		if( !sep->scanner.isActive )
			continue;

		if( !ret || sep->scanner.baseScanRange > ret->scanner.baseScanRange )
			ret = sep;
	}

	//Ned NAS gives double non-pen
	//if( race->lrtVector[noAdvancedScanners_LRT] == 1 )
		//ret->scanner.baseScanRange *= 2;

	return ret;
}

// construct from XML
bool PlayerData::parseXML(mxml_node_t *const tree)
{
	if( !gd_ )
	{
		std::cerr << "Error: Bad programming: PlayerData not done building\n";
		return false;
	}

	std::vector<Race*> tempRaceList;
	if( !parseRaces(tree, tempRaceList) )
	{
		std::cerr << "Error parsing race data!\n";
		return false;
	}

	// currently one race per player
	if( tempRaceList.size() != 1 )
	{
		std::cerr << "Error parsing race data -- did not find exactly 1 Race!\n";
		return false;
	}

	race = tempRaceList[0];

	gd_->raceList.push_back(race);

	// find the current tech
	if( !currentTechLevel.parseXML(tree, "CURRENTTECHLEVEL") )
	{
		std::cerr << "Error, could not parse CURRENTTECHLEVEL for race "
		     << race->singularName << endl;
		starsExit("newstars.cpp", -1);
	}

	// cache all the components available to us
	buildCmpAvail();

	// update race terraforming after the tech level is determined
	race->updateTerraformAbility(*gd_);

	GameData &gameData = *gd_;

	mxml_node_t *uPlanetListNode = mxmlFindElement(tree, tree, "PLANET_LIST", NULL, NULL,MXML_DESCEND);
	if( uPlanetListNode != NULL )
		parsePlanets(uPlanetListNode, planetList, false);
	else // something happened to the player's planet data -- reconstruct it
	     // from the universe copy as best as possible
	{
      for(unsigned pIndex = 0; pIndex < gameData.planetList.size(); pIndex++)
      {
         Planet *cp = gameData.planetList[pIndex];
         cp->yearDataGathered = gameData.theUniverse->gameYear;
         if( cp->ownerID == race->ownerID )
            planetList.push_back(cp);
         else
         {
            //we don't know everything about the planet -- put in the basics
				//Ned? use clearPrivateData?
            Planet *newPlanet = new Planet();
            newPlanet->setName(cp->getName());
            newPlanet->ownerID = 0;
            newPlanet->objectId = cp->objectId;
            newPlanet->coordinates = cp->coordinates;

            planetList.push_back(newPlanet);
         }
      }
	}

	if( !currentResearch.parseXML(tree, "CURRENTRESEARCH") )
	{
		cerr << "Error, could not parse CURRENTRESEARCH for race "
		     << race->singularName << endl;
		starsExit("newstars.cpp", -1);
	}

	if( !researchAllocation.parseXML(tree, "RESEARCHALLOCATION") )
	{
		cerr << "Error, could not parse RESEARCHALLOCATION for race "
		     << race->singularName << endl;
		starsExit("newstars.cpp", -1);
	}

	researchNextLowest   = true;
	parseXML_noFail(tree, "RESEARCHNEXTLOWEST",   researchNextLowest);

	researchNextSame     = false;
	parseXML_noFail(tree, "RESEARCHNEXTSAME",     researchNextSame);
        researchNextID     = 0;
        parseXML_noFail(tree,"RESEARCHNEXTID",researchNextID);

	researchTechHoldback = 0;
	parseXML_noFail(tree, "RESEARCHTECHHOLDBACK", researchTechHoldback);

	parseXML_noFail(tree, "RESEARCHTECHID",       researchTechID);

	parseXML_noFail(tree,"RESOURCESALLOCATEDTORESEARCH",
	    resourcesAllocatedToResearch);

	// one and only one
	if( (researchNextLowest == true) && (researchNextSame == true) )
	{
		researchNextLowest = true;
		researchNextSame   = false;
	}

	mxml_node_t *dList = mxmlFindElement(tree, tree, "DESIGNLIST", NULL, NULL,
	    MXML_DESCEND);
	if( dList != NULL )
		parseHulls(dList, designList, gameData);
	unsigned i; // MS bug
	for(i = 0; i < designList.size(); i++)
	{
		designList[i]->updateCostAndMass(gameData);
		designList[i]->quantity = 0;
		gameData.playerDesigns.push_back(designList[i]);
	}

	if( !parseFleets(tree, fleetList, gameData, false) )
	{
		cerr << "Error parsing fleet data!\n";
		return false;
	}

	mxml_node_t *epList = mxmlFindElement(tree, tree, "ENEMY_PLAYER_LIST", NULL, NULL,
	                                     MXML_DESCEND);
	if( !epList )
	{
		std::cerr << "Error parsing enemy player list!\n";
		return false;
	}
	mxml_node_t *epChild = mxmlFindElement(epList, epList, "ENEMY_PLAYER", NULL, NULL,
	                                     MXML_DESCEND);

	while( epChild )
	{
		EnemyPlayer *tmp = new EnemyPlayer;
		tmp->parseXML(epChild);

		knownEnemyPlayers.push_back(tmp);

		epChild = mxmlFindElement(epChild, tree, "ENEMY_PLAYER", NULL, NULL,
			MXML_NO_DESCEND);
	}

	mxml_node_t *prList = mxmlFindElement(tree, tree, "PLAYER_RELATIONS", NULL, NULL,
	                                     MXML_DESCEND);
	if( !prList )
	{
		std::cerr << "Error parsing player relation list!\n";
		return false;
	}
	mxml_node_t *prChild = mxmlFindElement(prList, prList, "PR_ELEMENT", NULL, NULL,
	                                     MXML_DESCEND);
	while( prChild )
	{
		mxml_node_t *child = prChild->child;

		if( child->next )
			playerRelationMap_[child->value.integer] = PlayerRelation(child->next->value.integer);
		else
		{
			std::cerr << "Error in player relation!\n";
			return false;
		}

		prChild = mxmlFindElement(prChild, tree, "ENEMY_PLAYER", NULL, NULL,
			MXML_NO_DESCEND);
	}

	mxml_node_t *edList = mxmlFindElement(tree, tree, "ENEMY_DESIGNS", NULL, NULL,
	    MXML_DESCEND);
	if( edList != NULL )
		parseHulls(edList, knownEnemyDesignList, gameData);
	for(i = 0; i < knownEnemyDesignList.size(); i++)
	{
		knownEnemyDesignList[i]->quantity = 0;
	}

	mxml_node_t *efList = mxmlFindElement(tree, tree, "ENEMY_FLEETS", NULL,
	    NULL, MXML_DESCEND);
	if( !efList || !parseFleets(efList, detectedEnemyFleetList, *gd_, true) )
	{
		cerr << "Error parsing enemy fleet data!\n";
		return false;
	}

	if( !parseMineFields(tree, minefieldList, gameData) )
	{
		cerr << "Error parsing minefield data!\n";
		return false;
	}

	if( !parseBattleOrders(tree, battleOrderList, gameData) )
	{
		cerr << "Error parsing battle Orders!\n";
		return false;
	}

	for(i = 0; i < fleetList.size(); i++)
		gameData.fleetList.push_back(fleetList[i]);

	for(i = 0; i < minefieldList.size(); i++)
		gameData.minefieldList.push_back(minefieldList[i]);

	for(i = 0; i < battleOrderList.size(); i++)
		gameData.battleOrderList.push_back(battleOrderList[i]);

	buildObjectMap();
	return true;
}

void PlayerData::buildObjectMap(void)
{
	size_t i; // MS bug
	for(i = 0; i < fleetList.size(); ++i)
		objectMap[fleetList[i]->objectId] = fleetList[i];

	for(i = 0; i < designList.size(); i++)
	{
		objectMap[designList[i]->objectId] = designList[i];
	}
}

bool PlayerData::hasSeenPlayer(unsigned playerOwnerID) const
{
	if( playerOwnerID == race->ownerID )
		return true; // we can see ourselves

	for(size_t i = 0; i < knownEnemyPlayers.size(); ++i)
		if( knownEnemyPlayers[i]->ownerID == playerOwnerID )
			return true;

	return false;
}


EnemyPlayer* PlayerData::findEnemyPlayer(unsigned playerOwnerID)
{
	for(size_t i = 0; i < knownEnemyPlayers.size(); ++i)
		if( knownEnemyPlayers[i]->ownerID == playerOwnerID )
			return knownEnemyPlayers[i];
	return NULL;
}

void PlayerData::seePlayer(const Race &r)
{
	knownEnemyPlayers.push_back(new EnemyPlayer(r));
}

void PlayerData::toXMLString(std::string &theString) const
{
	const unsigned curPlayer = race->ownerID;

	unsigned i; // MS bug

	// refer to global versions of data
	std::vector<Race*> whichRaceList;
	for(i = 0; i < gd_->raceList.size(); i++)
		if( gd_->raceList[i]->ownerID == curPlayer )
			whichRaceList.push_back(gd_->raceList[i]);

	std::vector<Fleet*> whichFleetList;
	for(i = 0; i < gd_->fleetList.size(); i++)
		if( gd_->fleetList[i]->ownerID == curPlayer )
			whichFleetList.push_back(gd_->fleetList[i]);

	for(i = 0; i < gd_->packetList.size(); i++)
		if( gd_->packetList[i]->ownerID == curPlayer )
			whichFleetList.push_back(gd_->packetList[i]);

	for(i = 0; i < gd_->debrisList.size(); i++)
		if( gd_->debrisList[i]->ownerID == curPlayer )
			whichFleetList.push_back(gd_->debrisList[i]);

	std::vector<Ship*> whichDesignList;
	for(i = 0; i < gd_->playerDesigns.size(); i++)
		if( gd_->playerDesigns[i]->ownerID == curPlayer )
			whichDesignList.push_back(gd_->playerDesigns[i]);

	std::vector<Minefield*> whichMineFields;
	for(i = 0; i < gd_->minefieldList.size(); i++)
		if( gd_->minefieldList[i]->ownerID == curPlayer )
			whichMineFields.push_back(gd_->minefieldList[i]);

	std::string finalString;
	std::string tempString;

	finalString += "<PLAYERDATA>\n";
	racesToXMLString(whichRaceList, tempString);
	finalString += tempString;

	currentTechLevel.toXMLString(tempString, "CURRENTTECHLEVEL");
	finalString += tempString;

	currentResearch.toXMLString(tempString, "CURRENTRESEARCH");
	finalString += tempString;

	researchAllocation.toXMLString(tempString,"RESEARCHALLOCATION");
	finalString += tempString;

	char cTemp[1024];
	sprintf (cTemp,"<RESEARCHNEXTSAME>%i</RESEARCHNEXTSAME>\n", researchNextSame);
	finalString += cTemp;

	sprintf (cTemp,"<RESEARCHNEXTLOWEST>%i</RESEARCHNEXTLOWEST>\n", researchNextLowest);
	finalString += cTemp;

	sprintf (cTemp,"<RESEARCHNEXTID>%i</RESEARCHNEXTID>\n", researchNextID);
	finalString += cTemp;

        sprintf (cTemp,"<RESEARCHTECHHOLDBACK>%i</RESEARCHTECHHOLDBACK>\n", researchTechHoldback);
	finalString += cTemp;

	sprintf (cTemp, "<RESOURCESALLOCATEDTORESEARCH>%i</RESOURCESALLOCATEDTORESEARCH>\n",
	    resourcesAllocatedToResearch);
	finalString += cTemp;

	sprintf(cTemp,"<RESEARCHTECHID>%i</RESEARCHTECHID>\n", researchTechID);
	finalString += cTemp;

	fleetsToXMLString(whichFleetList, tempString);
	finalString += tempString;

	if( battles_.size() )
	{
		finalString += "<BATTLES>\n";
		for(std::vector<BattleReport*>::const_iterator i = battles_.begin(); i != battles_.end(); ++i)
		{
			(**i).toXMLString(tempString);
			finalString += tempString;
		}
		finalString += "</BATTLES>\n";
	}
	else
	{
		finalString += "<BATTLES/>\n";
	}

	finalString += "<ENEMY_PLAYER_LIST>\n";
	for(i = 0; i < knownEnemyPlayers.size(); ++i)
	{
		knownEnemyPlayers[i]->toXMLString(tempString);
		finalString += tempString;
	}
	finalString += "</ENEMY_PLAYER_LIST>\n";

	std::ostringstream os;
	os << "<PLAYER_RELATIONS>\n";
	for(std::map<unsigned, PlayerRelation>::const_iterator it = playerRelationMap_.begin();
		it != playerRelationMap_.end(); ++it)
	{
		os << "<PR_ELEMENT>" << it->first << ' ' << it->second
			<< "</PR_ELEMENT>\n";
	}
	os << "</PLAYER_RELATIONS>\n";
	finalString += os.str();

	finalString += "<ENEMY_FLEETS>\n";
	fleetsToXMLString(detectedEnemyFleetList, tempString);
	finalString += tempString;
	finalString += "</ENEMY_FLEETS>\n";

	minefieldsToXMLString(whichMineFields,tempString);
	finalString += tempString;

	finalString += "<DESIGNLIST>\n";
	for(i = 0; i < whichDesignList.size(); i++)
	{
		Ship *design = whichDesignList[i];
		design->updateCostAndMass(*gd_);
		design->toXMLString(tempString);
		finalString += tempString;
	}
	finalString += "</DESIGNLIST>\n";

	finalString += "<ENEMY_DESIGNS>\n";
	for(i = 0; i < knownEnemyDesignList.size(); ++i)
	{
		Ship *design = knownEnemyDesignList[i];
		design->toXMLString(tempString);
		finalString += tempString;
	}
	finalString += "</ENEMY_DESIGNS>\n";

	finalString += "<BATTLE_ORDER_LIST>\n";
	for(i = 0; i < battleOrderList.size(); i++)
	{
		BattleOrder *bOrder = battleOrderList[i];
		bOrder->toXMLString(tempString);
		finalString += tempString;
	}
	finalString += "</BATTLE_ORDER_LIST>\n";

	finalString += "<PLANET_LIST>\n";
	planetsToXMLString(planetList,tempString);
	finalString += tempString;
	finalString += "</PLANET_LIST>";

	finalString += "</PLAYERDATA>\n";

	theString = finalString;
}

Planet* PlayerData::getPlayerPlanet(const Planet *gamePlanet)
{
	for(std::vector<Planet*>::iterator i = planetList.begin();
	    i != planetList.end(); ++i)
	{
		if( (**i).objectId == gamePlanet->objectId )
			return *i;
	}
	return NULL;
}

void PlayerData::syncPlanetsFromGame(const std::vector<Planet*> &gpl)
{
	for(std::vector<Planet*>::const_iterator i = gpl.begin();
	    i != gpl.end(); ++i)
	{
		*getPlayerPlanet(*i) = **i;
	}
}

unsigned PlayerData::getDefenseValue(void) const
{
	unsigned dVal = 0;
	for(size_t i = 0; i < cmpAvail_.size(); ++i)
	{
		StructuralElement *sep = cmpAvail_[i];
		if( sep->elementType != planetary_ELEMENT )
			continue;
		if( sep->defense > dVal )
			dVal = sep->defense;
	}

	return dVal;
}

// from Stars Faq
// race Fact is in quarters (50% cheaper is 2, normal is 4, 75% is 7)
unsigned calcTechCost(unsigned base, unsigned totalTech, unsigned raceFact, bool slow)
{
	unsigned total = (base + (totalTech * Universe::techKickerPerLevel)) * raceFact / 4;
	if( slow )
		total *= 2;

	return total;
}

// convert from race enum (50% less, normal, 75% more) to
// input to 'calcTechCost'
static const unsigned raceTechMultToCalcMult[3] = {
	2, 4, 7
};

// check for tech advances
void PlayerData::advanceResearchAgenda(void)
{
	// build a list of tech fields to research
	unsigned checkOrder[numberOf_TECH];

	unsigned tid = researchTechID; // start field (tech id)
	unsigned count = 0; // count of techs

	// fill the array
	while( count < numberOf_TECH )
	{
		checkOrder[count] = tid;
		count++;

		// next tech
		tid++;
		if( tid == numberOf_TECH ) // wrap
			tid = 0;
	}

	for(unsigned idx = 0; idx < numberOf_TECH; idx++)
	{
		unsigned i = checkOrder[idx];

		//Note: cost of advancing *OUT* of the current tech
		// is stored in its index, not the cost of advancing in
		unsigned nextTechCost = unsigned(-1);
		if( currentTechLevel.techLevels[i] < Universe::maxTechLevel )
			nextTechCost = calcTechCost(
			    gd_->theUniverse->techCost[currentTechLevel.techLevels[i]],
			    currentTechLevel.scalarSum(),
				 raceTechMultToCalcMult[race->techCostMultiplier.techLevels[i]],
			    gd_->theUniverse->slowTech_);

		while( currentTechLevel.techLevels[i] < Universe::maxTechLevel &&
		       currentResearch.techLevels[i] >= nextTechCost ) // we get a tech level
        {
			  // give the level
            currentTechLevel.techLevels[i]++;
				// pay for it
            currentResearch.techLevels[i] -= nextTechCost;

				// calculate next level cost
            nextTechCost = calcTechCost(
				    gd_->theUniverse->techCost[currentTechLevel.techLevels[i]],
				    currentTechLevel.scalarSum(),
				    raceTechMultToCalcMult[race->techCostMultiplier.techLevels[i]],
				    gd_->theUniverse->slowTech_);

				// notify the player
            char cTemp[1024];
            sprintf (cTemp," %s %i.", TechNameString[i],currentTechLevel.techLevels[i]);
            std::string msg = "Your scientists have pioneered a tech advance:  ";
            msg += cTemp;

            if( (researchNextSame &&
				    currentTechLevel.techLevels[i] < Universe::maxTechLevel) ||
				    i != researchTechID )
            {
                msg += "  They will continue to research ";
                msg +=  TechNameString[i];
                msg += ".";
            }
				else
            {
                // go the the min tech and work on that
                unsigned minTech = 0;
                unsigned u; // MS bug
                for(u = 1; u < numberOf_TECH ; u++)
                {
                    if( currentTechLevel.techLevels[minTech] > currentTechLevel.techLevels[u] )
                        minTech = u;
                }
                if (!researchNextLowest && (currentTechLevel.techLevels[researchNextID]<Universe::maxTechLevel))
                    minTech = researchNextID;

                unsigned allocBase = 0;
                unsigned allocMax = 100;
                if( race->lrtVector[generalizedResearch_LRT] == 1 )
                {
                    allocBase = 15;
                    allocMax = 50;
                }
                for(u = 0; u < numberOf_TECH; u++)
                {
                    researchAllocation.techLevels[u] = allocBase;
                }
                researchAllocation.techLevels[minTech] = allocMax;
                researchTechID = minTech;

                msg += "  They will begin researching ";
                msg += TechNameString[minTech];
                msg += ".";
            }
				// message from server (id 0)
            gd_->playerMessage(race->ownerID, 0, msg);
        }
	}
}

void PlayerData::normalResearch(void)
{
	// get amount of resources available for resources
	// planets have already deposited reserved amount
	// during their process build queue phase
	for(unsigned planetIndex = 0; planetIndex < gd_->planetList.size(); planetIndex++)
	{
		Planet *pl = gd_->planetList[planetIndex];
		if( pl->ownerID == race->ownerID )
	        resourcesAllocatedToResearch += pl->resourcesRemainingThisTurn;
	}
	// bin resources into category, and sum in the global pool for SS stealing

	unsigned totalToUse = resourcesAllocatedToResearch;
	while( totalToUse > 0 &&
	       currentTechLevel.scalarSum() <
	       Universe::maxTechLevel * numberOf_TECH )
	{
		// target level is the level of the tech we are currently researching
		// the base cost for advancing out of the tech is stored in its index
	   unsigned targetLevel = currentTechLevel.techLevels[researchTechID];

		unsigned neededResources =
		    calcTechCost(gd_->theUniverse->techCost[targetLevel],
		        currentTechLevel.scalarSum(),
		        raceTechMultToCalcMult[race->techCostMultiplier.techLevels[researchTechID]],
		        gd_->theUniverse->slowTech_);
		// subtract off banked research
	   neededResources -= currentResearch.techLevels[researchTechID];

		// generalized research, only 50% goes into current field
		if( race->lrtVector[generalizedResearch_LRT] == 1 )
			neededResources *= 2;

		unsigned useThisRound = neededResources;
		if( neededResources > totalToUse )
			useThisRound = totalToUse;

		totalToUse -= useThisRound;

		for(unsigned i = 0; i < numberOf_TECH; i++)
		{
			currentResearch.techLevels[i] += useThisRound * researchAllocation.techLevels[i] / 100;
			gd_->globalResearchPool.techLevels[i] += useThisRound * researchAllocation.techLevels[i] / 100;
		}
		advanceResearchAgenda();
	}
}

void PlayerData::stealResearch(void)
{
    if (race->prtVector[superStealth_PRT] == 0) return;
    std::string msg = "Your spies have stolen the following research points";
    char cTemp[1024];
    for (unsigned i = 0;i<numberOf_TECH;i++)
    {
        unsigned amountToDo = gd_->globalResearchPool.techLevels[i] / gd_->playerList.size() /2;
        currentResearch.techLevels[i] += amountToDo;
        if (amountToDo > 0)
        {
            sprintf(cTemp,": %s %i",TechNameString[i],amountToDo);
            msg += cTemp;
        }
    }
    gd_->playerMessage(race->ownerID,0,msg);
    advanceResearchAgenda();
}

// read in a player data file.  read
// it into its own data structures and should reconcile against the master
// (not yet complete)
bool GameData::parsePlayerTree(mxml_node_t *playerUniverse)
{
	GameData   *playerGameView = new GameData();
	PlayerData *newPlayer      = new PlayerData(playerGameView);

	newPlayer->playerRelationMap_[0] = PR_FRIEND; // befriend non-players

	playerGameView->componentList = this->componentList;
	playerGameView->componentMap  = this->componentMap;
	playerGameView->hullList      = this->hullList;

	mxml_node_t *uPlanetListNode = mxmlFindElement(playerUniverse, playerUniverse, "UNIVERSE_PLANET_LIST", NULL, NULL,MXML_DESCEND);
	parsePlanets(uPlanetListNode, newPlayer->planetList, false);

	playerGameView->playerList.push_back(newPlayer);

	mxml_node_t *playerNode = mxmlFindElement(playerUniverse, playerUniverse, "PLAYERDATA", NULL, NULL,MXML_DESCEND);
	if( !playerNode )
	{
		cerr<<"Error: problem parsing player data\n";
		return false;
	}

	bool result = newPlayer->parseXML(playerNode);
	newPlayer->playerRelationMap_[newPlayer->race->ownerID] = PR_FRIEND; // force alliance with self

	if( !result )
	{
		cerr << "Error: problem parsing player data\n";
		return false;
	}

	// by this point the player's file should be loaded into newPlayer and
	// playerGameView, and we now need to reconcile changes

   cerr<<"Attempting to reconcile player file #"<<newPlayer->race->ownerID<<endl;

	return reconcilePlayerGameView(this, newPlayer);
//   return false;//code not yet complete!  return false here to get rid of
               //warning on intermediate compiles
}


uint64_t      GameData::getNumBuiltOfDesign(uint64_t designID)
{
    uint64_t count=0;
    for (unsigned i = 0; i<fleetList.size();i++)
    {
        Fleet *cf = fleetList[i];
        for (unsigned siter=0;siter<cf->shipRoster.size();siter++)
        {
            Ship *cs = cf->shipRoster[siter];
            if (cs->designhullID==designID)
                count+=cs->quantity;
        }
    }
//    starsExit("structuralElement.cpp",-1);
   return count; //compiler warning otherwise

}

// main construction code, get all the relevant setup information from XML
bool GameData::parseTrees(mxml_node_t *uniTree,
                          bool parsePlayerFiles,
                          const std::string &rootPath)
{
	GameObject::nextObjectId = 30000;  // min starting objectID
	// universe does not include planets, just global info (width, height,
	// filenames, tech costs)
	if( !theUniverse->parseUniverseData(uniTree) )
	{
		cerr << "Error parsing base universe data\n";
		return false;
	}

	// build the component list
	std::string strTmp = theUniverse->componentFileName;
	// if relative path, prepend the root path
	if( strTmp[0] != '/' )
		strTmp = rootPath + '/' + strTmp;

	mxml_node_t *compTree;
	compTree = getTree(strTmp.c_str());
	if( !compTree )
	{
		cerr << "Unable to read component XML file!\n";
		return 1;
	}

	// build the list of base hull types
	strTmp = theUniverse->hullFileName;
	// if relative path, prepend the root path
	if( strTmp[0] != '/' )
		strTmp = rootPath + '/' + strTmp;
	mxml_node_t *hullTree;
	hullTree = getTree(strTmp.c_str());
	if( !hullTree )
	{
		cerr << "Unable to read hull XML file!\n";
		return 1;
	}

	// convert the component XML tree into a vector of components
	if( !StructuralElement::parseTechnicalComponents(
	        compTree, componentList) )
	{
		cerr << "Error: Could not read or parse buildable components list!\n";
		return false;
	}

	// set up Map of component names to vector index
	unsigned i; // MS bug
	for(i = 0; i < componentList.size(); ++i)
	{
		StructuralElement *el = componentList[i];
		componentMap[el->name] = el;
	}

    if (!parseHulls(hullTree,hullList, *this))
    {
        cerr << "Error parsing hull definition data\n";
        return false;
    }
    mxml_node_t *uPlanetListNode = mxmlFindElement(uniTree, uniTree, "UNIVERSE_PLANET_LIST", NULL, NULL,MXML_DESCEND);
    parsePlanets(uPlanetListNode, planetList, false);

    mxml_node_t *playerNode = mxmlFindElement(uniTree, uniTree, "PLAYERDATA", NULL, NULL,MXML_DESCEND);
    while (playerNode != NULL)
    {
        PlayerData *newPlayer = new PlayerData(this);
        playerList.push_back(newPlayer);

		  // force alliances with NPCs and self
        newPlayer->playerRelationMap_[0] = PR_FRIEND;
        newPlayer->playerRelationMap_[playerList.size()] = PR_FRIEND;
        bool result = newPlayer->parseXML(playerNode);
        if (!result)
        {
            cerr<<"Error: problem parsing player data\n";
            return false;
        }
        playerNode = mxmlFindElement(playerNode, uniTree, "PLAYERDATA", NULL, NULL,MXML_NO_DESCEND);
    }

    //only here for file conversion
    if (theUniverse->numPlayers == 0)
        theUniverse->numPlayers = playerList.size();
    //get the messages
    mxml_node_t *messageNode = mxmlFindElement(uniTree,uniTree,"MESSAGE_DATA",NULL, NULL, MXML_DESCEND);
    while (messageNode != NULL)
    {
        PlayerMessage *pm = new PlayerMessage();
        pm->parseXML(messageNode->child);
        playerMessageList.push_back(pm);
        messageNode = mxmlFindElement(messageNode,uniTree,"MESSAGE_DATA",NULL, NULL, MXML_NO_DESCEND);
    }

    mxml_node_t *npcFleetNode = mxmlFindElement(uniTree, uniTree, "NONPLAYERFLEETDATA", NULL,NULL,MXML_DESCEND);
    if (npcFleetNode != NULL)
        if( !parseFleets(npcFleetNode, fleetList,*this, true) )
        {
            cerr << "Error parsing npcFleet data!\n";
            return false;
        }

    for (unsigned u = 0; u< fleetList.size(); u++)
    {
        Fleet *tempFleet = fleetList[u];
		Fleet *origFleetCopy = new Fleet(*tempFleet);
		previousYearFleetList.push_back(origFleetCopy);

        tempFleet->updateFleetVariables(*this);
        if (tempFleet->myType == gameObjectType_packet)
        {
            packetList.push_back(tempFleet);
            deleteFleet(tempFleet->objectId);
        }
        else if (tempFleet->myType == gameObjectType_debris)
        {
            debrisList.push_back(tempFleet);
            deleteFleet(tempFleet->objectId);
        }
    }

   if (theUniverse->masterCopy && parsePlayerFiles)
   {
      for (unsigned z = 0; z<playerList.size();z++)
      {
         PlayerData *whichPlayer = playerList[z];
          /*string fName = rootPath+"/";
		  fName = fName + theUniverse->playerFileBaseName;*/
         string fName = theUniverse->playerFileBaseName;
          char cTemp[256];
         sprintf (cTemp,"_P%i_Y%i.xml",whichPlayer->race->ownerID,theUniverse->gameYear);
         fName += cTemp;
         mxml_node_t *pTree = getTree(fName.c_str());
         parsePlayerTree(pTree);
      }
   }

   processFleetDeletes();

	for(i = 0; i < planetList.size(); ++i)
	{
		Planet *pl = planetList[i];

		uint64_t hash = formXYhash(pl->getX(), pl->getY());

		planetMap[hash] = pl;
	}
	return true;
}

// update the object map with everything we know about.
void PlayerData::updateObjectMap(void)
{
	unsigned u;

	objectMap.clear();
	for (u=0;u<battleOrderList.size();u++)
	{
		GameObject *g = battleOrderList[u];
		objectMap[g->objectId] = g;
	}

	for (u=0;u<designList.size();u++)
	{
		GameObject *g = designList[u];
		objectMap[g->objectId] = g;
	}

	for (u=0;u<detectedEnemyFleetList.size();u++)
	{
		GameObject *g = detectedEnemyFleetList[u];
		objectMap[g->objectId] = g;
	}

	for (u=0;u<fleetList.size();u++)
	{
		GameObject *g = fleetList[u];
		objectMap[g->objectId] = g;
	}

	for (u=0;u<knownEnemyDesignList.size();u++)
	{
		GameObject *g = knownEnemyDesignList[u];
		objectMap[g->objectId] = g;
	}

	for (u=0;u<minefieldList.size();u++)
	{
		GameObject *g = minefieldList[u];
		objectMap[g->objectId] = g;
	}

	for (u=0;u<planetList.size();u++)
	{
		GameObject *g = planetList[u];
		objectMap[g->objectId] = g;
	}
}

void GameData::incYear(void)
{
	// update fleet data
	unsigned i; // MS bug
	for(i = 0; i < fleetList.size(); i++)
		fleetList[i]->updateFleetVariables(*this);
	for(i = 0; i < packetList.size(); i++)
		packetList[i]->updateFleetVariables(*this);
	for(i = 0; i < debrisList.size(); i++)
		debrisList[i]->updateFleetVariables(*this);

	theUniverse->gameYear++;
}

// dump the player XML file AKA the .m# file
void GameData::dumpPlayerFile(PlayerData *whichPlayer) const
{
	theUniverse->masterCopy = false;
	std::string fName = theUniverse->playerFileBaseName;
	char cTemp[256];
	sprintf (cTemp,"_P%i_Y%i.xml",whichPlayer->race->ownerID,theUniverse->gameYear);
	fName += cTemp;
	std::ofstream fileOut;
	fileOut.open(fName.c_str(),ios_base::out | ios_base::trunc);
	if (!fileOut.good())
	{
		cerr<<"Error when opening file: "<<fName<<" for output\n";
		starsExit("newstars.cpp",-1);;
	}

	std::string tempString;
	std::string finalString;

	theUniverse->xmlPrelude(tempString);
	finalString += tempString;

	std::vector<Fleet*> whichFleetList;
	whichPlayer->toXMLString(tempString);
	finalString += tempString;
	unsigned i; // MS bug
	for(i = 0; i < fleetList.size(); i++)
		if( fleetList[i]->ownerID == 0 )
			whichFleetList.push_back(fleetList[i]);

	// reoutput planets into the universe list
	planetsToXMLString(whichPlayer->planetList,tempString);
	finalString += "<UNIVERSE_PLANET_LIST>\n";
	finalString += tempString;
	finalString += "</UNIVERSE_PLANET_LIST>\n";

	finalString += "<NONPLAYERFLEETDATA>\n";
	fleetsToXMLString(whichFleetList, tempString);
	finalString += tempString;
	finalString += "</NONPLAYERFLEETDATA>\n";

	for(i = 0; i < playerMessageList.size(); i++)
	{
		PlayerMessage *m = playerMessageList[i];
		if( m->playerID == whichPlayer->race->ownerID )
		{
			playerMessageList[i]->toXML(tempString);
			finalString += tempString;
		}
	}
	theUniverse->xmlPostlude(tempString);
	finalString += tempString;

    mxml_node_t *unformatedTree;
    char        *formattedTree;

	 // XML version
	 fileOut << "<?xml version=\"1.0\"?>" << std::endl;

    unformatedTree = mxmlLoadString(NULL,finalString.c_str(),MXML_NO_CALLBACK);

    formattedTree = mxmlSaveAllocString(unformatedTree,whitespace_cb);
    if (!formattedTree)
    {
    	fileOut << finalString;
    }
    else
    {
    	fileOut << formattedTree;
    }

    fileOut.close();
   theUniverse->masterCopy = true;
}

void GameData::dumpXML(void)
{
	std::string outString;
	dumpXMLToString(outString);
	std::cout << outString;
}

// dump the complete game state in XML format to string
void GameData::dumpXMLToString(string &theString) const
{
	std::string tempString;
	std::string finalString;

	theUniverse->xmlPrelude(tempString);
	finalString += tempString;

	std::vector<Fleet *> whichFleetList;
	unsigned i; // MS bug
	for (i = 0; i < playerList.size(); i++)
	{
		PlayerData *cp = playerList[i];
		cp->toXMLString(tempString);
		finalString += tempString;
	}
    for (i=0;i<fleetList.size();i++)
        if (fleetList[i]->ownerID == 0)
            whichFleetList.push_back(fleetList[i]);

    finalString += "<NONPLAYERFLEETDATA>\n";
    fleetsToXMLString(whichFleetList, tempString);
    finalString += tempString;
    finalString += "</NONPLAYERFLEETDATA>\n";

    finalString += "<UNIVERSE_PLANET_LIST>\n";
    planetsToXMLString(planetList, tempString);
    finalString += tempString;
    finalString += "</UNIVERSE_PLANET_LIST>\n";

    for (i = 0;i<playerMessageList.size();i++)
    {
        playerMessageList[i]->toXML(tempString);
        finalString += tempString;

    }
    theUniverse->xmlPostlude(tempString);
    finalString += tempString;

    mxml_node_t *unformatedTree;
    char        *formattedTree;

    unformatedTree = mxmlLoadString(NULL,finalString.c_str(),MXML_NO_CALLBACK);
    if (!unformatedTree)
    {
    	 theString = finalString;
    	 return;
    }

    formattedTree = mxmlSaveAllocString(unformatedTree,whitespace_cb);

   theString = formattedTree;
//    cout <<formattedTree;
    free(formattedTree);
    //    cout<<finalString;
}

// return placeable object with id 'targetID'
// right now it is an exhaustive search, inefficient as hell, but
// now isn't the time to optimize for speed...
PlaceableObject* GameData::getTargetObject(uint64_t targetID)
{
   PlaceableObject *temp = getPlanetByID(targetID);
	if( temp )
		return temp;

   unsigned i; // MS bug
   for(i = 0; i < fleetList.size(); i++)
   {
      temp = fleetList[i];
      if( temp->objectId == targetID )
         return temp;
   }

   for(i = 0; i < minefieldList.size(); i++)
   {
      temp = minefieldList[i];
      if( temp->objectId == targetID )
         return temp;
   }

   for(i = 0; i < packetList.size(); i++)
   {
      temp = packetList[i];
      if( temp->objectId == targetID )
         return temp;
   }
   for(i = 0; i < debrisList.size(); i++)
   {
      temp = debrisList[i];
      if( temp->objectId == targetID )
         return temp;
   }
   //assert(false);
   return NULL;
}

// return placeable object with id 'targetID'
// right now it is an exhaustive search, inefficient as hell, but
// now isn't the time to optimize for speed...
Planet* GameData::getPlanetByID(uint64_t targetID)
{
	for(unsigned i = 0; i < planetList.size(); i++)
	{
		Planet *temp = planetList[i];
		if( temp->objectId == targetID )
			return temp;
	}

	return NULL;
}

BattleOrder* GameData::getBattleOrderByID(uint64_t targetID)
{
	for(unsigned i = 0; i < battleOrderList.size(); i++)
	{
		BattleOrder *temp = battleOrderList[i];
		if( temp->objectId == targetID )
			return temp;
	}
	return NULL;
}

// delete fleet by ID
bool GameData::deleteFleet(uint64_t fleetID)
{
	fleetDeletionMap[fleetID] = 1;
	return true;
}

// there are some default things, like default combat orders
// add these to the playerdata if their structures are empty
void GameData::addDefaults(void)
{
	for(unsigned i = 0; i < playerList.size(); i++)
	{
		PlayerData *pd = playerList[i];
		if( pd->battleOrderList.size() == 0 )
		{
			// no orders, create a default order and push it into the list
			BattleOrder *tempOrder = new BattleOrder();
			tempOrder->objectId = GameObject::nextObjectId++;
			tempOrder->setName("Default Battle Orders");
			tempOrder->ownerID = pd->race->ownerID;
			pd->battleOrderList.push_back(tempOrder);
			//assign all fleets the default orders
			for(unsigned j = 0; j < pd->fleetList.size(); j++)
				pd->fleetList[j]->battleOrderID = tempOrder->objectId;
		}
	}
}

bool GameData::processFleetDeletes(void)
{
	for(int i = fleetList.size() - 1; i >= 0; i--)
	{
		uint64_t theID = fleetList[i]->objectId;
		if( fleetDeletionMap[theID] == 1 )
		{
			//Ned? is the iterator good after erase?
			fleetList.erase(fleetList.begin()+i);
		}
	}
	fleetDeletionMap.clear();
	return true;
}

Ship* GameData::getDesignByID(uint64_t targetID)
{
	uint64_t remappedId = targetID;
	std::map<uint64_t,uint64_t>::const_iterator ri = objectIDRemapTable.find(targetID);
	if( ri != objectIDRemapTable.end() )
		remappedId = ri->second;

	for(unsigned i = 0; i < playerDesigns.size(); i++)
	{
		Ship *cd = playerDesigns[i];
		if( cd->objectId == remappedId )
			return cd;
	}
	for(size_t ed = 0; ed < playerList[0]->knownEnemyDesignList.size(); ++ed)
	{
		Ship *cd = playerList[0]->knownEnemyDesignList[ed];
		if( cd->objectId == remappedId )
			return cd;
	}
	//didn't find it
	std::cerr << "Error, could not find design id in getDesignByID() id # "
	    << targetID << std::endl;
	starsExit("newstars.cpp", -1);
	return NULL;
}

PlayerData* GameData::getPlayerData(unsigned targetID)
{
	for(unsigned i = 0; i < playerList.size(); ++i)
	{
		PlayerData *cp = playerList[i];
		if( cp->race->ownerID == targetID )
			return cp;
	}

	return NULL;
}

const PlayerData* GameData::getPlayerData(unsigned targetID) const
{
	for(unsigned i = 0; i < playerList.size(); ++i)
	{
		PlayerData *cp = playerList[i];
		if( cp->race->ownerID == targetID )
			return cp;
	}

	return NULL;
}

//BATTLEBOARD ENGINE
// void GameData::processBattle(std::vector<Fleet*> fList, GameData &gameData)
// {
//     std::map<unsigned,unsigned> playerMap; //what players were involved in this battle
//     std::vector<unsigned> playerList;
//     string mString = "A battle took place between the following Fleets: ";
//     bool printComma = false;
//     char temp[256];
//     for (unsigned i = 0;i< fList.size();i++)
//     {
//         if (playerMap[fList[i]->ownerID] ==0)
//         {
//             playerMap[fList[i]->ownerID] = 1;
//             playerList.push_back(fList[i]->ownerID);
//         }
//         if (printComma) mString += ", ";
//         sprintf(temp,"%I64u",fList[i]->objectId);
//         mString += temp;
//         printComma = true;
//     }
//
//     for (unsigned i = 0;i<playerList.size();i++)
//         gameData.playerMessage(playerList[i],0,mString);
//
//
//     //DO BATTLE STUFF HERE
// }

void PlayerMessage::toXML(string &theString)
{
	char temp[32];
	theString = "";
	theString += "<MESSAGE_DATA>\n";
	theString += "<RCPT>";
	sprintf(temp,"%i",playerID);
	theString += temp;
	theString += "</RCPT>\n<FROM>";
	sprintf(temp,"%i",senderID);
	theString += temp;
	theString += "</FROM>\n";

	theString += "<MSG>";
	theString += message;
	theString += "</MSG>\n";
	theString += "</MESSAGE_DATA>\n";
}

bool PlayerMessage::parseXML(mxml_node_t *tree)
{
    parseXML_noFail(tree, "RCPT", playerID);
    parseXML_noFail(tree, "FROM", senderID);
    parseXML_noFail(tree, "MSG",  message);

    return true;
}

unsigned StarsTechnology::scalarSum()
{
	unsigned s = 0;
	for(unsigned i = 0; i < numberOf_TECH; i++)
	{
		s += techLevels[i];
	}
	return s;
}

void Technology::encompassTechLevel(Technology whatlevel)
{
}

void StarsTechnology::encompassTechLevel(StarsTechnology whatlevel)
{
	for(int i = 0; i < numberOf_TECH ; i++)
		if( techLevels[i] < whatlevel.techLevels[i] )
			techLevels[i] = whatlevel.techLevels[i];
}
// parse the XML stream, but expect that there may or may not be a value
// thus do not output an error message if the node does not exist.
bool parseXML_noFail(mxml_node_t* tree, string nodeName, int &target)
{
    mxml_node_t *child = NULL;

    child = mxmlFindElement(tree, tree, nodeName.c_str(), NULL, NULL,
                            MXML_DESCEND);
    target = 0;
    if( !child )
        return false;

    child = child->child;
    target = atoi(child->value.text.string);
    return true;
}

// parse the XML stream, but expect that there may or may not be a value
// thus do not output an error message if the node does not exist.
bool parseXML_noFail(mxml_node_t* tree, string nodeName, double &target)
{
    mxml_node_t *child = NULL;

    child = mxmlFindElement(tree, tree, nodeName.c_str(), NULL, NULL,
                            MXML_DESCEND);
    target = 0;
    if( !child )
        return false;

    child = child->child;
    target = atof(child->value.text.string);
    return true;
}

bool parseXML_noFail(mxml_node_t* tree, string nodeName, unsigned &target)
{
    mxml_node_t *child = NULL;

    child = mxmlFindElement(tree, tree, nodeName.c_str(), NULL, NULL,
                            MXML_DESCEND);
    target = 0;
    if( !child )
        return false;

    child = child->child;
    target = atoi(child->value.text.string);
    return true;
}

bool parseXML_noFail(mxml_node_t *tree, string nodeName, uint64_t &target)
{
    mxml_node_t *child = NULL;

    child = mxmlFindElement(tree, tree, nodeName.c_str(), NULL, NULL,
                            MXML_DESCEND);
    target = 0;
    if( !child )
        return false;

    child = child->child;
    target = atoll(child->value.text.string);
    return true;
}

bool parseXML_noFail(mxml_node_t *tree, std::string nodeName, bool &target)
{
    mxml_node_t *child = NULL;

    child = mxmlFindElement(tree, tree, nodeName.c_str(), NULL, NULL,
                            MXML_DESCEND);
    target = 0;
    if( !child )
        return false;

    child = child->child;
    target = atoi(child->value.text.string) == 1;
    return true;
}

bool parseXML_noFail(mxml_node_t *tree, std::string nodeName, std::string &target)
{
    mxml_node_t *child = NULL;

    child = mxmlFindElement(tree, tree, nodeName.c_str(), NULL, NULL,
                            MXML_DESCEND);
    target = "";
    if( !child )
        return false;

    child = child->child;
    assembleNodeArray(child,target);
    return true;
}

void initNameArrays(void)
{
	structuralElementCategoryStrings[orbital_ELEMENT] = "Orbital";
	structuralElementCategoryStrings[mechanical_ELEMENT] = "Mechanical";
	structuralElementCategoryStrings[electrical_ELEMENT] = "Electrical";
	structuralElementCategoryStrings[engine_ELEMENT] = "Engine";
	structuralElementCategoryStrings[weapon_ELEMENT] = "Weapon";
	structuralElementCategoryStrings[armor_ELEMENT] = "Armor";
	structuralElementCategoryStrings[shield_ELEMENT] = "Sheild";
	structuralElementCategoryStrings[minelayer_ELEMENT] = "Minelayer";
	structuralElementCategoryStrings[bomb_ELEMENT] = "Bomb";
	structuralElementCategoryStrings[scanner_ELEMENT] = "Scanner";
	structuralElementCategoryStrings[remoteMiner_ELEMENT] = "RemoteMine";
	structuralElementCategoryStrings[planetary_ELEMENT] = "Planetary System";
	structuralElementCategoryStrings[planetaryBuildable_ELEMENT] = "Planetary Buildable";

	structuralElementCategoryCodes[    orbital_ELEMENT] = "ORB";
	structuralElementCategoryCodes[ mechanical_ELEMENT] = "ME";
	structuralElementCategoryCodes[ electrical_ELEMENT] = "EL";
	structuralElementCategoryCodes[     engine_ELEMENT] = "ENG";
	structuralElementCategoryCodes[     weapon_ELEMENT] = "WE";
	structuralElementCategoryCodes[      armor_ELEMENT] = "AR";
	structuralElementCategoryCodes[     shield_ELEMENT] = "SH";
	structuralElementCategoryCodes[  minelayer_ELEMENT] = "ML";
	structuralElementCategoryCodes[       bomb_ELEMENT] = "BM";
	structuralElementCategoryCodes[    scanner_ELEMENT] = "SC";
	structuralElementCategoryCodes[remoteMiner_ELEMENT] = "RM";
	structuralElementCategoryCodes[  planetary_ELEMENT] = "PS";
	structuralElementCategoryCodes[  planetary_ELEMENT] = "PB";

	structuralElementCodeMap["ORB"] = (int)orbital_ELEMENT;
	structuralElementCodeMap["ME"]  = (int)mechanical_ELEMENT;
	structuralElementCodeMap["EL"]  = (int)electrical_ELEMENT;
	structuralElementCodeMap["ENG"] = (int)engine_ELEMENT;
	structuralElementCodeMap["WE"]  = (int)weapon_ELEMENT;
	structuralElementCodeMap["AR"]  = (int)armor_ELEMENT;
	structuralElementCodeMap["SH"]  = (int)shield_ELEMENT;
	structuralElementCodeMap["ML"]  = (int)minelayer_ELEMENT;
	structuralElementCodeMap["BM"]  = (int)bomb_ELEMENT;
	structuralElementCodeMap["SC"]  = (int)scanner_ELEMENT;
	structuralElementCodeMap["RM"]  = (int)remoteMiner_ELEMENT;
	structuralElementCodeMap["PS"]  = (int)planetary_ELEMENT;
	structuralElementCodeMap["PB"]  = (int)planetaryBuildable_ELEMENT;

	mineTypeArray[    _heavyMine] = "Heavy Mines";
	mineTypeArray[ _standardMine] = "Standard Mines";
	mineTypeArray[_speedTrapMine] = "SpeedTrap Mines";

	ColonizerModuleComponent::ColonizerDescriptions[ColonizerModuleComponent::_baseColonizer] = "Basic Colonizer";
	ColonizerModuleComponent::ColonizerDescriptions[ColonizerModuleComponent::_ARColonizer]   = "Alternate Reality Colonizer";
	for(unsigned i = 0; i < (unsigned)num_order; i++)
	{
		orderNameMap[OrderString[i]] = i;
	}
}

void prtLrtCPPString(std::string &theString,
    std::vector<unsigned> acceptablePRTs, std::vector<unsigned> acceptableLRTs)
{
    bool firstPrint = true;
    std::string tempString = "";
    theString = "";
    tempString = "PRTs:";
    bool tempFlag = true;

    int i; // MS bug
    for(i = 0; i < numberOf_PRT; i++)
    {
        if( acceptablePRTs[i] == 1 )
        {
            if( firstPrint )
                tempString += " ";
            else
                tempString += ", ";

            tempString += PRT_NAMES[i];
            firstPrint = false;
        }
        else
            tempFlag = false;
    }
    tempString += "\n";

    if( tempFlag )
        tempString = "PRTs: ALL\n";
    theString += tempString;

    tempString = "LRTs:";
    tempFlag = true;
    bool exclusivityFlag = false;
    firstPrint = true;

    //a one in any LRT field means that it is exclusive to that particular LRT
    //2's are a don't care, and 0s are an absolute exclusion.
    // thus any component with a 1 for any of the values is exclusive
    //2s with 0s mean anything with 2s do not explicitly exclude or include
    //while 0s explicitly exclude

    for(i = 0; i < numberOf_LRT; i++)
        if( acceptableLRTs[i] == 1 )
            exclusivityFlag = true;

    for(i = 0; i < numberOf_LRT; i++)
    {
        if (acceptableLRTs[i]==1)
        {
            if (firstPrint)
                tempString += " ";
            else
                tempString += ", ";
            tempString += LRT_NAMES[i];
            firstPrint=false;
            tempFlag = false;
        }
        else if (acceptableLRTs[i]==0)
        {
            if (firstPrint)
                tempString += " !";
            else
                tempString += ", !";
            tempString += LRT_NAMES[i];
            firstPrint=false;
            tempFlag = false;
        }
    }

    tempString += "\n";
    if (tempFlag)
        tempString = "LRTs: ALL\n";
    theString += tempString;
}

void vectorToString(string &theString, vector<unsigned> vec)
{
	char cTemp[128];
	theString = "";
	for (unsigned i = 0; i < vec.size(); i++)
	{
		sprintf(cTemp, "%i ", vec[i]);
		if( (i+1) == vec.size() )
			sprintf(cTemp,"%i",vec[i]);
		theString += cTemp;
	}
}

const char*                       // O - Whitespace string or NULL
whitespace_cb(mxml_node_t *node,  // I - Element node
              int          where) // I - Open or close tag?
{
	if( where == MXML_WS_AFTER_CLOSE )
		return "\n";

	// Name of element
	const char *name = node->value.element.name;

	bool multiLineHierarchy = false;
	if( !strcmp(name, "NSTARS_UNIV") || !strcmp(name, "UNIV_HDR") || !strcmp(name, "RACELIST") ||
	    !strcmp(name, "RACE") || !strcmp(name, "PLAYERDATA") || !strcmp(name, "HULL")||
	    !strcmp(name, "ORDERLIST") || !strcmp(name, "FLEET") || !strcmp(name, "ORDER") ||
	    !strcmp(name, "SHIPLIST") || !strcmp(name, "PLANET_LIST") || !strcmp(name, "PLANET") ||
	    !strcmp(name, "WARPENGINECOMPONENT") || !strcmp(name, "DESIGNLIST") ||
		 !strcmp(name, "ENEMY_PLAYER_LIST") || !strcmp(name, "ENEMY_PLAYER") ||
		 !strcmp(name, "PLAYER_RELATIONS") || !strcmp(name, "BATTLES") ||
		 !strcmp(name, "ENEMY_FLEETS") || !strcmp(name, "ENEMY_DESIGNS") ||
	    !strcmp(name, "PLANETARYBUILDQUEUE") || !strcmp(name, "NONPLAYERFLEETDATA")||
	    !strcmp(name, "PLANETARYBUILDQUEUEENTRY")|| !strcmp(name, "MESSAGE_DATA") ||
	    !strcmp(name, "MINEFIELD")|| !strcmp(name, "MINEFIELD_LIST") ||
	    !strcmp(name, "BATTLE_ORDER")|| !strcmp(name, "BATTLE_ORDER_LIST") ||
	    !strcmp(name, "UNIVERSE_PLANET_LIST")|| !strcmp(name, "PLANET_LIST")
	  )
		multiLineHierarchy = true;

	if( where == MXML_WS_AFTER_OPEN )
	{
		if( multiLineHierarchy )
			return "\n";
	}

	if( where != MXML_WS_BEFORE_OPEN && !multiLineHierarchy )
		return NULL;

	// find tree depth to this point
	unsigned depth = 0;
	mxml_node_t *temp = node;
	while( temp->parent != NULL )
	{
		depth++;
		temp = temp->parent;
	}

	switch( depth )
	{
	case  0: return NULL;
	case  1: return "   ";
	case  2: return "      ";
	case  3: return "         ";
	case  4: return "            ";
	case  5: return "               ";
	case  6: return "                  ";
	case  7: return "                     ";
	case  8: return "                        ";
	default: return "                           ";
	}
	return NULL;
}

bool GameObject::isValidNewRange(uint64_t theID, unsigned playerNumber)
{
	const uint64_t top24 = theID >> 40;
	if( top24 != 0 ) // player space
		return top24 == playerNumber;
	//else global namespace

	const uint64_t top40 = theID >> 24;
	// check for design
	if( 1 <= top40 && top40 <= 255 )
	{
		return (unsigned(theID) & 0xffffff) == playerNumber;
	}
	// no players allowed (packets and debris created by server)
	return false;
}

//----------------------------------------------------------------------------
IdGen* IdGen::instance_ = NULL;

IdGen::IdGen(void):
   planetCt_(1)
{
}

IdGen* IdGen::getInstance(void)
{
	if( !instance_ )
	{
		instance_ = new IdGen;
	}
	return instance_;
}

uint64_t IdGen::nextPlanet(void)
{
	uint64_t ret = planetCt_;
	++planetCt_;
	return ret;
}

uint64_t IdGen::getDesignId(unsigned pid, unsigned did) const
{
	uint64_t ret = did;
	ret <<= 24;
	ret |= pid;

	return ret;
}

uint64_t IdGen::makeDesign(unsigned pid, const std::map<uint64_t, GameObject*> &map)
{
	unsigned designNum = 1;
	uint64_t ret;

	std::map<uint64_t, GameObject*>::const_iterator i;
	do
	{
		ret = getDesignId(pid, designNum);

		i = map.find(ret);
		++designNum;
	} while( i != map.end() );

	return ret;
}

uint64_t IdGen::makeMinefield(unsigned pid, const std::map<uint64_t, GameObject*> &map)
{
	return 0;
}

uint64_t IdGen::makePacket(const std::map<uint64_t, GameObject*> &map)
{
	return 0;
}

uint64_t IdGen::makeDebris(const std::map<uint64_t, GameObject*> &map)
{
	return 0;
}

uint64_t IdGen::makeFleet(unsigned pid, bool isNew, const std::map<uint64_t, GameObject*> &map)
{
	uint64_t hdr = pid;
	hdr <<= 1;
	hdr |= isNew ? 1 : 0;

	std::map<uint64_t, GameObject*>::const_iterator i;
	uint64_t ret = hdr;
	do
	{
		ret <<= 16;
		ret |= rand() & 0xffff;
		
		ret <<= 16;
		ret |= rand() & 0xffff;

		ret <<= 7;
		ret |= rand() & 0x7f;

		i = map.find(ret);
	} while( i != map.end() );

	return ret;
}

//----------------------------------------------------------------------------
void starsExit(const char *message, int code)
{
	cerr << message << endl;
	exit(code);
}

