#ifndef PLANET__H
#define PLANET__H
// C++ Interface: planet
//
// Copyright: See COPYING file that comes with this distribution
//
#include "environ.h"
#include "placeable.h"

#include <map>
#include <string>
#include <vector>

struct mxml_node_s;

class Fleet;
class Race;

extern const unsigned fractionalCompletionDenominator;

struct WaypointDropDetail
{
	Race *theRace;
	unsigned number; // number of colonists
};

class PlanetaryInfrastructure
{
public: // methods
	PlanetaryInfrastructure(void)
	{
		clear();
	}

	PlanetaryInfrastructure operator+(const PlanetaryInfrastructure &rhs);
	PlanetaryInfrastructure operator-(const PlanetaryInfrastructure &rhs);

	void clear(void)
	{
		mines     = 0;
		defenses  = 0;
		factories = 0;
		scanner   = 0;
	}

public: // data
	unsigned mines;
	unsigned defenses;
	unsigned factories;
	unsigned scanner;
};

enum buildableItemType
{
    _noBuild_BUILDTYPE = 0,
    _ship_BUILDTYPE,
    _defense_BUILDTYPE,
    _factory_BUILDTYPE,
    _mine_BUILDTYPE,
    _scanner_BUILDTYPE,
    _ironiumPacket_BUILDTYPE,
    _boraniumPacket_BUILDTYPE,
    _germaniumPacket_BUILDTYPE,
    _combinedPacket_BUILDTYPE,
    _minTerraform_BUILDTYPE,
    _maxTerraform_BUILDTYPE,
    _mineralAlchemy_BUILDTYPE,
    _numBuildableItemTypes_BUILDTYPE
};

class BuildQueueEntry
{
public:
   unsigned          ownerID;
   uint64_t          planetID;
   buildableItemType itemType;
   uint64_t          designID; //hull design ID, if it is in fact a ship
   unsigned          originalBuildQuantity;
   unsigned          remainingBuildQuantity;
   unsigned          percentComplete;

   bool autoBuild;

public: // methods
   BuildQueueEntry(void);

   bool parseXML(mxml_node_s *tree);
	void toXMLString(std::string &theString);
};

class Planet : public PlaceableObject
{
protected: // methods
	void processBuildQueue(GameData &gd);
	// build a single item
	void buildOneFromQueueEntry(BuildQueueEntry *, GameData &gd);

	// split off partially completed autobuild Items
	// and reset autobuild remainingBuildQuantity
	void cleanBuildQueue(GameData &gd);

	void resolveGroundCombat(GameData &gd);
	void resolveBombingRuns(GameData &gd, bool smart);

	unsigned getFlingerSpeed(GameData &gd);

protected: // data
	CargoManifest packetBuild;

public:
	// construct a planet
	Planet(void);
	bool parseXML(mxml_node_s *tree); // parse data from xml branch

	// format data for export
	virtual void toXMLString(std::string &theString) const;

	// turn generation
	virtual bool actionHandler(GameData &gd, const Action &what);

	void abandonPlanet(void);

	// convert a server planet to a hidden player planet
	void clearPrivateData(void);

	// convert a server planet to a scanned planet
	void clearScanData(void);

	// accessors
	unsigned getPopulation(void) const; // in kT
	unsigned getMines     (void) const; // actual mines built
	unsigned getFactories (void) const; // actual

	bool     isHomeworld  (void) const { return isHomeworld_; }
	void  setIsHomeworld  (bool val)   { isHomeworld_ = val; }

	CargoManifest getMineralConcentrations(void) const;

	// propagate data from 'this' to 'target'
	void propagateData(const std::map<uint64_t, uint64_t> &remapTbl,
	    Planet *target);

public: // data
	std::vector<BuildQueueEntry*>    planetBuildQueue;
	std::vector<WaypointDropDetail*> waypointDropRecords;
	Environment                      currentEnvironment;
	Environment                      originalEnvironment;
	PlanetaryInfrastructure          infrastructure;
	PlanetaryInfrastructure          maxInfrastructure;     // (calculated for client)
	CargoManifest                    mineralConcentrations;
	unsigned                         popRemainder_;
	unsigned                         currentResourceOutput; // calculated resources from planet
	unsigned                         resourcesRemainingThisTurn;
	uint64_t                         packetTarget; // objectID of any created packets
	uint64_t                         routeTarget;  // objectID of any route command target
	unsigned                         packetSpeed;
	unsigned                         yearDataGathered; // year the information about this planet was gathered
	uint64_t                         starBaseID;    // ObjectID of this object's starbase fleet
	std::vector<Fleet*>              bombingList;
	bool                             isHomeworld_;
};

// parse the planet information out of the XML root
bool parsePlanets(mxml_node_s *tree, std::vector<Planet*> &list, bool initMode);

// dump a single planet (to CPP or XML)
void planetToCPPString(Planet *thePlanet, std::string &theString);
void planetToXMLString(Planet *thePlanet, std::string &theString);

// dump all planets
void planetsToXMLString(const std::vector<Planet*> &list, std::string &theString);

class BombCalculations
{
public:
	// the normal bombs bool tells the algo to calculate with only normal (and
	// LBU) bombs (true) or with only smart Bombs (false)
	static unsigned popKilledBombing(GameData &gd,
	    std::vector<Fleet*> bombingFleet, Planet *defendingPlanet,
	    bool normalBombs);

	static PlanetaryInfrastructure infraKilledBombing(GameData &gd,
		std::vector<Fleet*> bombingFleet, Planet *defendingPlanet,
	   bool normalBombs);

	static double defensePopulation(unsigned defenseValue, unsigned quantity, bool isSmartAttack, bool isInvasion);
	static double defenseBuilding  (unsigned defenseValue, unsigned quantity, bool isSmartAttack, bool isInvasion);
};

#endif

