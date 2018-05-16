#ifndef CLASSDEF__H
#define CLASSDEF__H
/*****************************************************************************
                          classDef.h -  description
                             -------------------
    begin                : 26 Jan 04
    copyright            : (C) 2004
                           All Rights Reserved.
 ****************************************************************************/

/*****************************************************************************
 This program is copyrighted, and may not be reproduced in whole or
 part without the express permission of the copyright holders.
*****************************************************************************/
#include "placeable.h"
#include "environ.h"

#include "mxml.h"

#include <vector>
#include <string>
#include <map>

using namespace std;

#include <cmath>
#include <cstdlib>

class Race; // race.h
class GameData; // newStars.h

#ifndef MSVC
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
   #ifndef MSVC8
std::ostream& operator<<(std::ostream &o, uint64_t i);
   #endif
#define atoll _atoi64
#pragma warning(disable: 4786)
#pragma warning(disable: 4996)
#endif //not MSVC

// ---------------------------------------------------------------------------
template<typename T>
class deleteT
{
public:
	void operator()(T*& t)
	{
		delete t;
		t = NULL;
	}
};

// constants
//----------------------------------------------------------------------------
enum NSConst
{
	MAXPLAYERSPEED = 10
};

// max number of elements in slot list -- numslots = MAX/2
#define MAX_NUM_SLOTS 256

// denominator for damage unit of a ship e.g. a damage value of 1000 is 1%
// damaged 1000/100000 = 1/100 = 1%
#define FRACTIONAL_DAMAGE_DENOMINATOR 100000

extern std::map<string,int>      structuralElementCodeMap;
extern std::map<string,unsigned> orderNameMap;
extern std::map<string,unsigned> seElementNameMap;

extern string structuralElementCategoryStrings[];
extern string structuralElementCategoryCodes  [];
extern string PRT_NAMES [];
extern string HULL_NAMES[];
extern string LRT_NAMES [];

enum structuralElementCategory
{
    orbital_ELEMENT=0,
    mechanical_ELEMENT,
    electrical_ELEMENT,
    engine_ELEMENT,
    weapon_ELEMENT,
    armor_ELEMENT,
    shield_ELEMENT,
    minelayer_ELEMENT,
    bomb_ELEMENT,
    scanner_ELEMENT,
    remoteMiner_ELEMENT,
    planetary_ELEMENT,
    planetaryBuildable_ELEMENT,
    numberOf_ELEMENT
};

enum hullClassification
{
    freighter_class = 1,
    escort_class = 2,
    warship_class = 3,
    multiroll_class = 4,
    colonizer_class = 5,
    bomber_class = 6,
    remoteMiner_class = 7,
    fuelTransport_class = 8,
    minelayer_class = 9,
    orbitalStation_class = 10
};

// Strings for node types
static const char *const BuildableItemDescriptions[] =
{
   "NOBUILD",
   "Ship Design",
   "Defense",
   "Factory",
   "Mine",
   "Planetary Scanner",
   "Ironium Packet",
   "Boranium Packet",
   "Germanium Packet",
   "Combined Packet",
   "Min Terraform",
   "Max Terraform",
   "Mineral Alchemy",
   "ERROR"
};

/*
 Action is an abstraction that allows for sequencing of events and sending
 "action" notifiers to objects. I envision a single co-ordinator object that
 sends action events to each object subscribing to that event during the
 appropriate phase.  For example, during the "move" phase of the turn the
 coordinator sends move action events to each object that registered a need
 for receiving those types of actions.

 Order of events for actions should be read in from file.
*/
enum actionID
{
   base_action=0,
   planetaryPopulationGrowth_action,
   planetaryUpdatePlanetVariables_action,
   planetaryExtractMineralWealth_action,
   planetaryProcessBuildQueue_action,
	scrapFleet_action,
   fleetMove_action,
   unload_cargo_action,
   gui_unload_action,
   load_cargo_action,
   gui_load_action,
   follow_fleet_action,
   colonize_action,
   resolve_ground_combat_action,
   lay_mines_action,
   decay_mines_action,
   lay_mines_before_move_action,
   standard_bomb_planet_action,
   smart_bomb_planet_action
};

