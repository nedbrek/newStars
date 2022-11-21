/// NewStars Interface from Tcl to C++
/// Export a command "newStars" allowing Tcl to query the state
/// of game objects

// ahh, the joys of MSVC
#ifdef MSVC
#pragma warning(disable: 4786)
#define strcasecmp stricmp
#include <direct.h>
typedef unsigned __int64   uint64_t;
typedef          __int64    int64_t;
#else
#include <unistd.h>
#include <stdint.h>
#endif

#include "gen.h"
#include "actionDispatcher.h"
#include "universe.h"
#include "race.h"
#include "newStars.h"
#include "classDef.h"
#include "planet.h"

#include <tcl.h>
#include <cassert>
#include <fstream>
#include <sstream>
#include <algorithm>

//----------------------------------------------------------------------------
template<typename Type>
void vectorDelete(std::vector<Type> &v, unsigned idx)
{
   for(unsigned i = idx; i < v.size()-1; i++)
      v[i] = v[i+1];

   v.pop_back();
}

template<typename Type>
void vectorInsertAt(std::vector<Type> &v, Type &t, unsigned idx)
{
   for(unsigned i = v.size()-1; i > idx; i--)
      v[i] = v[i-1];

   v[idx] = t;
}

//----------------------------------------------------------------------------
/// interface to Tcl parameters like an array
class Parms
{
protected:
   Tcl_Interp     *interp_;
   Tcl_Obj *CONST *objv_;
   unsigned        objc_;

public:
   Parms(Tcl_Interp *interp, Tcl_Obj *CONST* objv, unsigned objc):
      interp_(interp), objv_(objv), objc_(objc) {}

   Tcl_Interp* getInterp(void) { return interp_; }

	unsigned getNumArgs(void) const { return objc_-1; }

   int64_t operator[](unsigned i)
   {
      if( i+1 >= objc_ ) return -1;

		Tcl_WideInt ret;
      Tcl_GetWideIntFromObj(interp_, objv_[i+1], &ret);
      return int64_t(ret);
   }

   const char* getStringParm(unsigned i)
   {
      if( i+1 >= objc_ ) return NULL;
      return Tcl_GetStringFromObj(objv_[i+1], NULL);
   }
};

//----------------------------------------------------------------------------
/// reference to {optimal, range, and immunity} settings of {Grav,Temp, or Rad}
struct HabFields
{
   int *env;
   int *rng;
   int *imm;
};

//----------------------------------------------------------------------------
/// Object created for the Tcl command
/// holds the game, plus extra data to help Tcl
class NewStarsTcl
{
protected:
	std::string  rootPath_; // path to the game files
   GameData    *gameData_; // the current game
   mxml_node_t *hullTree_; // XML parsing of the game files

   Race        *wizRace_; // race currently being edited

   Ship        *newDesign_; // design currently being edited

	// Tcl will only have a single object at any point
	// we need to tell what all fleets live at the same point,
	// and if a planet (or minefield) is at any given point
   std::map<uint64_t, Planet*>                  planetMap_;
   std::map<uint64_t, std::vector<Minefield*> > mineFieldMap_;

   std::map<uint64_t , std::vector<Fleet*> > fleetsInSpace_;
   std::map<Planet*, std::vector<Fleet*> > fleetsInOrbit_;

   // should be 0 or 1 for most cases, can be any for master...
   unsigned playerIdx_; // index into gameData_->playerList

   bool fileOpen_;

protected: // methods
	/// given a planet or fleet, find all the fleets co-located
	///@return ptr to a vector from one of our fleet maps
	std::vector<Fleet*>* getFleetList(PlaceableObject *pop);

	void deleteShip(Parms &p, Fleet *fromFleet, size_t shipOff);

	/// push the game data into Tcl variables
	void setupFleets(Parms &p, const std::vector<Fleet*> &fleets, bool friendly);
	void setupMinefields(Parms &p);

	/// generate a game from a gameDef file
	void makeNewGame(const char *fname);

public:
   NewStarsTcl(void);
	/// push all our enums into Tcl variables
	void setEnums(Tcl_Interp *interp);

	/// clear all our data structures, in prep for a new game load
	void reset(void);

	/// Tcl uses "newStars cmd <optional enum|cmd|arg list>"
	/// so, cmd starts the list, but the list can contain more commands
   enum Command
   {
      Cmd_Invalid, Cmd_Component, Cmd_Design, Cmd_Generate, Cmd_Hull,
      Cmd_Message, Cmd_Open, Cmd_Orders, Cmd_Planet, Cmd_Player, Cmd_Race,
      Cmd_Save, Cmd_Universe, Cmd_Max,
      Enum_Begin = Cmd_Max,
      Enum_AddAtIndex,
      Enum_DeleteAtIndex,
      Enum_GetHab,
      Enum_GetX,
      Enum_GetY,
      Enum_GetName,
      Enum_GetNumFleets,
      Enum_GetFleetIdFromIndex,
      Enum_GetNumOrders,
      Enum_GetOrderNameFromIndex,
      Enum_GetOrderXMLfromIndex,
		Enum_GetOwnerId,
      Enum_GetPlanetId,
      Enum_GetPoints,
		Enum_GetScan,
      Enum_GetXML,
      Enum_GetYear,
      Enum_HabCenter,
      Enum_HabImmune,
      Enum_HabWidth,
      Enum_LoadPre,
      Enum_OrderColonize,
      Enum_OrderMove,
      Enum_OrderTransport,
		Enum_PlayerRelation,
		Enum_ResearchTax,
      Enum_SetName,
      Enum_SetXML,
		Enum_Speed,
      Enum_Max
   };

   typedef bool (NewStarsTcl::*AnOrder)(Parms &p);

	// every command gets its own function
   bool component(Parms &p);
   bool design   (Parms &p);
   bool generate (Parms &p);
   bool hull     (Parms &p);
   bool message  (Parms &p);
   bool open     (Parms &p);
   bool doOrders (Parms &p);
   bool planet   (Parms &p);
	bool player   (Parms &p);
   bool race     (Parms &p);
   bool save     (Parms &p);
   bool universe (Parms &p);

	// commands are placed in an array for easy access
   AnOrder orders[Cmd_Max];
};

// Tcl strings used to represent each enum
static const char *const NS_Tcl_Cmds[NewStarsTcl::Enum_Max] = {
   "",
   "::ns_component",
   "::ns_design",
   "::ns_generate",
   "::ns_hull",
   "::ns_message",
   "::ns_open",
   "::ns_orders",
   "::ns_planet",
	"::ns_player",
   "::ns_race",
   "::ns_save",
   "::ns_universe",
   "",
   "::ns_addAtIndex",
   "::ns_deleteAtIndex",
   "::ns_getHab",
   "::ns_getX",
   "::ns_getY",
   "::ns_getName",
   "::ns_getNumFleets",
   "::ns_getFleetIdFromIndex",
   "::ns_getNumOrders",
   "::ns_getOrderNameFromIndex",
   "::ns_getOrderXMLfromIndex",
	"::ns_getOwnerId",
   "::ns_getPlanetId",
   "::ns_getPoints",
	"::ns_getScan",
   "::ns_getXML",
   "::ns_getYear",
   "::ns_habCenter",
   "::ns_habImmune",
   "::ns_habWidth",
   "::ns_loadPre",
   "::ns_orderColonize",
   "::ns_orderMove",
   "::ns_orderTransport",
	"::ns_playerRelation",
	"::ns_researchTax",
   "::ns_setName",
   "::ns_setXML",
	"::ns_speed"
};

//----------------------------------------------------------------------------
/// sort of an ==, for checking when to combine entries
static
bool buildQueueEntryMatch(BuildQueueEntry *b1, BuildQueueEntry *b2)
{
   return b1->autoBuild == b2->autoBuild &&
          b1->itemType  == b2->itemType  &&
          (b2->itemType != _ship_BUILDTYPE ||
           b1->designID == b2->designID);
}

/// find ship with object id 'id' from 'desList'
static Ship* findDesignById(const std::vector<Ship*> &desList,
    const uint64_t id)
{
   std::vector<Ship*>::const_iterator k = desList.begin();
   while( k != desList.end() )
   {
      Ship *ship = *k;

      if( ship->objectId == id )
			return ship;

      ++k;
   }
   return NULL;
}

/// calculate the fuel needed for a fleet to execute all its current orders
// currently only used by pickSpeed
//Ned, refuel at starbases
static int calcPathFuel(const Fleet *f)
{
	// minimize loop overhead
	// need to repartition cargo to ships for each jump due to possibly
	// different speed and efficiency
	std::map<unsigned, CargoMoveCap> massAssignment;

	int totFuel = 0; // return value

	Location curLoc = f->coordinates; // loop carry
	//foreach order
	for(std::vector<Order*>::const_iterator i = f->currentOrders.begin();
	    i != f->currentOrders.end(); ++i)
	{
		Order *curOr = *i;
		// only inspect moves
		if( curOr->myOrderID != move_order ) continue;

		// partition cargo based on jump speed
		massAssignment.clear();
		f->assignMass(curOr->warpFactor, &massAssignment);

		// add the fuel for the next jump (could be multi-turn)
		totFuel += f->calcFuel(curOr->destination, curOr->warpFactor,
		   &massAssignment, &curLoc);

		// starting point for next move order
		curLoc = curOr->destination;
	}
	return totFuel;
}

