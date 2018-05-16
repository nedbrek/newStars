#ifndef NEWSTARS__H
#define NEWSTARS__H
//----------------------------------------------------------------------------
#include "mxml.h"
#include "classDef.h"

#include <vector>
#include <map>
#include <string>
#include <cstdlib>

class BattleReport;
class PlaceableObject;
class Race;
class Fleet;
class GameData; // defined locally
class Planet;

void starsExit(const char *message, int code);

/// hold information about another player
class EnemyPlayer
{
public: // data
	std::string singularName;
	std::string   pluralName;
	unsigned    ownerID;

	EnemyPlayer(void);
	EnemyPlayer(const Race &r);

	bool parseXML(mxml_node_t *tree);
	void toXMLString(std::string &theString) const;
};

/// our relationship to another player
enum PlayerRelation
{
	PR_ENEMY,   /// attack fleets and planets
	PR_NEUTRAL, /// attack only fleets
	PR_FRIEND   /// do not attack
};

/// data structure visible to a player
class PlayerData
{
public: // data
	GameData       *gd_; /// back pointer to main game object
	uint64_t        nextUID_;

	Race           *race; /// race for this player
	StarsTechnology currentTechLevel; /// current tech achieved

	/// resources spent on each next tech level
	StarsTechnology currentResearch;

	/// research Allocation contains a % 0-100 for how much
	/// of each months research goes to each cat
	/// note that gen research races will sum to more than 100
	StarsTechnology researchAllocation;
	bool            researchNextLowest; //Ned? mutually exclusive with below?
	bool            researchNextSame;
        int             researchNextID;

	/// contains % of research to hold back from production
	/// for research
	unsigned  researchTechHoldback;
	unsigned  resourcesAllocatedToResearch;
	unsigned  researchTechID;

	std::map<uint64_t, class GameObject*> objectMap;
	std::vector<Fleet*>                   fleetList;
	std::vector<class Ship*>              designList;
	std::vector<class Minefield*>         minefieldList;
	std::vector<class BattleOrder*>       battleOrderList;
	std::vector<Planet*>                  planetList;

	std::vector<BattleReport*>             battles_;
	std::vector<EnemyPlayer*>              knownEnemyPlayers;
	std::vector<Fleet*>                    detectedEnemyFleetList;
	std::vector<class Ship*>               knownEnemyDesignList;
	std::map<unsigned, PlayerRelation>     playerRelationMap_;
	std::vector<uint64_t>                  remappedObjectVector;

	// cached info
	std::vector<StructuralElement*> cmpAvail_;

	std::map<Location, Fleet*> newBuildMap_;

protected: // methods
	void buildObjectMap(void);

public: // methods
	PlayerData(GameData *gd); // not useful at construction
	bool parseXML(mxml_node_t *tree); // build from XML

	void buildCmpAvail(void); // build avail cache

	bool hasSeenPlayer(unsigned playerOwnerID) const;
	EnemyPlayer* findEnemyPlayer(unsigned playerOwnerID);
	void seePlayer(const Race &r);

	Planet* getPlayerPlanet(const Planet *gamePlanet);
	void syncPlanetsFromGame(const std::vector<Planet*> &gpl);

	void    toXMLString(std::string &theString) const;

	const StructuralElement* getBestPlanetScan(void) const;

	unsigned getDefenseValue(void) const; // best planetary defense they can build
	void     advanceResearchAgenda(void);
	void     normalResearch (void);
	void      stealResearch (void);
	void     updateObjectMap(void);
};

// index into GameData::hullList
enum HullTypes
{
    smallFreighter_hull=0,
    mediumFreighter_hull,
    largeFreighter_hull,
    superFreighter_hull,
    scout_hull,
    frigate_hull,
    destroyer_hull,
    cruiser_hull,
    battleCruiser_hull,
    battleship_hull,
    dreadnought_hull,
    privateer_hull,
    rogue_hull,
    galleon_hull,
    miniColonizer_hull,
    colonizer_hull,
    miniBomber_hull,
    b17Bomber_hull,
    stealthBomber_hull,
    b52Bomber_hull,
    midgetMiner_hull,
    miniMiner_hull,
    miner_hull,
    maxiMiner_hull,
    ultraMiner_hull,
    fuelTransport_hull,
    superFuelTransport_hull,
    miniMinelayer_hull,
    superMinelayer_hull,
    nubian_hull,
    metaMorph_hull,
    numberOf_hull
};