enum orderID
{
   base_order=0,
   move_order,
   remote_mine_order,
   lay_mines_order,
   scrap_fleet_order,
   unload_cargo_order,
   load_cargo_order,
   colonize_world_order,
   patrol_order,
   transfer_fleet_order,
   detonate_mines_order,
   follow_fleet_order,
   gui_unload_order,
   gui_load_order,
   num_order
};

static const char *const OrderString[] =
{
   "base_order",
   "Move to",
   "remote_mine_order",
   "lay_mines_order",
   "Scrap Fleet",
   "Unload Cargo",
   "Load Cargo",
   "Colonize",
   "patrol_order",
   "transfer_fleet_order",
   "detonate_mines_order",
   "follow_fleet_order",
   "GUI Unload",
   "GUI Load",
   "Error - no order"
};

enum techID
{
   energy_TECH=0,
   weapons_TECH,
   propulsion_TECH,
   construction_TECH,
   electronics_TECH,
   biotech_TECH,
   numberOf_TECH
};

static const char *const TechNameString[] =
{
   "Energy",
   "Weapons",
   "Propulsion",
   "Construction",
   "Electronics",
   "Biotech",
   "ERROR - NOTECH"
};

enum mineType
{
   _heavyMine=0,
   _speedTrapMine,
   _standardMine,
   _numberOfMineTypes
};

extern string mineTypeArray[_numberOfMineTypes];

enum TargetType
{
   _noTarget = 0,
   _anyTarget,
   _starbaseTarget,
   _armedShipsTarget,
   _bomberFreighterTarget,
   _unarmedShipsTarget,
   _fuelTransportTarget,
   _freighterOnlyTarget,
   _numTargetTypes
};

static const char *const TargetTypeDescriptions[] =
{
   "None",
   "Any",
   "Starbases/Orbitals",
   "Armed Ships",
   "Bombers and Freighters",
   "Unarmed Ships",
   "Fuel Transports",
   "Freighters",
   "ERROR"
};

enum TacticType
{
   _disengageTactic = 0,
   _disengageIfChallengedTactic,
   _minimizeDamageToSelfTactic,
   _maximizeNetDamageTactic,
   _maximizeDamageRatioTactic,
   _maximizeDamage,
   _numTacticTypes
};

static const char *const TacticTypeDescriptions[] =
{
   "Disengage",
   "Disengage if challenged",
   "Minimize damage to self",
   "Maximize net damage",
   "Maximize damage ratio",
   "Maximize damage",
   "ERROR"
};

class Action
{
public:
   actionID ID;
   string   description; // text description of the action

	Action(actionID i, const string &des = "BASE ACTION"):
	   ID(i), description(des)
   {
   }
};

class PlayerMessage
{
public: // data
   unsigned    playerID;
   unsigned    senderID;
	std::string message;

public: // methods
   void    toXML(std::string &theString);
   bool parseXML(mxml_node_s *tree);
};

class BattleOrder : public GameObject
{
public: // data
   TargetType     primaryTarget;
   TargetType     secondaryTarget;
   TacticType     tactics;
   bool           attackNone;
   bool           attackEnemies;
   bool           attackNeutralsAndEnemies;
   bool           attackEveryone;
   bool           attackSpecificPlayer;
   unsigned       targetPlayer;
   bool           dumpCargo;

public: // methods
   BattleOrder(void);

   // parse data for itself out of this xml branch
   bool parseXML(mxml_node_t *tree);
   // convert to XML
   void toXMLString(string &theString) const;
};

class Order
{
public:
   int      playerNumber;          // which player number this order came from
   unsigned orderNumber;           // index number, starts at 1 each turn
   string   orderDescriptorString; // string describing the base order class
   orderID  myOrderID;             // unique ID indicating what order it is:
   // move, scrap, lay, transfer, etc