// (used to take an index into the order list, but we need to pick a speed
//  before inserting the command)
static unsigned pickSpeed(Fleet *f, Order *op/*unsigned oidx*/, const Location &start)
{
	//Order *op = f->currentOrders[oidx];
	const Location &dest = op->destination;

	// a good first speed is just to get there in one turn
	const unsigned dist = dest.distanceFrom(start);
	unsigned spd = unsigned(sqrt(double(dist)));
	// don't risk blowing up
	spd = std::min(spd, unsigned(f->topSafeSpeed));

	// check for rounding problems (both in sqrt(dist) and in
	// travelling > spd^2 - e.g. 81.4 ly at warp 9)
	Location tmp = start;
	tmp.moveToward(dest, spd * spd);
	// if we didn't make it, and can go faster
	if( tmp != dest && spd < unsigned(f->topSafeSpeed) )
		++spd; // punch it up

	// avoid loop overhead
	// repartition cargo to ships because of speed change
	std::map<unsigned, CargoMoveCap> massAssignment;

	// available fuel is current fuel minus expected used fuel
	const unsigned availFuel = f->cargoList.cargoDetail[_fuel].amount -
		calcPathFuel(f);

	int maxFuel = 0; // scoped to while
	do
	{
		// reassign cargo for current speed
		massAssignment.clear();
		f->assignMass(spd, &massAssignment);

		// calculate the fuel to reach our destination (handles multi-turn)
		maxFuel = f->calcFuel(dest, spd, &massAssignment, &start);

		//Ned how can we reserve enough fuel to get home?

		// if we are using too much fuel
		if( maxFuel > int(availFuel) )
		{
			// try again at a lower speed
			--spd;
		}
		// while we can go slower and are using too much fuel
		// (speed 1 should consume 0 fuel [actually, generate it], but be safe)
	} while( spd > 1 && maxFuel > int(availFuel) );

	//Ned recalc speed for time (e.g. 82 ly -> warp 7, warp6)
	return spd;
}

///@param[out] hfp reference to race's {optimal, range, and immunity} based
///                on habIdx (grav,temp,rad)
///@return false on index out of range
static bool getHabFields(Race *rp, HabFields *hfp, int habIdx)
{
   switch( habIdx )
   {
   case 0:
      hfp->env = &rp->optimalEnvironment.grav;
      hfp->rng = &rp->environmentalTolerances.grav;
      hfp->imm = &rp->environmentalImmunities.grav;
      break;

   case 1:
      hfp->env = &rp->optimalEnvironment.temp;
      hfp->rng = &rp->environmentalTolerances.temp;
      hfp->imm = &rp->environmentalImmunities.temp;
      break;

   case 2:
      hfp->env = &rp->optimalEnvironment.rad;
      hfp->rng = &rp->environmentalTolerances.rad;
      hfp->imm = &rp->environmentalImmunities.rad;
      break;

   default: return false;
   }

   return true;
}

///---setup(*) helpers for open
/// push component data into Tcl variables
/// builds 3 Tcl variables
///   componentIdxList an array of buildable component ids
///   numComponents length of the above array
///   numAllComponents number of components in game (valid ids are 1..max)
static void setupComponents(const GameData *gameData, Parms &p, unsigned playerIdx)
{
   PlayerData *pdp = gameData->playerList[playerIdx];
   Race       *rp  = pdp->race;

   unsigned ct = 0;
   unsigned inum = 0;
   char arryIdx[16] = {0,};
   char buf    [16] = {0,};
   std::vector<StructuralElement*>::const_iterator i =
      gameData->componentList.begin();
   while( i != gameData->componentList.end() )
   {
      StructuralElement *sep = *i;

      if( sep->elementType < planetary_ELEMENT &&
          sep->isBuildable(rp, pdp->currentTechLevel) )
      {
         sprintf(arryIdx, "%d", ct);
         sprintf(buf, "%d", inum);
         Tcl_SetVar2(p.getInterp(), "componentIdxList", arryIdx, buf,
                     TCL_GLOBAL_ONLY);
         ct++;
      }

      i++;
      inum++;
   }
   sprintf(buf, "%d", ct);
   Tcl_SetVar(p.getInterp(), "numComponents", buf, TCL_GLOBAL_ONLY);
   sprintf(buf, "%d", inum);
   Tcl_SetVar(p.getInterp(), "numAllComponents", buf, TCL_GLOBAL_ONLY);
}

/// push design data into Tcl variables
/// (used for our designs and enemy designs)
/// sets two Tcl variables
/// mapName an array of design ids
/// ctName the length of the array
static void setupDesigns(Parms &p, const std::vector<Ship*> &desAry,
    const char *mapName, const char *ctName)
{
   char arryIdx[16] = {0,}; // sixteen digits for printf(id)

   unsigned ct = 0;
   std::vector<Ship*>::const_iterator i = desAry.begin();
   while( i != desAry.end() )
   {
      Ship *ship = *i;

      sprintf(arryIdx, "%d", ct);
		std::ostringstream buf;
		buf << ship->objectId;
		Tcl_SetVar2(p.getInterp(), mapName, arryIdx, buf.str().c_str(), TCL_GLOBAL_ONLY);

      ++i;
      ++ct;
   }
   sprintf(arryIdx, "%d", ct);
   Tcl_SetVar(p.getInterp(), ctName, arryIdx, TCL_GLOBAL_ONLY);
}

/// push fleet data into Tcl variables
/// (used for our fleets and other player fleets)
/// sets two variables (for each data set)
/// all*Map array of all fleets in the game
/// *Map array of unique objects in space
void NewStarsTcl::setupFleets(Parms &p, const std::vector<Fleet*> &fleets,
    bool friendly)
{
	const char *const tclNames[2][2] = {
		{"allFleetMap", "fleetMap"},
		{"allEnemyMap", "enemyFleetMap"},
	};
	const unsigned nameIdx = friendly ? 0 : 1;

   char arryIdx[16] = {0,};

   unsigned ct = 0;
   unsigned allCt = 0;
   std::vector<Fleet*>::const_iterator i = fleets.begin();
   while( i != fleets.end() )
   {
      Fleet *fleet = *i;
      uint64_t tmp64 = formXYhash(fleet->getX(), fleet->getY());

      sprintf(arryIdx, "%d", allCt);
		std::ostringstream buf;
		buf << fleet->objectId;
		Tcl_SetVar2(p.getInterp(), tclNames[nameIdx][0], arryIdx, buf.str().c_str(), TCL_GLOBAL_ONLY);
      allCt++;

      // check for planet with same hash
      std::map<uint64_t, Planet*>::iterator pm = planetMap_.find(tmp64);
      if( pm != planetMap_.end() )
      {
         // found a planet, 'fleet' is in orbit
         Planet *planet = pm->second;
         std::map<Planet*, std::vector<Fleet*> >::iterator orbit =
            fleetsInOrbit_.find(planet);
         if( orbit != fleetsInOrbit_.end() )
         {
            // not the first, push back on existing vector
            orbit->second.push_back(fleet);
         }
         else
         {
            // first fleet in orbit
            std::vector<Fleet*> firstFleet;
            firstFleet.push_back(fleet);
            fleetsInOrbit_.insert(std::pair<Planet*, std::vector<Fleet*> >(
               planet, firstFleet));
         }
      }
      else
      {
         // no planet, fleet is in space
         std::map<uint64_t, std::vector<Fleet*> >::iterator inSpace =
            fleetsInSpace_.find(tmp64);
         if( inSpace != fleetsInSpace_.end() )
         {
            // other fleets present
            inSpace->second.push_back(fleet);
         }
         else
         {
            // first at this location
            std::vector<Fleet*> firstFleet;
            firstFleet.push_back(fleet);
            fleetsInSpace_.insert(std::pair<uint64_t, std::vector<Fleet*> >(
               tmp64, firstFleet));

            sprintf(arryIdx, "%d", ct);
				Tcl_SetVar2(p.getInterp(), tclNames[nameIdx][1], arryIdx, buf.str().c_str(),
				    TCL_GLOBAL_ONLY);

            ct++;
         }
      }

      i++;
   }
}

/// push hull data into Tcl variables
static void setupHulls(const GameData *gameData, Parms &p, unsigned playerIdx)
{
   PlayerData *pdp = gameData->playerList[playerIdx];

	// avoid loop overhead
   char arryIdx[16] = {0,};
   char buf    [16] = {0,};

	// loop carry
	unsigned ct = 0;
   unsigned inum = 0;
   std::vector<Ship*>::const_iterator i =
      gameData->hullList.begin();
   while( i != gameData->hullList.end() )
   {
      Ship *sp = *i;

      if( sp->isBuildable(pdp) )
      {
         sprintf(arryIdx, "%d", ct);
         sprintf(buf, "%d", inum);
         Tcl_SetVar2(p.getInterp(), "hullMap", arryIdx, buf,
                     TCL_GLOBAL_ONLY);
         ct++;
      }

      i++;
      inum++;
   }
   sprintf(buf, "%d", ct);
   Tcl_SetVar(p.getInterp(), "numHulls", buf, TCL_GLOBAL_ONLY);
}

/// push minefield data into Tcl variables
void NewStarsTcl::setupMinefields(Parms &p)
{
   char arryIdx[16] = {0,};
   unsigned ct = 0;
   std::vector<Minefield*>::const_iterator i = gameData_->minefieldList.begin();
   while( i != gameData_->minefieldList.end() )
   {
      Minefield *mfp = *i;
      uint64_t tmp64 = formXYhash(mfp->getX(), mfp->getY());

      // check for other minefields
      std::map<uint64_t, std::vector<Minefield*> >::iterator coMf =
               mineFieldMap_.find(tmp64);
      if( coMf == mineFieldMap_.end() )
      {
         // first at point
         std::vector<Minefield*> firstMf;
         firstMf.push_back(mfp);
         mineFieldMap_.insert(std::pair<uint64_t,
                  std::vector<Minefield*> >(tmp64, firstMf));
      }
      else
      {
         // in place with others
         coMf->second.push_back(mfp);
      }

      sprintf(arryIdx, "%d", ct);
		std::ostringstream buf;
		buf << mfp->objectId;
		Tcl_SetVar2(p.getInterp(), "mineFieldList", arryIdx, buf.str().c_str(),
                  TCL_GLOBAL_ONLY);
      ct++;

      i++;
   }

   sprintf(arryIdx, "%d", ct);
   Tcl_SetVar(p.getInterp(), "numMinefields", arryIdx, TCL_GLOBAL_ONLY);
}