/// contains all the data in the game.
/// universe, fleet and planet lists, player data, etc
class GameData
{
	friend class ActionDispatcher;

public: // data
	// assisting maps
	std::map<std::string, StructuralElement*> componentMap;
	std::map<uint64_t, Planet*>               planetMap;
	std::map<uint64_t, GameObject*>           allMap_;

	// main data structures
    std::vector<Planet*>                  planetList;
    std::vector<Race*>                    raceList;
    std::vector<class Fleet *>            fleetList;
    std::vector<class Fleet *>            previousYearFleetList;
    std::vector<class Fleet *>            packetList;
    std::vector<class Fleet *>            debrisList;
    std::vector<class Minefield *>        minefieldList;
    std::vector<StructuralElement*>       componentList;
    std::vector<class Ship*>              hullList;
    std::vector<class Ship *>             playerDesigns;
    std::vector<class PlayerData*>        playerList;
    std::vector<PlayerMessage *>          playerMessageList;
    std::vector<class BattleOrder *>      battleOrderList;
    class Universe                       *theUniverse;
    int                                   habRangeMultiplier; // multiplier for decimal adjust
    unsigned                              followPhase;
    Environment                           minValues;
    Environment                           maxValues;
    Environment                           onePercentValues;
    StarsTechnology                       globalResearchPool;
	 struct UnivParms                     *univParms_;

protected: // methods
	void placePlayerHomeworlds(void);

public: // methods
    GameData(void);
    ~GameData(void)
    {
        componentMap.clear();
    }

	 void createFromParms(const char *rootPath, const struct UnivParms &up);

    bool parseTrees(mxml_node_t *uniTree,
                    bool parsePlayerFiles,
                    const std::string &rootPath);
   bool parsePlayerTree(mxml_node_t *playerUniverse);
   void addDefaults(void);
   void incYear(void);

   void technicalComponentsToString(std::string &theString);
   void hullListToString(std::string &theString);
   void dumpXML(void);
   void dumpXMLToString(string &theString) const;
   void dumpPlayerFile(PlayerData *whichPlayer) const;

   typedef void PlaceWalker(PlaceableObject *po);
   void walkPlaceables(PlaceWalker *pwf);

   bool hasPlanets(void) const
   {
      return planetList.size() != 0;
   }

   PlaceableObject*   getTargetObject(uint64_t targetID);
   Fleet*             getFleetByID   (uint64_t targetID);
   Fleet*             getLastYearFleetByID(uint64_t targetID);
   Planet*            getPlanetByID  (uint64_t targetID);
   Ship*              getDesignByID  (uint64_t targetID);
   BattleOrder*       getBattleOrderByID(uint64_t targetID);
   Race*              getRaceById    (unsigned id);
	StructuralElement* getStructureByName(const std::string &name);

	const PlayerData*  getPlayerData  (unsigned targetID) const;
	PlayerData*        getPlayerData  (unsigned targetID);
        uint64_t      getNumBuiltOfDesign(uint64_t designID);

   // fleet deletion (primarily as a result of battle)
   std::map<uint64_t,int> fleetDeletionMap;
   bool             deleteFleet(uint64_t fleetID);
   bool             processFleetDeletes(void);

    void  playerMessage(unsigned playerID, unsigned senderID,
        const string &theMessage);

    void  processBattle(std::vector<Fleet*>, GameData &gameData);

   // used when clients create new object in their allocated creation range
   // to remap to globally acceptable new objectIDs
   std::map<uint64_t,uint64_t>               objectIDRemapTable;
};

void initNameArrays(void);
void prtLrtCPPString(std::string &theString, std::vector<unsigned> acceptablePRTs, std::vector<unsigned> acceptableLRTs);

mxml_node_t* getTree(const char *argv);

uint64_t formXYhash(unsigned x, unsigned y);
void generateFiles(const GameData &gameData, const char *path);

Ship* makeScoutDesign(bool armed, PlayerData *pd, const std::string &name);

extern const double PI;

#endif