   // order details follow ie fleet num, planet num, destination, warp factor, etc
   // these should be encapsulated into constructs for virtual order types, but I am too lazy right now...
   uint64_t      objectId; // what is the object ID this order applies to
   Location      destination;
   unsigned      warpFactor;
   bool          useJumpGate;  //should the fleet use a JumpGate
   unsigned      whichJumpGate; //which jumpgate (componentID)

   union _extraDataSpace
	{
	   unsigned timespan;
	   unsigned interceptRadius;
	   unsigned targetPlayerNumber;
   } extraData;

   CargoManifest cargoTransferDetails;
   uint64_t targetID; // either the ID of the object to transfer cargo to/from,
   // or the player ID for fleet transfers, object ID for follow, etc

public: // methods
   Order(void);
   virtual ~Order(void) {}

   bool parseXML(mxml_node_t *tree);
	void toXMLString(std::string &theString) const;

   virtual bool processOrder(GameData &theGame);
};

class Technology
{
public:
   virtual ~Technology(void) {}

   virtual void toCPPString(string &theString);
   virtual void toXMLString(string &theString);
   virtual bool parseXML(mxml_node_t *tree,string nodeName);

   virtual bool meetsTechLevel(Technology whatLevel);
   virtual void encompassTechLevel(Technology whatlevel);
};

class StarsTechnology : public Technology
{
public: // data
   unsigned techLevels[numberOf_TECH];

public: // methods
   StarsTechnology(void)
   {
      for (unsigned i = 0;i<numberOf_TECH;i++)
      {
         techLevels[i] = 0;
      }
   }

   virtual ~StarsTechnology(void) {}

   void clear(void)
   {
      for(unsigned i = 0; i < numberOf_TECH; i++)
      {
         techLevels[i] = 0;
      }
   }

   // parse data for itself out of this xml branch
   virtual void toCPPString(std::string &theString);
   virtual void toXMLString(std::string &theString, const std::string &nodeName) const;
   virtual bool parseXML(mxml_node_t *tree, std::string nodeName);

   unsigned scalarSum(void);
   virtual bool meetsTechLevel(const StarsTechnology &whatLevel) const;
   virtual void encompassTechLevel(StarsTechnology whatlevel); //increase to at least this level
};

class ComplexComponent
{
public: // data
   bool isActive;

public: // methods
   ComplexComponent(void)
   {
      isActive = false;
   }
   virtual ~ComplexComponent(void) {}

   virtual void toCPPString(string &theString);
   virtual void toXMLString(string &theString);
   virtual bool parseXML(mxml_node_t *tree,string nodeName);
};

class MinelayerComponent : public ComplexComponent
{
public: // data
    int mineLayingRates[_numberOfMineTypes];

public: // methods
    virtual ~MinelayerComponent(void) {}

    // parse data for itself out of this xml branch
    bool parseXML(mxml_node_t *tree);
    virtual void toCPPString(string &theString);
    virtual void toXMLString(string &theString);
};

class BombComponent : public ComplexComponent
{
public: // data
   bool isSmart;
   int  popKillRate; // in 10th's of a percent
   int  minPopKill;
   int  infrastructureKillRate; // in absolute numbers

public: // methods
   virtual ~BombComponent(void) {}

   // parse data for itself out of this xml branch
   bool parseXML(mxml_node_t *tree);
   virtual void toCPPString(string &theString);
   virtual void toXMLString(string &theString);
};

class BeamWeaponComponent : public ComplexComponent
{
public: // data
   bool     isGatlingCapable;
   bool     isShieldSapper;
   unsigned range;
   std::vector<unsigned> damageFactors;

public: // methods
   virtual ~BeamWeaponComponent(void) {}

   // parse data for itself out of this xml branch
   bool parseXML(mxml_node_t *tree);
   virtual void toCPPString(string &theString);
   virtual void toXMLString(string &theString);

   unsigned damageAtRange(unsigned int range);
};

class MissleWeaponComponent : public ComplexComponent
{
public: // data
   unsigned damage;
   bool     isCapitalShipMissle;
   int      accuracy; //in % accurate
   unsigned range;

public: // methods
   virtual ~MissleWeaponComponent(void) {}