/// push packet data into Tcl variables
/// sets two Tcl variables
///   packetList array of packet ids
///   numPackets length of the array
static void setupPackets(const GameData *gameData, Parms &p)
{
   char arryIdx[16] = {0,};

   unsigned ct = 0;
   std::vector<Fleet*>::const_iterator i = gameData->packetList.begin();
   while( i != gameData->packetList.end() )
   {
      Fleet *packet = *i;

      sprintf(arryIdx, "%d", ct);
		std::ostringstream buf;
		buf << packet->objectId;
		Tcl_SetVar2(p.getInterp(), "packetList", arryIdx, buf.str().c_str(), TCL_GLOBAL_ONLY);

      i++;
      ct++;
   }
   sprintf(arryIdx, "%d", ct);
   Tcl_SetVar(p.getInterp(), "numPackets", arryIdx, TCL_GLOBAL_ONLY);
}

/// push planet data into Tcl variables
static void setupPlanets(const GameData *gameData, Parms &p,
                         std::map<uint64_t, Planet*> *planetMap)
{
   char arryIdx[16] = {0,};

   unsigned ct = 0;
   std::vector<Planet*>::const_iterator i = gameData->planetList.begin();
   while( i != gameData->planetList.end() )
   {
      Planet *planet = *i;

      uint64_t tmp64 = formXYhash(planet->getX(), planet->getY());

      planetMap->insert(std::pair<uint64_t, Planet*>(tmp64, planet));

      sprintf(arryIdx, "%d", ct);
		std::ostringstream buf;
		buf << planet->objectId;
		Tcl_SetVar2(p.getInterp(), "planetMap", arryIdx, buf.str().c_str(), TCL_GLOBAL_ONLY);

      i++;
      ct++;
   }
   sprintf(arryIdx, "%d", ct);
   Tcl_SetVar(p.getInterp(), "numPlanets", arryIdx, TCL_GLOBAL_ONLY);
}

//----------------------------------------------------------------------------
NewStarsTcl::NewStarsTcl(void)
{
	char *tmpPath = getcwd(NULL, 0);
	rootPath_ = tmpPath;
	free(tmpPath);

   initRaceWizard();

   fileOpen_  = false;
   playerIdx_ = unsigned(-1); // invalid

   gameData_ = NULL;

   wizRace_   = NULL;
   newDesign_ = NULL;

   orders[Cmd_Invalid  ] = NULL;
   orders[Cmd_Component] = &NewStarsTcl::component;
   orders[Cmd_Design   ] = &NewStarsTcl::design;
   orders[Cmd_Generate ] = &NewStarsTcl::generate;
   orders[Cmd_Hull     ] = &NewStarsTcl::hull;
   orders[Cmd_Message  ] = &NewStarsTcl::message;
   orders[Cmd_Open     ] = &NewStarsTcl::open;
   orders[Cmd_Orders   ] = &NewStarsTcl::doOrders;
   orders[Cmd_Planet   ] = &NewStarsTcl::planet;
   orders[Cmd_Player   ] = &NewStarsTcl::player;
   orders[Cmd_Race     ] = &NewStarsTcl::race;
   orders[Cmd_Save     ] = &NewStarsTcl::save;
   orders[Cmd_Universe ] = &NewStarsTcl::universe;
}

void NewStarsTcl::reset(void)
{
   planetMap_.clear();
   mineFieldMap_.clear();
   fleetsInSpace_.clear();
   fleetsInOrbit_.clear();
}

/// command dealing with a player
/// accepted commands:
///   ns_getName        <id>
///     returns string with player[id] name
///   ns_playerRelation <id> <0:get/1:set> [val]
///     returns the value (after set)
///   ns_researchTax    [setVal]
///     returns current value (after set)
bool NewStarsTcl::player(Parms &p)
{
   PlayerData *pdp = gameData_->playerList[playerIdx_];

	Command c = Command(p[1]);
   const unsigned id = unsigned(p[2]);

	if( c == Enum_ResearchTax )
	{
		// only set if it is a valid value
		if( id != unsigned(-1) )
		{
			pdp->researchTechHoldback = id;
		}

		Tcl_Obj *res = Tcl_NewIntObj(pdp->researchTechHoldback);
		Tcl_SetObjResult(p.getInterp(), res);
		return true;
	}

	if( c == Enum_PlayerRelation )
	{
		const bool setVal = p[3] != 0;
		const PlayerRelation val = PlayerRelation(p[4]);

		PlayerRelation rel = PR_NEUTRAL;

		if( setVal )
		{
			assert( val != -1 );
			pdp->playerRelationMap_[id] = val;
			rel = val;
		}
		else // get val
		{
			// don't insert into the map
			std::map<unsigned, PlayerRelation>::const_iterator it = pdp->playerRelationMap_.find(id);
			if( it != pdp->playerRelationMap_.end() )
				rel = it->second;
		}

		Tcl_Obj *res = Tcl_NewIntObj(rel);
		Tcl_SetObjResult(p.getInterp(), res);
		return true;
	}
	assert( c == Enum_GetName );

	// if we want our name
	if( id == pdp->race->ownerID )
	{
		Tcl_Obj *res = Tcl_NewStringObj(pdp->race->pluralName.c_str(),
                                      strlen(pdp->race->pluralName.c_str()));
      Tcl_SetObjResult(p.getInterp(), res);
		return true;
	}
	//else, must be some enemy we have seen

	for(std::vector<EnemyPlayer*>::const_iterator i = pdp->knownEnemyPlayers.begin();
		i != pdp->knownEnemyPlayers.end(); ++i)
	{
		if( (**i).ownerID == id )
		{
			Tcl_Obj *res = Tcl_NewStringObj((**i).pluralName.c_str(),
	                                      strlen((**i).pluralName.c_str()));
			Tcl_SetObjResult(p.getInterp(), res);
			return true;
		}
	}
	return true;
}

/// command for handling game messages
/// accepted commands:
///   <id> ns_getXML
bool NewStarsTcl::message(Parms &p)
{
	// check id
   unsigned i = unsigned(p[1]);
   if( i > gameData_->playerMessageList.size() )
      return false;

	// get the message
   PlayerMessage *pmp = gameData_->playerMessageList[i];

	// convert it to XML
	assert( pmp && Command(p[2]) == Enum_GetXML );
   std::string xmlStr;
   pmp->toXML(xmlStr);

	// send it to Tcl
	const char *strRes = xmlStr.c_str();
   Tcl_Obj *res = Tcl_NewStringObj(strRes, strlen(strRes));
   Tcl_SetObjResult(p.getInterp(), res);

   return true;
}

/// helper for generate new game
///@param[in] fname name of the gameDef file
void NewStarsTcl::makeNewGame(const char *fname)
{
	mxml_node_t *tree = getTree(fname);
	UnivParms up;
	up.parseXMLtree(tree);

	gameData_->createFromParms(rootPath_.c_str(), up);

	// do initial pen scans
	ActionDispatcher ad(gameData_);
	ad.calcScan();

   unsigned newLen = std::string(fname).find_last_of("/\\");
   char *rootPath = new char[newLen+1];
   strncpy(rootPath, fname, newLen);
   rootPath[newLen] = 0;

	generateFiles(*gameData_, rootPath);
}

/// command to generate new stuff
/// accepted commands:
/// <master filename>
///   generate the next turn using the master
/// <gamedef filename> 1
///   generate a new game from the gamedef
bool NewStarsTcl::generate(Parms &p)
{
	// check for valid file
   const char *fname = p.getStringParm(1);
   if( !fname )
   {
      const char *strRes = "Bad file name";
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return false;
   }

	// clear our current game state
   if( gameData_ )
   {
      reset();
      delete gameData_;
   }
   gameData_ = new GameData;

	// check for generate new game
	if( p[2] != -1 )
	{
		makeNewGame(fname);
		return true; // done
	}
	//else, generate next turn

	mxml_node_t *tree = getTree(fname);
	if( !gameData_->parseTrees(tree, true, rootPath_) )
   {
      const char *strRes = "Parse error in turn file";
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return false;
   }

   gameData_->playerMessageList.clear();
   ActionDispatcher aDispatchEngine(gameData_);

	aDispatchEngine.dispatchAll();

   std::string gameString;
   gameData_->theUniverse->masterCopy = true;
   gameData_->dumpXMLToString(gameString);
   std::string fName = gameData_->theUniverse->playerFileBaseName;

   fName += "_master.xml";
   std::ofstream fileOut;
   fileOut.open(fName.c_str(),ios_base::out | ios_base::trunc);
   if( !fileOut.good() )
   {
      //cerr << "Error when opening file: " << fName << " for output\n";
      return false;
   }
   fileOut << gameString;
   fileOut.close();

   for(unsigned i = 0; i < gameData_->playerList.size(); i++)
      gameData_->dumpPlayerFile(gameData_->playerList[i]);

   return true;
}