   // parse data for itself out of this xml branch
   bool parseXML(mxml_node_t *tree);
   virtual void toCPPString(string &theString);
   virtual void toXMLString(string &theString);
};

class ScanComponent : public ComplexComponent
{
public: // data
   unsigned baseScanRange;
   unsigned penScanRange;

public: // methods
	ScanComponent(void)
	{
		baseScanRange = 0;
		penScanRange  = 0;
	}
   virtual ~ScanComponent(void) {}

   bool parseXML(mxml_node_t *tree);
   // parse data for itself out of this xml branch
   virtual void toCPPString(string &theString);
   virtual void toXMLString(string &theString);
};

class JumpgateComponent : public ComplexComponent
{
public: // data
   int  massLimit;
   int  distanceLimit;
   bool infiniteMass;
   bool infiniteDistance;
   bool useTargetGateSpecs;

public: // methods
   virtual ~JumpgateComponent(void) {}

   // parse data for itself out of this xml branch
   bool parseXML(mxml_node_t *tree);
   virtual void toCPPString(string &theString);
   virtual void toXMLString(string &theString);
};

class WarpEngineComponent : public ComplexComponent
{
public: // data
    int  battleSpeed;
    int  maxSafeSpeed;
    bool isScoop;
    bool dangerousRadiation;
    // fuel consumption/generation data for speeds 0-10
    int  fuelConsumptionVector[MAXPLAYERSPEED +1];

public: // methods
    WarpEngineComponent(void)
    {
		clear();
    }
    virtual ~WarpEngineComponent(void) {}

	 void clear(void)
	 {
		 isActive = false;
       battleSpeed = 0;
       maxSafeSpeed = 0;
       isScoop = false;
       dangerousRadiation = false;
		 memset(fuelConsumptionVector, 0, sizeof(int)*(MAXPLAYERSPEED+1));
	 }

	 // don't know the unit for this yet...
    // parse data for itself out of this xml branch
    bool parseXML(mxml_node_t *tree);
    virtual void toCPPString(string &theString);
    virtual void toXMLString(string &theString);
};

class ColonizerModuleComponent : public ComplexComponent
{
public:
    enum ColonizerType {_baseColonizer=0, _ARColonizer, _numberOfColonizerTypes};

    static string ColonizerDescriptions[_numberOfColonizerTypes];

    ColonizerType colonizerType;

    virtual ~ColonizerModuleComponent(void)
    {}

    // parse data for itself out of this xml branch
    bool parseXML(mxml_node_t *tree);
    virtual void toCPPString(string &theString);
    virtual void toXMLString(string &theString);
};

class StructuralElement
{
public: // methods
    static bool parseTechnicalComponents(mxml_node_t *tree,
                                         std::vector<StructuralElement*> &list);
    //static void technicalComponentsToString(string &theString);

    // parse data for itself out of this xml branch
    bool parseXML(mxml_node_t *tree);
    void toCPPString(string &theString);
    void toXMLString(string &theString);

    // returns true if the component is buildable
    // by the race indicated at the given tech level
    bool isBuildable(Race *whichRace, StarsTechnology whichTechLevel);

public: // data
    static unsigned nextID; // this is the next unique ID - also will be the
    // number of components read

    unsigned                  structureID; // unique ID that identifies the
    // component created at load time when the component database is read in
    // these will be assigned in the order in which the components are read

    std::vector<unsigned>     acceptableHulls;
    std::vector<unsigned>     acceptablePRTs;
    std::vector<unsigned>     acceptableLRTs;
    unsigned                  accuracyImprovement;
    unsigned                  armor;
    unsigned                  baseInitiative;
    unsigned                  beamReflection;
    unsigned                  cargoCapacity;
    unsigned                  cloak;
    unsigned                  defense;
    structuralElementCategory elementType;
    unsigned                  energyCapacitance;
    unsigned                  energyDampening;
    bool                      fleetTheftCapable;
    unsigned                  fuelCapacity;
    unsigned                  fuelGen;
    unsigned                  jam;
    unsigned                  mass;
    CargoManifest             mineralCostBase;
    unsigned                  mineralGenRate;  // rate at which this generates minerals out of thin air
    string                    name;
    unsigned                  packetAcceleration;
    unsigned                  planetMineRate;  // rate at which this planet based component extracts minerals
    bool                      planetTheftCapable;
    unsigned                  remoteMineing;
    bool                      remoteTerraformCapable;
    unsigned                  resourceCostBase;
    unsigned                  resourceGenRate; // rate of resource generation
    unsigned                  shields;
    unsigned                  tachyonDetection;
    StarsTechnology           techLevel;
    unsigned                  techTreeID;
    Environment               terraformAbility;
    unsigned                  thrust;  // combat move bonus-overthrusters, etc

    BeamWeaponComponent       beam;
    BombComponent             bomb;
    ColonizerModuleComponent  colonizerModule;
    WarpEngineComponent       engine;
    JumpgateComponent         jumpgate;
    MinelayerComponent        minelayer;
    MissleWeaponComponent     missle;
    ScanComponent             scanner;
};

// for possible future expansion
class DamageObject
{
public:
    int percentFunctional(void)
    {
        return 100;
    }
};

class ShipSlot
{
public: // methods
    ShipSlot(void);

public: // data
    unsigned slotID;
    unsigned maxItems;        // maximum number of elements that can be placed in the slot
    unsigned numberPopulated; // the current number of elements placed in the slot
    bool     acceptableElements[numberOf_ELEMENT]; // which types of elements can be placed in the slot

    unsigned structureID; // which Structure is resident

    std::vector<DamageObject*> damageReport;
};

class Ship : public GameObject
{
public: // data
	//   StructuralDesign   components;
	uint64_t               designhullID;   // pointer to the base design
    unsigned               baseHullID;     // pointer to the base ship hull
    std::vector<ShipSlot*> slotList;

    unsigned               quantity;
    unsigned               percentDamaged;
    unsigned               numberDamaged;

    bool                   fleetable;         // from updateCostAndMass()
    bool                   hasColonizer;
    bool                   isSpaceStation;    // from updateCostAndMass()
    bool                   hasJumpGate;       // from updateCostAndMass()
    double                 combatMove;        // from updateCostAndMass()
    unsigned               combatRating;
    unsigned               shieldMax;         // from updateCostAndMass()
    unsigned               shields;           // from updateCostAndMass()
    unsigned               armor;             // from updateCostAndMass()
    unsigned               fuelCapacity;      // from updateCostAndMass()
    unsigned               cargoCapacity;     // from updateCostAndMass()
    unsigned               mass;              // from updateCostAndMass()
    unsigned               baseResourceCost;  // from updateCostAndMass()
    unsigned               totalResourceCost; // from updateCostAndMass()
    CargoManifest          baseMineralCost;   // from updateCostAndMass()
    CargoManifest          totalMineralCost;  // from updateCostAndMass()
    unsigned               initiative;        // from updateCostAndMass()
    unsigned               baseInitiative;
    unsigned               maxPacketSpeed;
    unsigned               cloak;
    unsigned               jam;
    unsigned               fuelGen;           // from updateCostAndMass()
    unsigned               damageRepair;      // from updateCostAndMass()
    WarpEngineComponent    warpEngine;        // from updateCostAndMass()
    unsigned               numEngines;        // from updateCostAndMass()
    unsigned               dockCapacity;      // from updateCostAndMass()

    unsigned               hullClass;         // from updateCostAndMass(), hull class for hull type groups

    std::vector<unsigned>  acceptablePRTs;
    std::vector<unsigned>  acceptableLRTs;
    StarsTechnology        techLevel;
    int                    mineLayingRates[_numberOfMineTypes];

protected:
	void copyFrom(const Ship &b, bool newSlots);

public: // methods
   Ship(void);
   Ship(const Ship &b);

	Ship& operator=(const Ship &b);
   bool operator==(const Ship &param) const;