/// command dealing with ship designs
/// (uses the newDesign_ field)
/// accepted commands:
/// ns_loadPre <0/1/2> <id>
///   based on existing design (self or enemy) or hull given by id
/// ns_loadPre scout <name> [armed]
///   based on what we think would be a good design for a scout
/// ns_addAtIndex <idx> <cmpId>
///   add component to slot[idx]
/// ns_deleteAtIndex <idx>
///   delete the design at idx
/// ns_deleteAtIndex <ignored> <slot>
///   remove one component from slot, returns component id
/// ns_save
///   commit the current design, returns error (empty on success)
/// ns_getXML ns_getName ns_setName
bool NewStarsTcl::design(Parms &p)
{
   Command c = Command(p[1]);
   PlayerData *pdp = gameData_->playerList[playerIdx_];

   // start a new design, based on something existing
   if( c == Enum_LoadPre )
   {
      delete newDesign_; // delete anything left over
      const unsigned playerNum = pdp->race->ownerID;

		const char *p2arg = p.getStringParm(2);
		if( !strcasecmp("scout", p2arg) )
		{
			newDesign_ = makeScoutDesign(p[4] == 1,
			    pdp, p.getStringParm(3));

			return true;
		}

      Ship *orig;
      unsigned where = unsigned(p[2]);
      uint64_t which = p[3];
      switch( where )
      {
      case 0: // one of my existing designs
      {
         orig = findDesignById(gameData_->playerDesigns, which);
         break;
      }

      case 1: // an empty hull
         orig = gameData_->hullList[size_t(which)];
         break;

      case 2: // an enemy design
         orig = findDesignById(pdp->knownEnemyDesignList, which);
         break;

      default: assert(false);
      }

      newDesign_ = new Ship(*orig);
      newDesign_->setName(newDesign_->getName() + " (2)");

      newDesign_->ownerID = playerNum;

      newDesign_->objectId = IdGen::getInstance()->makeDesign(pdp->race->ownerID, pdp->objectMap);
      newDesign_->designhullID = newDesign_->objectId;

		newDesign_->updateCostAndMass(*gameData_);
      return true;
   }
   //else
   if( c == Cmd_Save ) // commit the design to my list
   {
		if( !newDesign_->isBuildable(pdp) )
		{
			const char* const errStr = "Insufficient tech";
	      Tcl_Obj *res = Tcl_NewStringObj(errStr, strlen(errStr));
		   Tcl_SetObjResult(p.getInterp(), res);
			return true;
		}

		if( !newDesign_->isSpaceStation &&
		     newDesign_->numEngines == 0 )
		{
			const char* const errStr = "No engine";
	      Tcl_Obj *res = Tcl_NewStringObj(errStr, strlen(errStr));
		   Tcl_SetObjResult(p.getInterp(), res);
			return true;
		}

		gameData_->playerDesigns.push_back(newDesign_);

      pdp->designList = gameData_->playerDesigns;
      pdp->objectMap.insert(std::make_pair(newDesign_->objectId, newDesign_));

      setupDesigns(p, gameData_->playerDesigns, "designMap", "numDesigns");

      Tcl_Obj *res = Tcl_NewStringObj("", 0);
	   Tcl_SetObjResult(p.getInterp(), res);

		newDesign_ = nullptr;
		return true;
   }
   //else
   if( c == Enum_AddAtIndex ) // add a component to a slot
   {
      unsigned slotOff = unsigned(p[2]);
      unsigned cmpId   = unsigned(p[3]);
      assert( slotOff < newDesign_->slotList.size() );
      assert( cmpId   < gameData_->componentList.size() );

      ShipSlot *ssp = newDesign_->slotList[slotOff];

      if( ssp->numberPopulated )
      {
         // adding to existing installation
         if( ssp->structureID == cmpId && // same item
             ssp->numberPopulated < ssp->maxItems ) // not full
         {
            ssp->numberPopulated++;
         }
      }
      else // adding to empty slot
      {
         // check acceptability
			StructuralElement *sep = gameData_->componentList[cmpId];
			if( ssp->acceptableElements[sep->elementType] )
			{
				// ok
            ssp->structureID     = cmpId;
            ssp->numberPopulated = 1;
			}
			else
			{
				const char* const errStr = "Invalid Slot Type";
		      Tcl_Obj *res = Tcl_NewStringObj(errStr, strlen(errStr));
			   Tcl_SetObjResult(p.getInterp(), res);
				return true;
			}
      }

		newDesign_->updateCostAndMass(*gameData_);
      return true;
   }
   //else
   if( c == Enum_SetName )
   {
      const char *newName = p.getStringParm(2);
      newDesign_->setName(std::string(newName));
      return true;
   }
   //else
   if( c == Enum_GetXML )
   {
      std::string xmlStr;
      newDesign_->toXMLString(xmlStr);

      const char *strRes = xmlStr.c_str();
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
   if( c == Enum_GetName )
   {
      const char *strRes = newDesign_->getName().c_str();
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
   assert( c == Enum_DeleteAtIndex );
   unsigned slotOff = unsigned(p[3]);
   if( slotOff == unsigned(-1) )
   {
      // delete design
      //Ned
      return true;
   }
   //else
   // delete slot
   assert( slotOff < newDesign_->slotList.size() );
   ShipSlot *ssp = newDesign_->slotList[slotOff];

   if( ssp->numberPopulated )
      ssp->numberPopulated--;

	newDesign_->updateCostAndMass(*gameData_);

	Tcl_Obj *res = Tcl_NewIntObj(ssp->structureID);
   Tcl_SetObjResult(p.getInterp(), res);
	return true;
}

/// command dealing with game defined hulls
/// accepted commands:
/// ns_getXML ns_getName
bool NewStarsTcl::hull(Parms &p)
{
	// check the idx
   unsigned i = unsigned(p[1]);
   if( i > gameData_->hullList.size() )
      return false;

   Ship *sp = gameData_->hullList[i];
   assert( sp );

   std::string outStr;

   Command c = Command(p[2]);
   if( c == Enum_GetName )
   {
      outStr = sp->getName();
   }
   else
   {
      assert( c == Enum_GetXML );
      sp->toXMLString(outStr);
   }

   const char *strRes = outStr.c_str();
   Tcl_Obj *res = Tcl_NewStringObj(strRes, strlen(strRes));
   Tcl_SetObjResult(p.getInterp(), res);
   return true;
}

/// command dealing with game defined components
/// accepted commands:
/// ns_getXML ns_getName
bool NewStarsTcl::component(Parms &p)
{
	// check idx
   unsigned i = unsigned(p[1]);
   if( i > gameData_->componentList.size() )
      return false;

   StructuralElement *sep = gameData_->componentList[i];
   assert( sep );

   Command c = Command(p[2]);
   if( c == Enum_GetName )
   {
      const char *strRes = sep->name.c_str();
      Tcl_Obj *res = Tcl_NewStringObj(strRes, strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }

   assert( c == Enum_GetXML );
   std::string xmlStr;
   sep->toXMLString(xmlStr);
   const char *strRes = xmlStr.c_str();
   Tcl_Obj *res = Tcl_NewStringObj(strRes, strlen(strRes));
   Tcl_SetObjResult(p.getInterp(), res);

   return true;
}

/// command dealing with race (mostly the race wizard)
/// accepted commands:
/// ns_loadPre <idx>
///   load one of the pre-defined races (e.g. Humanoids)
/// ns_getPoints
///   return number of points the race costs
///
/// ns_habCenter <habType> <amount>
/// ns_habWidth  <habType> <amount>
/// ns_habImmune <habType> <bool>
///   adjust the hab range
///
/// ns_open <filename>
///   load the race given by filename into the wizard
/// ns_getXML ns_setXML
bool NewStarsTcl::race(Parms &p)
{
   Command c = Command(p[1]);

	// race wizard specific stuff
   if( c == Enum_LoadPre )
   {
      delete wizRace_;

      wizRace_ = raceFactory(unsigned(p[2]));

      return true;
   }
   if( c == Cmd_Open )
   {
      delete wizRace_;

      mxml_node_t *tree = getTree(p.getStringParm(2));
      wizRace_ = new Race;
      wizRace_->initRace(tree);
      return true;
   }
   //else

	// figure out if we are in the race wizard or not
	Race *rp = wizRace_;
   if( !rp ) rp = gameData_->playerList[playerIdx_]->race;
   if( !rp ) return false;

	if( c == Enum_GetName )
	{
		const char *strRes = rp->pluralName.c_str();
		Tcl_Obj *res = Tcl_NewStringObj(strRes,
		                                strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
	}
	//else
   if( c == Enum_GetPoints )
   {
      long pts = calculateAdvantagePointsFromRace(rp);
      Tcl_Obj *res = Tcl_NewLongObj(pts);

      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
   if( c == Enum_HabCenter )
   {
      int which = int(p[2]);
      int amt   = int(p[3]);

      HabFields hf;

      if( !getHabFields(rp, &hf, which) ) return false;

      if( (*hf.env - *hf.rng <= 0 && amt < 0)   ||
          (*hf.env + *hf.rng >= 100 && amt > 0) )
      {
         Tcl_Obj *res = Tcl_NewLongObj(*hf.env);
         Tcl_SetObjResult(p.getInterp(), res);

         return true;
      }

      *hf.env += amt;

      Tcl_Obj *res = Tcl_NewLongObj(*hf.env);
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
   if( c == Enum_HabWidth )
   {
      int which = int(p[2]);
      int amt   = int(p[3]);

      HabFields hf;

      if( !getHabFields(rp, &hf, which) ) return false;

      if( *hf.env == 50 && *hf.rng == 50 && amt > 0 )
      {
         Tcl_Obj *res = Tcl_NewLongObj(*hf.rng);
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
      }

      // if the range would be stretched below 0, move the center up
      if( (*hf.env - *hf.rng <= 0) && amt > 0 )
      {
         *hf.env += *hf.env - *hf.rng + amt;
      }

      if( (*hf.env + *hf.rng >= 100) && amt > 0 )
      {
         *hf.env -= *hf.env + *hf.rng + amt - 100;
      }

      // check for underflow
      if( amt < 0 && *hf.rng == 1 )
      {
         Tcl_Obj *res = Tcl_NewLongObj(*hf.rng);
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
      }

      *hf.rng += amt;

      Tcl_Obj *res = Tcl_NewLongObj(*hf.rng);
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
   if( c == Enum_HabImmune )
   {
      int which = int(p[2]);
      int amt   = int(p[3]);

      HabFields hf;

      if( !getHabFields(rp, &hf, which) ) return false;

      assert( amt == 0 || amt == 1 );
      *hf.imm = amt;

      Tcl_Obj *res = Tcl_NewLongObj(*hf.imm);
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
   if( c == Enum_SetXML )
   {
      string newXML(p.getStringParm(2));
      newXML = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n" + newXML;
      mxml_node_t *tree = mxmlLoadString(NULL, newXML.c_str(), MXML_NO_CALLBACK);
      rp->initRace(tree);
      mxmlDelete(tree);
      return true;
   }
   //else
   assert( c == Enum_GetXML );

   std::string xmlStr;
   rp->toXMLString(xmlStr);

   const char *strRes = xmlStr.c_str();
   Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                   strlen(strRes));
   Tcl_SetObjResult(p.getInterp(), res);

   return true;
}

/// ns_save <filename>
bool NewStarsTcl::save(Parms &p)
{
   const char *strRes;
   const char *fname = p.getStringParm(1);
   if( !fname )
   {
      strRes = "Bad file name";
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return false;
   }
   if( !gameData_ )
      return false;

	// need to pull the next research field back into C++
	Tcl_Obj *nrop = Tcl_GetVar2Ex(p.getInterp(), "curResearch", NULL, TCL_GLOBAL_ONLY);
	int tmpI;
	Tcl_GetIntFromObj(p.getInterp(), nrop, &tmpI);

	// got it!
	if( tmpI < numberOf_TECH )
	{
		gameData_->playerList[playerIdx_]->researchTechID = tmpI;
	}

	// set research values accordingly
	for(int i = 0; i < numberOf_TECH; ++i)
		if( i == tmpI )
			gameData_->playerList[playerIdx_]->researchAllocation.techLevels[i] = 100;
		else
			gameData_->playerList[playerIdx_]->researchAllocation.techLevels[i] = 0;

	nrop = Tcl_GetVar2Ex(p.getInterp(), "nextResearch", NULL, TCL_GLOBAL_ONLY);
	Tcl_GetIntFromObj(p.getInterp(), nrop, &tmpI);
	if( tmpI < numberOf_TECH )
	{
		gameData_->playerList[playerIdx_]->researchNextID = tmpI;
		gameData_->playerList[playerIdx_]->researchNextSame   = false;
		gameData_->playerList[playerIdx_]->researchNextLowest = false;
	}
	else
	{
		tmpI -= numberOf_TECH;
		if( tmpI == 0 )
		{
			gameData_->playerList[playerIdx_]->researchNextSame = true;
		}
		else
		{
			--tmpI;
			if( tmpI == 0 )
			{
				gameData_->playerList[playerIdx_]->researchNextLowest = true;
			}
		}
	}

	// make sure the player copy of planets matches the global copy
   gameData_->playerList[playerIdx_]->syncPlanetsFromGame(gameData_->planetList);

	// construct the turn file XML
   std::string strBuf;

	// write it out
	FILE *ofile = fopen(fname, "w");
	strBuf = "<?xml version=\"1.0\"?>\n";
   fwrite(strBuf.c_str(), 1, strBuf.length(), ofile);

   gameData_->dumpXMLToString(strBuf);
   fwrite(strBuf.c_str(), 1, strBuf.length(), ofile);

   fflush(ofile);
   fclose(ofile);
   return true;
}

// ns_open <filename>
bool NewStarsTcl::open(Parms &p)
{
   const char *fname = p.getStringParm(1);
   if( !fname )
   {
      const char *strRes = "Bad file name";
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return false;
   }

   if( gameData_ )
   {
      reset();
      delete gameData_;
   }
   gameData_ = new GameData;

   mxml_node_t *tree = getTree(fname);
   if( !gameData_->parseTrees(tree, true, ".") )
   {
      const char *strRes = "Parse error in turn file";
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return false;
   }

   fileOpen_ = true;

   // assume first valid player, for now
   if( gameData_->playerList.size() > 1 ) playerIdx_ = 1;
   else                                   playerIdx_ = 0;

   // inform TCL of components buildable by this player
   setupComponents(gameData_, p, playerIdx_);

   setupHulls(gameData_, p, playerIdx_);

   // set up planetMap and TCL
   setupPlanets(gameData_, p, &planetMap_);

   // set up currently defined player designs for TCL
   setupDesigns(p, gameData_->playerDesigns, "designMap", "numDesigns");
	setupDesigns(p, gameData_->playerList[playerIdx_]->knownEnemyDesignList, "enemyDesMap", "numEnemyDesigns");

   // set up fleets, some in orbit of planets, some in space - arrays and TCL
   setupFleets(p, gameData_->fleetList, true); // lots of internal data
	setupFleets(p, gameData_->playerList[playerIdx_]->detectedEnemyFleetList, false);
   setupMinefields(p); // continue the chain
   setupPackets(gameData_, p);

   char buf[16];

   sprintf(buf, "%d", gameData_->theUniverse->numPlayers);
   Tcl_SetVar(p.getInterp(), "numPlayers", buf, TCL_GLOBAL_ONLY);

   sprintf(buf, "%zd", gameData_->playerMessageList.size());
   Tcl_SetVar(p.getInterp(), "numMessages", buf, TCL_GLOBAL_ONLY);

	sprintf(buf, "%d", gameData_->playerList[playerIdx_]->researchTechID);
	Tcl_SetVar(p.getInterp(), "curResearch", buf, TCL_GLOBAL_ONLY);

	sprintf(buf, "%d", gameData_->playerList[playerIdx_]->researchNextID);
	Tcl_SetVar(p.getInterp(), "nextResearch", buf, TCL_GLOBAL_ONLY);

	char idxBuf[4] = "0";
	for(unsigned i = 0; i < numberOf_TECH; ++i)
	{
		sprintf(buf, "%d", gameData_->playerList[playerIdx_]->currentTechLevel.techLevels[i]);
		Tcl_SetVar2(p.getInterp(), "currentTech", idxBuf, buf, TCL_GLOBAL_ONLY);

		++idxBuf[0];
	}

   delete wizRace_;
	wizRace_ = NULL;

   // report success
   Tcl_Obj *res = Tcl_NewLongObj(gameData_->playerList[playerIdx_]->race->ownerID);
   Tcl_SetObjResult(p.getInterp(), res);
   return true;
}

std::vector<Fleet*>* NewStarsTcl::getFleetList(PlaceableObject *pop)
{
   uint64_t tmp64 = formXYhash(pop->getX(), pop->getY());
   std::vector<Fleet*> *fleetList = NULL;

   // see if we are at a planet
	std::map<uint64_t, Planet*>::iterator planetIt = planetMap_.find(tmp64);
   if( planetIt != planetMap_.end() )
   {
      Planet *planet = planetIt->second;
      std::map<Planet*, std::vector<Fleet*> >::iterator orbit =
         fleetsInOrbit_.find(planet);
      if( orbit != fleetsInOrbit_.end() )
      {
         fleetList = &orbit->second;
      }
      //else no fleets here, leave fleetList NULL
   }
   else // fleet in space
   {
      std::map<uint64_t, std::vector<Fleet*> >::iterator inSpace =
         fleetsInSpace_.find(tmp64);
      if( inSpace != fleetsInSpace_.end() )
         fleetList = &inSpace->second;
   }

   return fleetList;
}

/// command dealing with fleet orders
/// accepted commands:
///   ns_orderTransport <itemidx> <newval>
///   ns_getX ns_getY
/// (most reads  are done with ns_getOrderXMLfromIndex)
/// (most writes are done with add new order)
bool NewStarsTcl::doOrders(Parms &p)
{
   if( !fileOpen_ ) return false;
   if( !gameData_->hasPlanets() ) return false;

   uint64_t planetId = p[1];
   PlaceableObject *pop = gameData_->getTargetObject(planetId);
   if( !pop ) return false;

   unsigned orderIdx = unsigned(p[2]);
   if( orderIdx >= pop->currentOrders.size() ) return false;
   Order *op = pop->currentOrders[orderIdx];

   Command c = Command(p[3]);
	if( c == Enum_GetX )
	{
		Tcl_Obj *res = Tcl_NewLongObj(op->destination.getX());
		Tcl_SetObjResult(p.getInterp(), res);
      return true;
	}

	if( c == Enum_GetY )
	{
		Tcl_Obj *res = Tcl_NewLongObj(op->destination.getY());
		Tcl_SetObjResult(p.getInterp(), res);
      return true;
	}

   if( c != Enum_OrderTransport ) return false;

   CargoManifest &ctdr = op->cargoTransferDetails;

   unsigned itemIdx = unsigned(p[4]);
   if( itemIdx >= _numCargoTypes ) return false;

   unsigned newVal = unsigned(p[5]);
   ctdr.cargoDetail[itemIdx].amount = newVal;

   return true;
}

static
void handleFleetXfer(Fleet *toFleet, Fleet *fromFleet, Ship *tgt, uint64_t ct)
{
	if( fromFleet->fuelCapacity == fromFleet->cargoList.cargoDetail[_fuel].amount )
	{
		// easy, fuel is max
		uint64_t f = tgt->fuelCapacity * ct;
		fromFleet->cargoList.cargoDetail[_fuel].amount -= f;
		  toFleet->cargoList.cargoDetail[_fuel].amount += f;
	}
	else // need to extract a portion of fuel
	{
	}

	// check for cargo

	// if this ship cannot hold any, or the fleet has none
	if( tgt->cargoCapacity == 0 || fromFleet->cargoList.getMass() == 0 )
		return; // none, done
}

void NewStarsTcl::deleteShip(Parms &p, Fleet *fromFleet, size_t shipOff)
{
	PlayerData *self = gameData_->playerList[playerIdx_];

	// all gone
	vectorDelete(fromFleet->shipRoster, shipOff);

	// handle last ship move
	if( fromFleet->shipRoster.size() != 0 )
	{
		fromFleet->updateFleetVariables(*gameData_);
		return;
	}

	// remove from gameData list
	std::vector<Fleet*>::iterator i = std::find(
	    gameData_->fleetList.begin(), gameData_->fleetList.end(), fromFleet);
	gameData_->fleetList.erase(i);

	// remove from Player data
	i = std::find(self->fleetList.begin(), self->fleetList.end(), fromFleet);
	assert( i != self->fleetList.end() );
	self->fleetList.erase(i);

	// remove from Player object map
	self->objectMap.erase(fromFleet->objectId);

	delete fromFleet;

	// update Tcl
	fleetsInSpace_.clear();
	fleetsInOrbit_.clear();
	Tcl_UnsetVar2(p.getInterp(), "allFleetMap", NULL, TCL_GLOBAL_ONLY);
	Tcl_UnsetVar2(p.getInterp(), "fleetMap"   , NULL, TCL_GLOBAL_ONLY);
	setupFleets(p, gameData_->fleetList, true);
}

// planet id [getX | getY | getName | getNumFleets | getNumOrders]
// planet id [getFleetIdFromIndex | getOrderNameFromIndex] index
bool NewStarsTcl::planet(Parms &p)
{
   if( !fileOpen_ ) return false;
   if( !gameData_->hasPlanets() ) return false;
	PlayerData *self = gameData_->playerList[playerIdx_];

   // also works for ships and such...
   uint64_t planetId = p[1];
   Command c = Command(p[2]);

   // look through our planets and fleets
   PlaceableObject *pop = gameData_->getTargetObject(planetId);

	if( !pop )
	{
		// check enemy fleets
		for(size_t eid = 0; !pop &&
		    eid < self->detectedEnemyFleetList.size();
		    ++eid)
		{
			Fleet *fleet = self->detectedEnemyFleetList[eid];
			if( fleet->objectId == planetId )
				pop = fleet;
		}
	}

   if( !pop )
   {
		// must be a design (ours or an enemy)
      Ship *design = findDesignById(gameData_->playerDesigns, planetId);
		if( !design )
			design = findDesignById(self->knownEnemyDesignList, planetId);
      if( !design )
		{
			const char *const strRes = "Error bad planetId";
			Tcl_Obj *res = Tcl_NewStringObj(strRes, strlen(strRes));
			Tcl_SetObjResult(p.getInterp(), res);
			return false;
		}

		if( c == Enum_GetOwnerId )
		{
			Tcl_Obj *res = Tcl_NewLongObj(design->ownerID);
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
		}
      if( c == Enum_GetXML )
      {
         std::string xmlStr;
         design->toXMLString(xmlStr);

         const char *strRes = xmlStr.c_str();
         Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                         strlen(strRes));
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
      }
      //else
      assert( c == Enum_GetName );

		std::string strTmp = design->getName();
		if( strTmp.size() == 0 )
		{
			EnemyPlayer* epp = self->findEnemyPlayer(design->ownerID);
			if( epp )
			{
				std::ostringstream os;
				os << epp->singularName << ' ';
				os << gameData_->hullList[design->baseHullID-1]->getName();
				strTmp = os.str();
			}
			else
				strTmp = "Enemy design";
		}
		Tcl_Obj *res = Tcl_NewStringObj(strTmp.c_str(),
			                             strTmp.size());
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   } // design handler

   assert( pop );

	if( c == Enum_GetOwnerId )
	{
		Tcl_Obj *res = Tcl_NewLongObj(pop->ownerID);
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
	}
   if( c == Enum_GetName )
   {
		std::string tmpName = pop->getName();
		if( tmpName[0] == 0 )
		{
			// should be a fleet...
			EnemyPlayer* epp = self->findEnemyPlayer(pop->ownerID);
			if( epp )
			{
				std::ostringstream os;
				os << epp->singularName << " fleet #";
				os << pop->objectId;
				tmpName = os.str();
			}
			else
				tmpName = "Enemy placeable";
		}
		Tcl_Obj *res = Tcl_NewStringObj(tmpName.c_str(),
			                             tmpName.size());
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }

   if( c == Enum_GetXML )
   {
      std::string xmlStr;
      pop->toXMLString(xmlStr);

      const char *strRes = xmlStr.c_str();
      Tcl_Obj *res = Tcl_NewStringObj(strRes,
                                      strlen(strRes));
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
   if( c == Enum_GetHab )
   {
      Planet *planet = dynamic_cast<Planet*>(pop);
      if( !planet ) return false;

		Race *race = self->race;
		Environment env = planet->currentEnvironment;
		if( p[3] == 1 )
		{
			env = race->getFinalTerraformEnvironment(planet);
		}
		long hab = planetValueCalcRace(env, race);
      Tcl_Obj *res = Tcl_NewLongObj(hab);
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
   //else
	if( c == Cmd_Design )
	{
      Fleet *fleet = dynamic_cast<Fleet*>(pop);
      if( !fleet ) return false;

		if( p.getNumArgs() == 3 )
		{
			// get num ship types in fleet
			Tcl_Obj *res = Tcl_NewLongObj(fleet->shipRoster.size());
			Tcl_SetObjResult(p.getInterp(), res);
			return true;
		}

		size_t shipIdx = size_t(p[3]);
		if( shipIdx >= fleet->shipRoster.size() ) return false;

		Ship *s = fleet->shipRoster[shipIdx];
		if( !s ) return false;

		Command subcmd = Command(p[4]);
		if( subcmd == Enum_GetName )
		{
			Tcl_Obj *res = Tcl_NewStringObj(s->getName().c_str(),
				                             s->getName().size());
			Tcl_SetObjResult(p.getInterp(), res);
		}
		else if( subcmd == Enum_GetNumFleets )
		{
			Tcl_Obj *res = Tcl_NewLongObj(s->quantity);
			Tcl_SetObjResult(p.getInterp(), res);
		}
		return true;
	}
   //else
   if( c == Enum_GetNumFleets || c == Enum_GetFleetIdFromIndex )
   {
      Tcl_Obj *res;

      std::vector<Fleet*> *fleetList = getFleetList(pop);
      if( !fleetList )
      {
         if( c != Enum_GetNumFleets )
         {
				std::ostringstream os;
				os << "No fleets found at id: " << planetId << ", named "
               << pop->getName();
            const char *strRes = os.str().c_str();
            res = Tcl_NewStringObj(strRes, strlen(strRes));
            Tcl_SetObjResult(p.getInterp(), res);
            return false;
         }

         res = Tcl_NewLongObj(0);
      }
      else if( c == Enum_GetNumFleets )
      {
         res = Tcl_NewLongObj(fleetList->size());
      }
      else if( c == Enum_GetFleetIdFromIndex )
      {
			const size_t idx = size_t(p[3]);
			if( idx >= fleetList->size() )
				return false;

			res = Tcl_NewWideIntObj((*fleetList)[idx]->objectId);
      }

      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }

   if( c == Enum_GetPlanetId )
   {
      // make sure we are a fleet
      Fleet *fleet = dynamic_cast<Fleet*>(pop);
      if( !fleet ) return false;

      uint64_t loc = formXYhash(pop->getX(), pop->getY());
      std::map<uint64_t, Planet*>::iterator pm = planetMap_.find(loc);
      if( pm == planetMap_.end() )
      {
         Tcl_Obj *res = Tcl_NewStringObj("", strlen(""));
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
      }

      Tcl_Obj *res = Tcl_NewWideIntObj(pm->second->objectId);
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }
	//else
	if( c == Enum_OrderTransport )
	{
		const uint64_t tgtFid = p[3];
		PlaceableObject *tfplace = gameData_->getTargetObject(tgtFid);
		if( !tfplace ) return false;

		const unsigned itemIdx = unsigned(p[4]);
		if( itemIdx >= _numCargoTypes ) return false;

		int64_t amt = int64_t(p[5]);
		if( pop ->cargoList.cargoDetail[itemIdx].amount < amt )
			amt = pop ->cargoList.cargoDetail[itemIdx].amount;

		tfplace->cargoList.cargoDetail[itemIdx].amount += amt;
		pop ->cargoList.cargoDetail[itemIdx].amount -= amt;

		return true;
	}
	//else

   // order processing
   if( c == Enum_GetNumOrders || c == Enum_GetOrderNameFromIndex ||
       c == Enum_DeleteAtIndex || c == Enum_AddAtIndex ||
       c == Enum_GetOrderXMLfromIndex || c == Enum_OrderMove )
   {
      // see if we are at a planet
      Planet *planet = dynamic_cast<Planet*>(pop);
      if( planet )
      {
         std::vector<BuildQueueEntry*> &orders = planet->planetBuildQueue;
         if( c == Enum_GetNumOrders )
         {
            Tcl_Obj *res = Tcl_NewLongObj(orders.size());
            Tcl_SetObjResult(p.getInterp(), res);
            return true;
         }
         //else
         if( c == Enum_DeleteAtIndex )
         {
            unsigned idx = unsigned(p[3]);
				if( orders[idx]->remainingBuildQuantity > 1 )
				{
					--orders[idx]->remainingBuildQuantity;
					Tcl_Obj *res = Tcl_NewIntObj(0);
					Tcl_SetObjResult(p.getInterp(), res);
					return true;
				}
            delete orders[idx];
            vectorDelete(orders, idx);
				Tcl_Obj *res = Tcl_NewIntObj(1);
				Tcl_SetObjResult(p.getInterp(), res);
            return true;
         }
         //else
         if( c == Enum_AddAtIndex )
         {
            BuildQueueEntry *bqe = new BuildQueueEntry;
            bqe->ownerID  = planet->ownerID;
            bqe->planetID = planet->objectId;

            bqe->originalBuildQuantity  = 1;
            bqe->remainingBuildQuantity = 1;
            bqe->percentComplete        = 0;

            bqe->autoBuild = p[4] == 0 ? false : true;
            bqe->itemType = buildableItemType(p[5]);
            if( bqe->itemType == _ship_BUILDTYPE )
               bqe->designID = p[6];

				Tcl_Obj *l[2];
				l[0] = Tcl_NewIntObj(0);
				l[1] = Tcl_NewIntObj(0);
				Tcl_Obj *res = Tcl_NewListObj(2, l);
				Tcl_SetObjResult(p.getInterp(), res);

				bool insertAtEnd = false;
            unsigned idx = unsigned(p[3]);
            if( orders.size() )
            {
					int offset = 0;
					if( idx >= orders.size() )
					{
						insertAtEnd = true;
						offset = orders.size() - idx;
						Tcl_SetIntObj(l[1], offset);
						idx = orders.size() - 1;
					}

               BuildQueueEntry *op = orders[idx];
               if( buildQueueEntryMatch(op, bqe) )
               {
                  ++op->originalBuildQuantity;
                  ++op->remainingBuildQuantity;
                  delete bqe;

						if( insertAtEnd )
							Tcl_SetIntObj(l[1], offset - 1);
                  return true;
               }

					if( !insertAtEnd && idx > 0 )
					{
						op = orders[idx-1];
						if( buildQueueEntryMatch(op, bqe) )
						{
							++op->originalBuildQuantity;
							++op->remainingBuildQuantity;
							delete bqe;
							Tcl_SetIntObj(l[1], -1);
							return true;
						}
					}

               if( idx+1 < orders.size() )
               {
                  op = orders[idx+1];
                  if( buildQueueEntryMatch(op, bqe) )
                  {
                     ++op->originalBuildQuantity;
                     ++op->remainingBuildQuantity;
                     delete bqe;
							Tcl_SetIntObj(l[1], 1);
                     return true;
                  }
               }
            }

				Tcl_SetIntObj(l[0], 1);
				if( !insertAtEnd )
				{
               orders.push_back(NULL);
               vectorInsertAt(orders, bqe, unsigned(p[3]));
				}
				else
				{
               orders.push_back(bqe);
				}

            return true;
         }
			if( c == Enum_OrderMove )
			{
				unsigned idx = unsigned(p[3]);
				if( idx >= orders.size() )
					return true;
				int dir = int(p[4]);
				if( idx == 0 && dir < 0 )
					return true;
				if( idx+dir == orders.size() )
					return true;

				std::swap(orders[idx], orders[idx+dir]);
				return true;
			}
         //else
         assert( c == Enum_GetOrderNameFromIndex );
         std::string orderName;
			unsigned idx = unsigned(p[3]);
			if( idx >= orders.size() )
				idx = orders.size() - 1;
         BuildQueueEntry *bqe = orders[idx];
         if( bqe->autoBuild ) orderName = "A ";
         else                 orderName = "N ";

         if( bqe->itemType == _ship_BUILDTYPE )
         {
            Ship *pDesign = findDesignById(gameData_->playerDesigns, bqe->designID);
            orderName += pDesign->getName();
         }
         else
         {
            orderName += BuildableItemDescriptions[bqe->itemType];
         }

         char buf[512];
         orderName += " ";
         sprintf(buf, "%d", bqe->remainingBuildQuantity);
         orderName += buf;
         orderName += " - ";
         unsigned tmpDiv = fractionalCompletionDenominator / 100;
         sprintf(buf, "%d", bqe->percentComplete / tmpDiv);
         orderName += buf;
         orderName += "%";
         Tcl_Obj *res = Tcl_NewStringObj(orderName.c_str(),
                                         strlen(orderName.c_str()));
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
      }
      //else

      // fleet orders
      if( c == Enum_GetNumOrders )
      {
         Tcl_Obj *res = Tcl_NewLongObj(pop->currentOrders.size());
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
      }
      //else
      if( c == Enum_DeleteAtIndex )
      {
         const unsigned idx = p[3];
         if (pop->currentOrders.size() > idx)
         {
         	delete pop->currentOrders[idx];
         	vectorDelete(pop->currentOrders, idx);
         }
         return true;
      }
		//else
      if( c == Enum_AddAtIndex )
      {
         uint64_t arg3 = p[3];

         Command orderEnum = Command(p[4]);
			if( orderEnum == Enum_AddAtIndex )
			{
				// fleet ship transfer
				Fleet *fromFleet = gameData_->getFleetByID(p[5]);
				if( !fromFleet ) return false;

				const size_t shipOff = size_t(arg3);
				if( shipOff >= fromFleet->shipRoster.size() ) return false;

				Ship *tgt = fromFleet->shipRoster[shipOff];
				uint64_t ct = p[6];

				if( tgt->quantity < ct ) return false;
				if( ct == 0 ) return true;

				Fleet *toFleet = dynamic_cast<Fleet*>(pop);
				if( !toFleet ) return false;

				handleFleetXfer(toFleet, fromFleet, tgt, ct);

				Ship *toFtShip = toFleet->hasShip(tgt->designhullID);
				if( toFtShip )
				{
					// target fleet has the ship, just shuffle counts
					toFtShip->quantity += ct;
					     tgt->quantity -= ct;

					if( tgt->quantity == 0 )
					{
						deleteShip(p, fromFleet, shipOff);
					}
					else
						fromFleet->updateFleetVariables(*gameData_);

					toFleet->updateFleetVariables(*gameData_);

					return true;
				}
				//else, to fleet lacks the ship

				if( tgt->quantity == ct )
				{
					// move all, easy
					toFleet->shipRoster.push_back(tgt);
					toFleet->updateFleetVariables(*gameData_);

					// handle last ship move
					deleteShip(p, fromFleet, shipOff);

					return true;
				}
				//else move some
				//Ned, handle damage and cargo assignment
				Ship *newShip = new Ship(*tgt);
				    tgt->quantity -= ct;
				newShip->quantity =  ct;

				toFleet->shipRoster.push_back(newShip);

				fromFleet->updateFleetVariables(*gameData_);
				toFleet->updateFleetVariables(*gameData_);

				return true;
			}
			//else orders

			const unsigned orderIdx = unsigned(arg3);
			Order *newOrderp = new Order;
         newOrderp->playerNumber = pop->ownerID;
         newOrderp->orderNumber = orderIdx + 1;

         newOrderp->warpFactor = 0;

			if( orderEnum == Enum_OrderMove )
         {
            newOrderp->myOrderID = move_order;

            unsigned x = unsigned(p[5]), y = unsigned(p[6]);
            newOrderp->destination.set(x, y);
            newOrderp->targetID   = 0; // follow id
            newOrderp->objectId   = 0;

				Location startLoc = pop->coordinates;
				CargoManifest origCargo = pop->cargoList;
				if( 0 < orderIdx && orderIdx <= pop->currentOrders.size() )
				{
					Order *prevO = pop->currentOrders[orderIdx-1];
					startLoc = prevO->destination;
					if( prevO->myOrderID == gui_load_order ||
					    prevO->myOrderID == load_cargo_order )
						pop->cargoList = pop->cargoList + prevO->cargoTransferDetails;
				}

				newOrderp->orderDescriptorString = OrderString[newOrderp->myOrderID];
				newOrderp->objectId = pop->objectId;

				newOrderp->warpFactor = pickSpeed((Fleet*)pop,
					newOrderp/*orderIdx*/, startLoc);

				pop->cargoList = origCargo;

				//Ned, need to restructure so this can be before pick speed
				pop->currentOrders.push_back(NULL);
				vectorInsertAt(pop->currentOrders, newOrderp, orderIdx);
				return true;
         }

			if( orderEnum == Enum_OrderTransport )
         {
            bool isUnload = p[5] == 1;

            unsigned x, y;
            if( orderIdx == 0 )
            {
	            if( isUnload )
	               newOrderp->myOrderID = gui_unload_order;
	            else
	               newOrderp->myOrderID = gui_load_order;

               x = pop->getX();
               y = pop->getY();
            }
            else
            {
	            if( isUnload )
	               newOrderp->myOrderID = unload_cargo_order;
	            else
	               newOrderp->myOrderID = load_cargo_order;

               x = pop->currentOrders[orderIdx-1]->destination.getX();
               y = pop->currentOrders[orderIdx-1]->destination.getY();
            }
            newOrderp->destination.set(x, y);

				const uint64_t hash = formXYhash(x, y);
				std::map<uint64_t, Planet*>::const_iterator pm = planetMap_.find(hash);
				if( pm != planetMap_.end() )
					newOrderp->targetID = pm->second->objectId;
         }
         else if( orderEnum == Enum_OrderColonize )
         {
            newOrderp->myOrderID = colonize_world_order;
            unsigned x, y;
            if( orderIdx == 0 )
            {
               x = pop->getX();
               y = pop->getY();
            }
            else
            {
               x = pop->currentOrders[orderIdx-1]->destination.getX();
               y = pop->currentOrders[orderIdx-1]->destination.getY();
            }
            newOrderp->destination.set(x, y);
            uint64_t hash = formXYhash(x, y);
            std::map<uint64_t, Planet*>::iterator pm = planetMap_.find(hash);
            if( pm != planetMap_.end() )
               newOrderp->targetID = pm->second->objectId;
         }
         else if( orderEnum == Enum_DeleteAtIndex )
			{
            newOrderp->myOrderID = scrap_fleet_order;
			}
         newOrderp->orderDescriptorString = OrderString[newOrderp->myOrderID];

         newOrderp->objectId = pop->objectId;

         //newOrderp->cargoTransferDetails;

         pop->currentOrders.push_back(NULL);
         vectorInsertAt(pop->currentOrders, newOrderp, orderIdx);
         return true;
      }
      //else
      const size_t oi = size_t(p[3]);
      if( oi >= pop->currentOrders.size() )
      {
			std::ostringstream os;
			os << "Fleet id: " << planetId << ", bad order idx " << oi;
         const char *strRes = os.str().c_str();
      	Tcl_Obj *res = Tcl_NewStringObj(strRes, strlen(strRes));
         Tcl_SetObjResult(p.getInterp(), res);
         return false;
      }

      Order *orderp = pop->currentOrders[oi];

      if( c == Enum_GetOrderXMLfromIndex )
      {
         std::string orderXML;
         orderp->toXMLString(orderXML);

         Tcl_Obj *res = Tcl_NewStringObj(orderXML.c_str(),
                                         strlen(orderXML.c_str()));
         Tcl_SetObjResult(p.getInterp(), res);
         return true;
      }
      //else
      assert( c == Enum_GetOrderNameFromIndex );
      std::string orderName = orderp->orderDescriptorString;
      orderName += " ";

      if (orderp->myOrderID == move_order)
      {
         // show destination
         const uint64_t tmp64 = formXYhash(orderp->destination.getX(),
                                     orderp->destination.getY());
         std::map<uint64_t, Planet*>::iterator pm = planetMap_.find(tmp64);
         if( pm == planetMap_.end() )
         {
               orderName += "(";
               char buf[512];
               sprintf(buf, "%d", orderp->destination.getX());
               orderName += buf;
               orderName += ", ";
               sprintf(buf, "%d", orderp->destination.getY());
               orderName += buf;
               orderName += ")";
         }
         else
         {
            orderName += pm->second->getName();
         }
      }

      Tcl_Obj *res = Tcl_NewStringObj(orderName.c_str(),
                                      strlen(orderName.c_str()));
      Tcl_SetObjResult(p.getInterp(), res);
      return true;
   }

   // generate (new fleet?)
   if( c == Cmd_Generate )
   {
      uint64_t ret = IdGen::getInstance()->makeFleet(
          self->race->ownerID, true, self->objectMap);

      // return the new fleet id
      Tcl_Obj *res = Tcl_NewWideIntObj(ret);

		// create the empty fleet
		Fleet *newFleet = new Fleet;
		newFleet->setName("");
		newFleet->objectId = ret;
		newFleet->ownerID  = self->race->ownerID;
		newFleet->coordinates.set(pop->getX(), pop->getY());
		newFleet->updateFleetVariables(*gameData_);

		self->fleetList.push_back(newFleet);
		gameData_->fleetList.push_back(newFleet);

		// update class fleet maps
      uint64_t loc = formXYhash(pop->getX(), pop->getY());
      std::map<uint64_t, Planet*>::iterator pm = planetMap_.find(loc);

		// update Tcl fleet maps
		if( pm == planetMap_.end() )
		{
			// in space
			std::map<uint64_t, std::vector<Fleet*> >::iterator inSpace =
			    fleetsInSpace_.find(loc);
			if( inSpace == fleetsInSpace_.end() )
				return false;

			// with other fleets
			inSpace->second.push_back(newFleet);

			// update the in space fleet map
			const char *tclCmd = "array size fleetMap";
			Tcl_Eval(p.getInterp(), tclCmd);
			Tcl_Obj *numFleet = Tcl_GetObjResult(p.getInterp());
			int numFleets;
			Tcl_GetIntFromObj(p.getInterp(), numFleet, &numFleets);
			char buf[32];
			sprintf(buf, "%d", numFleets);
			Tcl_SetVar2Ex(p.getInterp(), "fleetMap", buf, res, TCL_GLOBAL_ONLY);
		}
		else // in orbit
		{
			Planet *planet = pm->second;
			std::map<Planet*, std::vector<Fleet*> >::iterator orbit =
			    fleetsInOrbit_.find(planet);
			if( orbit == fleetsInOrbit_.end() )
				return false;

			orbit->second.push_back(newFleet);
		}

		// update the all fleet map
		const char *tclCmd = "array size allFleetMap";
		Tcl_Eval(p.getInterp(), tclCmd);
		Tcl_Obj *numFleet = Tcl_GetObjResult(p.getInterp());
		int numFleets;
		Tcl_GetIntFromObj(p.getInterp(), numFleet, &numFleets);
		char buf[32];
		sprintf(buf, "%d", numFleets);
		Tcl_SetVar2Ex(p.getInterp(), "allFleetMap", buf, res, TCL_GLOBAL_ONLY);

		Tcl_SetObjResult(p.getInterp(), res);
		return true;
	}

   char buf[32];
	if( c == Enum_GetScan )
	{
		ScanComponent scan;
		Planet *plan = dynamic_cast<Planet*>(pop);
		if( plan )
		{
			if( plan->infrastructure.scanner == 0 )
			{
				const char *const cres = "0 0";
			   Tcl_Obj *res = Tcl_NewStringObj(cres, strlen(cres));
			   Tcl_SetObjResult(p.getInterp(), res);
				return true;
			}

			PlayerData *pdp = self;
			const StructuralElement *sep = pdp->getBestPlanetScan();
			unsigned getName = unsigned(p[3]);
			if( getName )
			{
			   Tcl_Obj *res = Tcl_NewStringObj(sep->name.c_str(),
			                                  sep->name.length());
			   Tcl_SetObjResult(p.getInterp(), res);
				return true;
			}
			//else
			scan = sep->scanner;
			if( pdp->race->lrtVector[noAdvancedScanners_LRT] == 1 )
				scan.baseScanRange *= 2;
		}
		else
		{
			Fleet *fleet = dynamic_cast<Fleet*>(pop);
			assert( fleet );
			scan = fleet->getScan(*gameData_);
		}

		sprintf(buf, "%d %d", scan.baseScanRange, scan.penScanRange);
	   Tcl_Obj *res = Tcl_NewStringObj(buf,
	                                  strlen(buf));
	   Tcl_SetObjResult(p.getInterp(), res);
		return true;
	}
   //else
	if( c == Enum_Speed )
	{
		Fleet *fleet = dynamic_cast<Fleet*>(pop);
		if( fleet )
		{
         unsigned idx = unsigned(p[3]);
			int spd = int(p[4]);
			if( spd == -1 )
			{
				// get speed from index
				unsigned oSpd = 0;
				if( idx < fleet->currentOrders.size() )
					oSpd = fleet->currentOrders[idx]->warpFactor;
				Tcl_Obj *res = Tcl_NewIntObj(oSpd);
				Tcl_SetObjResult(p.getInterp(), res);
				return true;
			}
			// set speed at index
			if( idx < fleet->currentOrders.size() )
				fleet->currentOrders[idx]->warpFactor = spd;
			return true;
		}
		//else planet
		Planet *plan = dynamic_cast<Planet*>(pop);
		assert( plan );
		int spd = int(p[3]);
		if( spd == -1 )
		{
			// get speed
			unsigned pSpd = plan->packetSpeed;
			Tcl_Obj *res = Tcl_NewIntObj(pSpd);
			Tcl_SetObjResult(p.getInterp(), res);
			return true;
		}
		// set speed
		plan->packetSpeed = spd;
		return true;
	}

   if( c == Enum_GetX )
   {
      sprintf(buf, "%d", pop->getX());
   }
   else if( c == Enum_GetY )
   {
      sprintf(buf, "%d", pop->getY());
   }

   Tcl_Obj *res = Tcl_NewStringObj(buf,
                                  strlen(buf));
   Tcl_SetObjResult(p.getInterp(), res);
   return true;
}

// ns_universe [ns_getX | ns_getY | ns_getYear]
bool NewStarsTcl::universe(Parms &p)
{
   if( !fileOpen_ ) return false;
   if( !gameData_->hasPlanets() ) return false;

   bool retVal = true;

   Command c = Command(p[1]);
   char buf[32];
   if( c == Enum_GetX )
   {
      sprintf(buf, "%d", gameData_->theUniverse->width);
   }
   else if( c == Enum_GetY )
   {
      sprintf(buf, "%d", gameData_->theUniverse->height);
   }
   else if( c == Enum_GetYear )
   {
      sprintf(buf, "%d", gameData_->theUniverse->gameYear);
   }
   else
   {
      //sprintf(buf, "Error");
      //retVal = false;
   }

   Tcl_Obj *res = Tcl_NewStringObj(buf,
                                  strlen(buf));
   Tcl_SetObjResult(p.getInterp(), res);
   return retVal;
}

void NewStarsTcl::setEnums(Tcl_Interp *interp)
{
   char buf[32];
   unsigned i; // MS bug

   for(i = Cmd_Invalid; i < Enum_Max; i++)
   if( NS_Tcl_Cmds[i][0] )
   {
      sprintf(buf, "%d", i);
      Tcl_SetVar(interp, NS_Tcl_Cmds[i], buf, 0);
   }
}

//----------------------------------------------------------------------------
/// Tcl callback
int generate(ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[])
{
	// retrieve our info
   NewStarsTcl *nstp = static_cast<NewStarsTcl*>(clientData);

	// build the parms helper
	Parms p(interp, objv, unsigned(objc));

	// find the command method
   unsigned onum = unsigned(p[0]);
   if( onum >= NewStarsTcl::Cmd_Max ) return TCL_ERROR;

   NewStarsTcl::AnOrder order = nstp->orders[onum];

	// call it
	const bool success = order && (nstp->*order)(p);

	return success ? TCL_OK : TCL_ERROR;
}

void destructor(ClientData clientData)
{
   NewStarsTcl *nstp = static_cast<NewStarsTcl*>(clientData);
   delete nstp;
}

extern "C" int Nstclgui_Init(Tcl_Interp *interp);

int Nstclgui_Init(Tcl_Interp *interp)
{
	if( Tcl_InitStubs(interp, "8.5", 0) == NULL )
		return TCL_ERROR;

   initNameArrays();

   NewStarsTcl *nstp = new NewStarsTcl;
   nstp->setEnums(interp);

   Tcl_CreateObjCommand(interp, "newStars", generate, nstp, destructor);

   return TCL_OK;
}