   bool isBuildable(const class PlayerData *pd) const;

   bool parseXML(mxml_node_t *tree, GameData &gd);  //parse data for itself out of this xml branch
   void toXMLString(string &theString);
   void toCPPString(string &theString, GameData &gameData);

   bool parseHullSlots(mxml_node_t *tree, GameData &gd); //parse the hullslots string

	// possibly negative for scoops
	int calcFuel(const Location &coord, const Location &dest, unsigned spd, unsigned cargo) const;

	ScanComponent getScan(const PlayerData &pd) const;

   void updateCostAndMass(GameData &gameData); //update the cost of the ship -- will be reflected in
    //total and base resource cost, and total & base mineralCost

//    void recalculateTotals(void); //recalculate the totals based on current structural elements on the ship
//    void resetShields     (void); //reset shields based on current shields on the ship -- for after a battle

	void clearPrivateData(void);
	void clearDesignData (void);
};

struct CargoMoveCap
{
	unsigned idx; // index into ship roster
	unsigned numShips; // number of ships
	unsigned eff; // engine efficiency at current speed
	unsigned cargoCap; // total capacity
	unsigned assignedCargo; // current assigned

	CargoMoveCap(unsigned i, unsigned spd, const Ship *ship)
	{
		idx = i;
		numShips = ship->quantity;
		eff = ship->warpEngine.fuelConsumptionVector[spd];
		cargoCap = ship->cargoCapacity;

		assignedCargo = 0;
	}

	bool operator<(const CargoMoveCap &b) const
	{
		// sort by efficiency
		if( eff < b.eff )
			return true;
		if( eff > b.eff )
			return false;
		//==, use number of ships to break ties (get things over faster)
		return numShips > b.numShips;
	}
};

// also covers starbases
class Fleet : public PlaceableObject
{
public: // data
    uint64_t           battleOrderID;
    unsigned           totalCargoCapacity;
    unsigned           fuelCapacity;
    unsigned           mass;
    unsigned           maxPacketSpeed;      //max packet speed (if flinger available)
    unsigned           fleetID;            //fleet id -- not necessarily unique
    bool               hasColonizer;
    bool               isSpaceStation;
    bool               hasJumpGame;
    int                topSpeed;
    int                topSafeSpeed;
    int                lastKnownSpeed;      //last known speed and direction largely provided for client simplicity
    int                lastKnownDirection;
    int                mineLayingRates[_numberOfMineTypes];
    unsigned           followDistance;
    std::vector<Ship*> shipRoster;

public: // methods
   Fleet(void);

	// copy versions deep deep copy
   Fleet(const Fleet &cFleet);
	Fleet& operator=(const Fleet &b); // not implemented

   bool          parseXML(mxml_node_t *tree, GameData &gd);  //parse data for itself out of this xml branch
   virtual void  toXMLString(string &theString) const;

   void          clean(GameData &gameData);
	void          clearPrivateData(void);

   virtual bool  actionHandler(GameData &gd, const Action &what);

   virtual void  updateFleetVariables(GameData &gameData);

	enum ScrapCase
	{
		SCRAP_COLONIZE
		, SCRAP_STARBASE
		, SCRAP_NO_STARBASE
	};
	// calculate scrap value of fleet where 'whichRace' GETS the scrap
   CargoManifest scrapValue(const Race *whichRace, ScrapCase sc, unsigned *res) const;

   unsigned      getDockCapacity(void);

	void assignMass(unsigned spd, std::map<unsigned, CargoMoveCap> *pMassAssign) const;

	int calcFuel(const Location &dest, unsigned spd,
	    std::map<unsigned, CargoMoveCap> *pMassAssign,
	    const Location *start = NULL) const;

	ScanComponent getScan(const GameData &gameData) const;

	Ship* hasShip(uint64_t designId) const;

protected: // methods
   virtual
   bool unloadAction  (GameData &gd, bool guiBased);
   bool moveAction    (GameData &gd);
   bool useJumpGate   (GameData &gd);
   bool followAction  (GameData &gd);
   bool colonizeAction(GameData &gd);
   bool layMinesAction(GameData &gd, int layPhase);
   bool bombAction    (GameData &gd);
	bool scrapAction   (GameData &gd);

	void copyFrom(const Fleet &b);
};

bool isIdentical(Fleet* fleetA, Fleet* fleetB);
void fleetsToXMLString(vector<Fleet*> list, string &theString);
void minefieldsToXMLString(vector<class Minefield*> list, string &theString);

bool parseFleets      (mxml_node_t *tree, vector<Fleet*> &list, GameData &gd);
bool parseHulls       (mxml_node_t *tree, vector<Ship*> &list, GameData &gd);
bool parseMineFields  (mxml_node_t *tree, vector<class Minefield*> &list, GameData &gd);
bool parseBattleOrders(mxml_node_t *tree, vector<class BattleOrder*> &list, GameData &gd);

class Debris : public Fleet
{
public:
   Debris(unsigned owner, Location place, CargoManifest stuff);
   Debris(Fleet *flt);
   Debris(void)
   {
      myType = gameObjectType_debris;
   }
   virtual ~Debris(void) {}

   void toXMLString(string &theString) const;

   virtual bool actionHandler(GameData &gd, const Action &what)
   {
      return false;
   }
};

class MassPacket : public Fleet
{
public: // data
   unsigned throwerRating;

public: // methods
   MassPacket(void)
   {
        throwerRating = 0;
        myType = gameObjectType_packet;
   }
   MassPacket(Fleet * flt);
   virtual ~MassPacket(void) {}

   void toXMLString(string &theString) const;

   virtual bool actionHandler(GameData &gd, const Action &what);
   virtual void updateFleetVariables(GameData &gameData);
};

class Minefield : public PlaceableObject
{
public: // data
   unsigned mineComposition[_numberOfMineTypes];
   unsigned radius;
   unsigned nonPenScanCloakValue;
   unsigned penScanCloakValue;

public: // methods
   Minefield (void);

	//---serializations
   // parse data for itself out of this xml branch
   bool parseXML(mxml_node_t *tree);

   void toXMLStringGuts(string &theString) const;
   virtual void toXMLString(string &theString) const {toXMLStringGuts(theString);}

	//---Placeable iface
   virtual bool actionHandler(GameData &gd, const Action &what);

	//---const methods
   bool     canAdd      (Location l, int addComposition[]) const;
   bool     withinRadius(Location l) const;
   unsigned maxSafeSpeed(const Race *whichRace) const;
   double   hitProbability(unsigned speed, const Race *fleetOwner) const;

	// coarse grain hit detect
   bool     needToWorry(Location orig, Location dest, int px1, int py1, int pr) const;

	// we are traveling from pi to pf -- how far did we get, if you interfered?
	// if the minefield was not struck, the actual distance from pi to pf is
	// returned as calculated by pi.distanceFrom(pf);
   unsigned whereStruckMinefield(Location pi, Location pf, unsigned speed, Race *fleetOwner) const;

	//---non const
   void addMines(Location l, int addComposition[]);
   void decay(GameData &gd);

	// also updates this for lost mines
   void damageFleet(GameData &gd, Fleet *theFleet);
};

class IdGen
{
protected:
	static IdGen *instance_;

	unsigned planetCt_;

	IdGen(void);
	IdGen(const IdGen&); // not implemented

public:
	static
	IdGen* getInstance(void);

	uint64_t getDesignId(unsigned pid, unsigned did) const;

	uint64_t nextPlanet(void);
	uint64_t makeDesign   (unsigned pid, const std::map<uint64_t, GameObject*> &map);
	uint64_t makeMinefield(unsigned pid, const std::map<uint64_t, GameObject*> &map);
	uint64_t makePacket   (const std::map<uint64_t, GameObject*> &map);
	uint64_t makeDebris   (const std::map<uint64_t, GameObject*> &map);
	uint64_t makeFleet    (unsigned pid, bool isNew, const std::map<uint64_t, GameObject*> &map);
};

#endif

