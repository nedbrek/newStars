#ifdef MSVC
#pragma warning(disable: 4786)
#endif
/*****************************************************************************
                          fleet -  description
                             -------------------
    begin                : 08 Jul 04
    copyright            : (C) 2004,2005
                           All Rights Reserved.
 ****************************************************************************/

/*****************************************************************************
 This program is copyrighted, and may not be reproduced in whole or
 part without the express permission of the copyright holders.
*****************************************************************************/
#include "classDef.h"
#include "newStars.h"
#include "planet.h"
#include "utils.h"
#include "race.h"

#include <queue>
#include <sstream>
#include <iostream>
#include <cmath>

template<typename Type>
void deleteAll(std::vector<Type*> *vec)
{
	for(size_t i = 0; i < vec->size(); ++i)
		delete (*vec)[i];
	vec->clear();
}

Fleet::Fleet(void)
{
	fleetID            = 0;
	battleOrderID      = 0;
	totalCargoCapacity = 0;
	fuelCapacity       = 0;
	mass               = 0;
	hasColonizer       = false;
	topSpeed           = 0;
	topSafeSpeed       = 0;
	lastKnownSpeed     = 0;
	lastKnownDirection = 0;
	followDistance     = 0;
	myType             = gameObjectType_fleet;
	for(unsigned i = 0; i < _numberOfMineTypes; i++)
		mineLayingRates[i] = 0;
}

void Fleet::copyFrom(const Fleet &b)
{
	// GameObject
	name     = b.name;
	objectId = b.objectId;
	ownerID  = b.ownerID;
	myType   = b.myType;

	// Placeable
	coordinates        = b.coordinates;
	cargoList          = b.cargoList;
	penScanRadius      = b.penScanRadius;
	nonPenScanRadius   = b.nonPenScanRadius;
	currentOrders      = b.currentOrders;

	battleOrderID      = b.battleOrderID;
	totalCargoCapacity = b.totalCargoCapacity;
	fuelCapacity       = b.fuelCapacity;
	mass               = b.mass;
	maxPacketSpeed     = b.maxPacketSpeed;
	fleetID            = b.fleetID;
	hasColonizer       = b.hasColonizer;
	isSpaceStation     = b.isSpaceStation;
	topSpeed           = b.topSpeed;
	topSafeSpeed       = b.topSafeSpeed;
	lastKnownSpeed     = b.lastKnownSpeed;
	lastKnownDirection = b.lastKnownDirection;
	followDistance     = b.followDistance;

	for(unsigned j = 0; j < _numberOfMineTypes; ++j)
		mineLayingRates[j] = b.mineLayingRates[j];

	for(unsigned i = 0; i < b.shipRoster.size(); i++)
	{
		Ship *oldShip = b.shipRoster[b.shipRoster.size()-1-i];

		shipRoster.push_back(new Ship(*oldShip));
	}
}

// deep copy for copy constructor
Fleet::Fleet(const Fleet &cFleet)
{
	copyFrom(cFleet);
}

Fleet& Fleet::operator=(const Fleet &b)
{
	deleteAll(&shipRoster);

	copyFrom(b);

	return *this;
}

Ship* Fleet::hasShip(uint64_t designId) const
{
	for(std::vector<Ship*>::const_iterator i = shipRoster.begin(); i != shipRoster.end(); ++i)
	{
		if( (**i).designhullID == designId )
			return *i;
	}
	return NULL;
}

/// dispatch to the action in 'what.ID'
bool Fleet::actionHandler(GameData &gd, const Action &what)
{
	switch( what.ID )
	{
	// map straight across
	case   scrapFleet_action: return    scrapAction(gd);
	case    fleetMove_action: return     moveAction(gd);
	case follow_fleet_action: return   followAction(gd);
	case     colonize_action: return colonizeAction(gd);

	case standard_bomb_planet_action:
	case    smart_bomb_planet_action: return bombAction(gd);

	// load and unload need to know if it is a GUI action
	case unload_cargo_action:
	case   gui_unload_action:
		return unloadAction(gd, what.ID == gui_unload_action);

	case load_cargo_action:
	case   gui_load_action:
		return loadAction(gd, what.ID == gui_load_action);

	// check for lay before move
	case lay_mines_action:
	case lay_mines_before_move_action:
		return layMinesAction(gd, what.ID == lay_mines_action ? 1 :0);

	default:
		std::cerr << "Error, invalid action sent to fleet.  Object #" << objectId
		          << "(" <<name << ")\n";
		return false;
	}
}

/// check the fleet for the scrap order
///@return true if the fleet is scrapped, else false
bool Fleet::scrapAction(GameData &gd)
{
	// if no orders, done
	if( !currentOrders.size() )
		return false;

	// what is the first order?
	Order *firstOrderInQueue = currentOrders[0];

        if( firstOrderInQueue->myOrderID != scrap_fleet_order )
		return false; // not scrap
	// we are scrapping
	// mark us for deletion
	gd.deleteFleet(objectId);

	// where are we?
	uint64_t locHash = formXYhash(getX(), getY());
	std::map<uint64_t, Planet*>::iterator p = gd.planetMap.find(locHash);
	if( p == gd.planetMap.end() )
		return true; // in space, all minerals lost
	//else at planet
	Planet *const pp = p->second;

	unsigned res = 0; // resources from scrap (ultimate recycling)
	CargoManifest cargo = scrapValue(gd.getPlayerData(ownerID)->race,
	   pp->starBaseID == 0 ? SCRAP_NO_STARBASE : SCRAP_STARBASE, &res);

	pp->cargoList = pp->cargoList + cargo;
	pp->resourcesRemainingThisTurn += res;

	return true;
}

/// take minerals from the fleet, and place them in another fleet
/// or on a planet.
///@return true on success, else false
bool Fleet::unloadAction(GameData &gd, bool guiBased)
{
	// if no orders, success
	if( !currentOrders.size() )
		return true;

	// if no unload order, success
	Order *const firstOrderInQueue = currentOrders[0];
	const orderID expectedOrder = guiBased ?
	   gui_unload_order : unload_cargo_order;
	if( firstOrderInQueue->myOrderID != expectedOrder )
		return true;

	// if at a foreign planet
	Planet *const p = gd.getPlanetByID(firstOrderInQueue->targetID);
	if( p && p->ownerID != ownerID )
	{
		// possible invasion
		WaypointDropDetail *wdd = new WaypointDropDetail();
		wdd->theRace = gd.getRaceById(ownerID);

		// transfer the colonists into the waypoint drop
		wdd->number = firstOrderInQueue->cargoTransferDetails.cargoDetail[_colonists].amount;
		cargoList.cargoDetail[_colonists].amount -= wdd->number;

		// add the drop to the planet
		p->waypointDropRecords.push_back(wdd);

		// clear the colonists from the order
		firstOrderInQueue->cargoTransferDetails.cargoDetail[_colonists].amount = 0;
	}

	// proceed to normal unload
	return PlaceableObject::unloadAction(gd, guiBased);
}

/// execute the order to follow another fleet
///@return false if the fleet strikes a minefield, else true
bool Fleet::followAction(GameData &gd)
{
	//cout<<"*** Looking at fleet "<<name<<" for motion orders\n";
	//   lastKnownSpeed = 0;

	// if no orders, success
	if( !currentOrders.size() )
		return true;

	//cout<<"*** fleet "<<name<<" has orders\n";

	Order *const firstOrderInQueue = currentOrders[0];
	// if no move order, success
	if( firstOrderInQueue->myOrderID != follow_fleet_order)
		return true;

	//cout<<"*** fleet "<<name<<" has move orders\n";

	//Ned?
	if( (firstOrderInQueue->warpFactor * firstOrderInQueue->warpFactor) < gd.followPhase )
		return true;

	// if fleet is not the same it was last year, need to do something clever
	Fleet *origTargetFleet = gd.getLastYearFleetByID(firstOrderInQueue->targetID);
	Fleet    *targetFleet  = gd.getFleetByID        (firstOrderInQueue->targetID);
	if( !isIdentical(origTargetFleet, targetFleet) )
	{
		// get list of fleets at this location for the target player
		vector<Fleet*> curVec;
		unsigned i; // MS bug
		for(i = 0; i < gd.fleetList.size(); ++i)
		{
			Fleet *cf = gd.fleetList[i];
			if( cf->coordinates == origTargetFleet->coordinates &&
			    cf->ownerID == origTargetFleet->ownerID )
			{
				// cf is a current fleet in the right location and owned by the right
				Fleet *origCF = gd.getLastYearFleetByID(cf->objectId);
				if( !isIdentical(cf, origCF) )
					curVec.push_back(cf);
        }
		}
		//Ned? what if the ship design is deleted, and built ships are removed?
		if( curVec.size() == 0 )
			starsExit("Fatal Error: pursuit fleet lost target", -1);

		if( targetFleet )
			curVec.push_back(targetFleet);
		else
			targetFleet = curVec[0];

		// original fleet mass, without cargo
		uint64_t oMass = origTargetFleet->mass - origTargetFleet->cargoList.getMass();
		// current fleet mass with desired ID, without cargo
		uint64_t cMass = targetFleet->mass - targetFleet->cargoList.getMass();
		if( cMass > oMass ) // we don't want this one -- too big
			cMass = 0;

		// find the largest fleet mass, less than the orignal
		for(i = 0; i < curVec.size(); ++i)
		{
			Fleet *const cFleet = curVec[i];

			const uint64_t tMass = cFleet->mass - cFleet->cargoList.getMass();
			if( cMass < tMass && tMass < oMass )
			{
				cMass = tMass;
				targetFleet = cFleet;
			}
		}
	}

	//Ned?
	firstOrderInQueue->destination = targetFleet->coordinates;

	unsigned currentRange = 1;
	Location tempPoint = coordinates;
	tempPoint.moveToward(targetFleet->coordinates, currentRange);
	if( tempPoint == coordinates ) return true;

	lastKnownSpeed = firstOrderInQueue->warpFactor;

	// check for fuel and minefield interference
	Minefield *impactField = NULL;
	if( gd.followPhase == 1 ) // send pursuit message to player
	{
		std::ostringstream os;
		os << "Your fleet: " << name << " is in pursuit of target fleet ID: "
		   << targetFleet->objectId << " -- owned by the ";

		PlayerData *targetOwner = gd.getPlayerData(targetFleet->ownerID);
		os << targetOwner->race->pluralName;

		gd.playerMessage(ownerID, 0, os.str());
	}

	// check for minefield interference
	std::vector<Minefield*> &mfv = gd.minefieldList;
	PlayerData *cPlayer = gd.getPlayerData(ownerID);
	for(unsigned i = 0; i < mfv.size(); ++i)
	{
		Minefield *currentField = mfv[i];

		// check what the mf owner thinks of us
		PlayerData *mPlayer = gd.getPlayerData(currentField->ownerID);
		std::map<unsigned, PlayerRelation>::const_iterator it =
		   mPlayer->playerRelationMap_.find(ownerID);

		// only registered friends are safe
		if( it != mPlayer->playerRelationMap_.end() && it->second == PR_FRIEND )
			continue;

		// if not seen before, register as neutral
		if( it == mPlayer->playerRelationMap_.end() )
		{
			mPlayer->playerRelationMap_[ownerID] = PR_NEUTRAL;
		}

		// check for impact
		if( currentField->needToWorry(coordinates, tempPoint, currentField->coordinates.getX(),
		                              currentField->coordinates.getY(), currentField->radius) )
		{
			unsigned tDist = currentField->whereStruckMinefield(coordinates, tempPoint, firstOrderInQueue->warpFactor, cPlayer->race);
			if( tDist < currentRange ) // we hit this MF earlier than current completion range
			{
				currentRange = tDist;
				impactField = currentField;
			}
		}
	}

	if( impactField )
	{
		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		   << " struck the " << impactField->getName() << " ("
		   << impactField->objectId << ") after traveling " << gd.followPhase
		   << " lightyears.";

		gd.playerMessage(ownerID, 0, os.str());

		impactField->damageFleet(gd, this);
	}

	coordinates.moveToward(targetFleet->coordinates, currentRange);

	return impactField == NULL;
}

void Fleet::assignMass(unsigned spd,
    std::map<unsigned, CargoMoveCap> *pMassAssign) const
{
	// build a data structure to rapidly find the best cargo movers
	std::priority_queue<CargoMoveCap> cargoVessels;
	for(size_t sid = 0; sid < shipRoster.size(); ++sid)
	{
		const Ship *ship = shipRoster[sid];
		if( ship->cargoCapacity > 0 )
		{
			cargoVessels.push(CargoMoveCap(sid, lastKnownSpeed, ship));
		}
	}

	// assign cargo
	std::map<unsigned, CargoMoveCap> &massAssignments = *pMassAssign;
	unsigned moveMass = cargoList.getMass();
	while( moveMass && cargoVessels.size() )
	{
		// get best cargo mover
		CargoMoveCap tmp = cargoVessels.top();

		// fill 'er up
		tmp.assignedCargo = std::min(moveMass, tmp.cargoCap);
		massAssignments.insert(std::pair<unsigned, CargoMoveCap>(tmp.idx, tmp));

		moveMass -= tmp.assignedCargo;

		cargoVessels.pop();
	}
	if( moveMass )
	{
		std::cerr << "Error: Fleet with more cargo than capacity!\n";
		starsExit("Fleet move", -1);
	}
}

int Fleet::calcFuel(const Location &dest, unsigned spd,
    std::map<unsigned, CargoMoveCap> *pMassAssign,
	 const Location *pstart) const
{
	std::map<unsigned, CargoMoveCap> &massAssignments = *pMassAssign;

	Location start = coordinates;
	if( pstart ) start = *pstart;

	// calc fuel to move desired range
	int fuelCon = 0;
	for(size_t j = 0; j < shipRoster.size(); ++j)
	{
		std::map<unsigned, CargoMoveCap>::iterator i = massAssignments.find(j);
		unsigned cargo = 0;
		if( i != massAssignments.end() )
			cargo = i->second.assignedCargo;

		fuelCon += shipRoster[j]->calcFuel(start, dest, spd, cargo);
	}

	return fuelCon;
}

// -MAXINT = -pi, MAXINT = pi
int makeRadianInt(double rad)
{
	return int(rad / PI * 2147483647.0);
}

bool Fleet::useJumpGate(GameData& gd)
{
    	lastKnownSpeed = 0; // in case we early out
	lastKnownDirection = 0;
	Order *firstOrderInQueue = currentOrders[0];
	if( firstOrderInQueue->myOrderID != move_order)  //should never be true
        {
		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << "encountered a severe code error.  A Jump gate use was expected but the parent move order disappeared!";
		gd.playerMessage(ownerID, 0, os.str());
                return true;
        }
        //check for jumpgate orders
        if (firstOrderInQueue->useJumpGate!=true)
        {
		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << "encountered a severe code error.  A Jump gate use was expected but the jump order disappeared!";
		gd.playerMessage(ownerID, 0, os.str());
                return true;
        }
        //find out if we are at a planet
         uint64_t fleetHash = formXYhash(getX(), getY());
         std::map<uint64_t, Planet*>::const_iterator plLoc =
	    gd.planetMap.find(fleetHash);
	Planet *pl = NULL;
	if( plLoc != gd.planetMap.end() )
		pl = gd.planetMap[fleetHash];

        if (pl==NULL) //not orbiting a planet, havn't put in MT JUMPGATE yet (would go here)
        {
 		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << "Attempted to use a jumpgate, but is not in orbit around a planet.  MT devices not in yet.  Jump Canceled";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }

        if (pl->starBaseID==0)
        {
 		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << "Attempted to use a jumpgate, but the planet it is orbiting has no starbase!";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }
        Fleet *sb = gd.getFleetByID(pl->starBaseID);
        if (sb==NULL)
        {
 		std::ostringstream os;
		os << "Starbase (" << pl->starBaseID << ')'
		    << " Seems to have disapeared while attempting to engage JUMPGATE!  Critical error!";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }

        if (!sb->hasJumpGame)
        {
 		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << " was ordered to use a jumpgate, but the planet's starbase has no gate!";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }

        //LOCATE JUMP TARGET
                //find out if we are at a planet
         uint64_t targetHash = formXYhash(firstOrderInQueue->destination.getX(), firstOrderInQueue->destination.getY());
         std::map<uint64_t, Planet*>::const_iterator plLoc2 =
	    gd.planetMap.find(targetHash);
	Planet *targetPlanet = NULL;
	if( plLoc2 != gd.planetMap.end() )
		pl = gd.planetMap[targetHash];

        if (targetPlanet==NULL) //Target is not a planet
        {
 		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << "Attempted to use a jumpgate, but the destination was not a planet!";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }
        if (targetPlanet->starBaseID==0)
                    {
 		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << "Attempted to use a jumpgate, but the planet it is jumping to has no starbase!";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }

        Fleet *target_sb = gd.getFleetByID(targetPlanet->starBaseID);
        if (sb==NULL)
        {
 		std::ostringstream os;
		os << "Starbase (" << targetPlanet->starBaseID << ')'
		    << " Seems to have disapeared while attempting to engage JUMPGATE!  Critical error!";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }

        if (!target_sb->hasJumpGame)
        {
 		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << " was ordered to use a jumpgate, but the destination planet's starbase has no gate!";
		gd.playerMessage(ownerID, 0, os.str());
                currentOrders.erase(currentOrders.begin());
                return true;
        }

        //source gate exists, dest gate exists.  Now move the ships

        coordinates.set(targetPlanet->coordinates.getX(),targetPlanet->coordinates.getY());
        //still need to check for damage and if cargo should be dumped...
        return true;
}
bool Fleet::moveAction(GameData &gd)
{
	//cout<<"*** Looking at fleet "<<name<<" for motion orders\n";
	lastKnownSpeed = 0; // in case we early out
	lastKnownDirection = 0;

	if( !currentOrders.size() )
	{
		// no orders, just sit here
		return true;
	}
	//cout<<"*** fleet "<<name<<" has orders\n";

	Order *firstOrderInQueue = currentOrders[0];
	if( firstOrderInQueue->myOrderID != move_order)
		return true; // no move order, so just sit

        //check for jumpgate orders
        if (firstOrderInQueue->useJumpGate==true)
            return useJumpGate(gd);
	// moving at speed
	lastKnownSpeed = firstOrderInQueue->warpFactor;
	
	//cout<<"*** fleet "<<name<<" has move orders\n";

	Location tempPoint = coordinates; // start at current coordinates

	unsigned currentRange = lastKnownSpeed * lastKnownSpeed;
	Location fuelLimit = coordinates;
	fuelLimit.moveToward(firstOrderInQueue->destination, currentRange);

	std::map<unsigned, CargoMoveCap> massAssignments;
	assignMass(lastKnownSpeed, &massAssignments);
	int fuelCon = calcFuel(fuelLimit, lastKnownSpeed, &massAssignments);

	// check for out of fuel
	//Ned, calc actual range
	if( fuelCon > 0 &&
	    unsigned(fuelCon) > cargoList.cargoDetail[_fuel].amount )
	{
		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		    << "has run out of fuel!  Speed reduced to Warp 1.";
		gd.playerMessage(ownerID, 0, os.str());

		// calculate distance on fuel remaining
		double tmp = cargoList.cargoDetail[_fuel].amount;
		tmp *= 100.0;
		tmp -= 9.0;
		tmp *= 200.0;

		double massEff = 0.0;
		for(size_t j = 0; j < shipRoster.size(); ++j)
		{
			std::map<unsigned, CargoMoveCap>::iterator i = massAssignments.find(j);
			unsigned cargo = 0;
			if( i != massAssignments.end() )
				cargo = i->second.assignedCargo;

			massEff += (shipRoster[j]->mass + cargo) * shipRoster[j]->quantity;
		}
		tmp /= massEff;

		currentRange = unsigned(tmp);
		fuelCon = cargoList.cargoDetail[_fuel].amount;
	}
	cargoList.cargoDetail[_fuel].amount -= fuelCon;

	tempPoint.moveToward(firstOrderInQueue->destination, currentRange);

	const std::vector<Minefield*> &mfv = gd.minefieldList;
	// check for minefield interference
	Minefield *impactField = NULL;
	for(size_t i = 0; i < mfv.size(); ++i)
	{
		Minefield *currentField = mfv[i];

		// ask the minefield owner if he is an ally
		PlayerData *mfOwner = gd.getPlayerData(currentField->ownerID);
		std::map<unsigned, PlayerRelation>::const_iterator it =
		   mfOwner->playerRelationMap_.find(ownerID);
		if( it == mfOwner->playerRelationMap_.end() )
		{
			mfOwner->playerRelationMap_[ownerID] = PR_NEUTRAL;
		}
		else if( it->second == PR_FRIEND )
			continue; // no problem
		//else

		if( currentField->needToWorry(coordinates, tempPoint,
		        currentField->coordinates.getX(),
		        currentField->coordinates.getY(), currentField->radius) )
		{
			PlayerData *cPlayer = gd.getPlayerData(ownerID);
			unsigned tDist = currentField->whereStruckMinefield(coordinates,
			    tempPoint, firstOrderInQueue->warpFactor, cPlayer->race);
			// if we hit this MF earlier than current completion range
			if( tDist < currentRange )
			{
				currentRange = tDist;
				impactField = currentField;
			}
		}
	}

	if( impactField )
	{
		// we hit a minefield
		std::ostringstream os;
		os << "Fleet: " << name << ' ' << '(' << objectId << ')'
		   << " struck the " << impactField->getName() << ' ' << '('
		   << impactField->objectId << ") after traveling " << currentRange
		   << " lightyears.";

		// message to fleet owner from host (id 0)
		gd.playerMessage(ownerID, 0, os.str());

		// take damage
		impactField->damageFleet(gd, this);
	}

	// actually move
	Location orig(coordinates);
	coordinates.moveToward(firstOrderInQueue->destination, currentRange);

	lastKnownDirection = makeRadianInt(orig.makeAngleTo(coordinates));

	// rounding may mean we actually arrived, so check
	if( firstOrderInQueue->destination == coordinates )
	{
		// we are here, so delete the orders
		currentOrders.erase(currentOrders.begin());
	}

	return true;
}

// fleet should lay mines (if ordered).  layPhase indicated where in the
// order of events this is occuring  (Normal: 'layPhase' == 1)
// (lay before move: 'layPhase' == 0)
bool Fleet::layMinesAction(GameData& gd, int layPhase)
{
	if( currentOrders.empty() )
		return true;

	Order *firstOrderInQueue = currentOrders[0];
	if( firstOrderInQueue->myOrderID != lay_mines_order)
		return true;

	PlayerData *thePlayer = gd.getPlayerData(ownerID);

	// check the phase against the PRT.  if we are not in the
	// "normal" phase (1), and are not an SD fleet, then return
	if( layPhase != 1 && thePlayer->race->prtVector[spaceDemolition_PRT] != 1 )
		return true;

	int totalAddMines[_numberOfMineTypes];

	int totalConsolidatedRate = 0;
	for(unsigned i = 0; i < _numberOfMineTypes; ++i)
	{
		totalAddMines[i] = mineLayingRates[i];
		totalConsolidatedRate += mineLayingRates[i];
	}

	if( totalConsolidatedRate == 0 )
	{
		string msg = "Fleet: "+name+" was given orders to lay a minefield, but has no minelaying capability!";
		gd.playerMessage(ownerID, 0, msg);
		return true;
	}

	for(unsigned MT = 0; MT <_numberOfMineTypes; ++MT)
	{
		int numberMines[_numberOfMineTypes];

		for(unsigned init = 0; init < _numberOfMineTypes; ++init)
			numberMines[init] = 0;

		numberMines[MT] = totalAddMines[MT];

		bool minesAddedToExistingField = false;
		for(unsigned i = 0; i < thePlayer->minefieldList.size() &&
		    !minesAddedToExistingField; i++)
		{
			Minefield *MF = thePlayer->minefieldList[i];
			if( MF->canAdd(coordinates, numberMines) )
			{
				MF->addMines(coordinates, numberMines);

				minesAddedToExistingField = true;
			}
		}

		if( !minesAddedToExistingField )
		{
			// create a new minefield
			Minefield *NF = new Minefield();
			NF->objectId = nextObjectId++;

			NF->coordinates = coordinates;
			NF->ownerID     = ownerID;
			NF->radius      = 0;
			NF->addMines(coordinates, numberMines);

			std::string newName = thePlayer->race->singularName;
			newName += " ";
			newName += mineTypeArray[MT];
			newName += " minefield";
			NF->setName(newName);

			gd.minefieldList.push_back(NF);
			thePlayer->minefieldList.push_back(NF);
		}
	}
	return true;
}

// process a colonize order at the head of the order queue
bool Fleet::colonizeAction(GameData &gd)
{
	// if no orders
	if( currentOrders.size() == 0 )
		return true; // done

	// check head
	Order *firstOrderInQueue = currentOrders[0];
	if( firstOrderInQueue->myOrderID != colonize_world_order )
		return true; // done

	// which planent?
	Planet *whichP = gd.getPlanetByID(firstOrderInQueue->targetID);

	// notify player what happened
	std::ostringstream os;

	if( !hasColonizer )
	{
		os << "Error: you ordered fleet " << objectId << " to colonize planet "
		    << whichP->getName()
		    << ", but the fleet has no ships with colonizers!";

		gd.playerMessage(ownerID, 0, os.str());
		return true;
	}
	//else

	if( cargoList.cargoDetail[_colonists].amount == 0 )
	{
		os << "Error: you ordered fleet " << objectId << " to colonize planet "
		    << whichP->getName()
		    << ", but the fleet is not carrying colonists!";

		gd.playerMessage(ownerID, 0, os.str());
		return true;
	}
	//else

	if( !whichP )
	{
		os << "Error: you ordered fleet " << objectId
		    << " to colonize an invalid location.";

		gd.playerMessage(ownerID, 0, os.str());
		return true;
	}
	//else

	if( whichP->ownerID )
	{
		os << "Error: you ordered fleet " << objectId << " to colonize planet "
		    << whichP->getName() << ", but the planet is already owned by the ";

		const Race *tRace = gd.getRaceById(whichP->ownerID);
		os << tRace->pluralName;

		gd.playerMessage(ownerID, 0, os.str());
		return true;
	}
	//else success!

	// create a drop point record
	// (which will be resolved in ground combat phase)
	WaypointDropDetail *wdd = new WaypointDropDetail();

	wdd->theRace = gd.getRaceById(ownerID);

	// transfer colonists into the record
	wdd->number = cargoList.cargoDetail[_colonists].amount;
	cargoList.cargoDetail[_colonists].amount = 0;

	// fuel cannot be transferred
	cargoList.cargoDetail[_fuel     ].amount = 0;

	whichP->waypointDropRecords.push_back(wdd);

	// minerals go into the planet regardless
	whichP->cargoList = whichP->cargoList + cargoList;
	whichP->cargoList = whichP->cargoList + scrapValue(gd.getRaceById(ownerID),
	    SCRAP_COLONIZE, NULL);

	// colonizing fleet is removed
	gd.deleteFleet(objectId); // post a delete message for cleanup

	return true;
}

bool Fleet::parseXML(mxml_node_t *tree, GameData &gd, bool enemy)
{
  if( !PlaceableObject::parseXML(tree) )
    return false;

  parseXML_noFail(tree,"FLEET_ID",fleetID);
  parseXML_noFail(tree,"BATTLE_ORDER_ID",battleOrderID);
  mxml_node_t *child = mxmlFindElement(tree, tree, "CARGOCAPACITY", NULL,
                                       NULL, MXML_DESCEND);
  if( !child )
    {
      cerr<<"Error, could not parse CARGOCAPACITY for fleet:"<<name<<"\n";
      return false;
    }
  child = child->child;

  totalCargoCapacity = atoi(child->value.text.string);

  child = mxmlFindElement(tree, tree, "FUELCAPACITY", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, could not parse FUELCAPACITY for fleet:" << name << endl;
      return false;
    }
  child = child->child;
  fuelCapacity = atoi(child->value.text.string);

  child = mxmlFindElement(tree, tree, "TOPSPEED", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, could not parse TOPSPEED for fleet:" << name << endl;
      return false;
    }
  child = child->child;
  topSpeed = atoi(child->value.text.string);

  child = mxmlFindElement(tree, tree, "SAFESPEED", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, could not parse SAFESPEED for fleet:" << name << endl;
      return false;
    }
  child = child->child;
  topSafeSpeed = atoi(child->value.text.string);

  child = mxmlFindElement(tree, tree, "LASTSPEED", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, could not parse LASTSPEED for fleet:" << name << endl;
      return false;
    }
  child = child->child;
  lastKnownSpeed = atoi(child->value.text.string);

  child = mxmlFindElement(tree, tree, "LASTDIRECTION", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, could not parse LASTDIRECTION for fleet:" << name << endl;
      return false;
    }
  child = child->child;
  lastKnownDirection = atoi(child->value.text.string);

  child = mxmlFindElement(tree, tree, "MASS", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, could not parse MASS for fleet:" << name << endl;
      return false;
    }
  child = child->child;
  mass = atoi(child->value.text.string);
  /*
  std::vector<Ship *> shipRoster;
  */
  child = mxmlFindElement(tree, tree, "SHIPLIST", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, could not parse SHIPLIST for fleet:" << name << endl;
      return false;
    }
  child = mxmlFindElement(child, child, "HULL", NULL, NULL, MXML_DESCEND);

  hasColonizer = false;
  isSpaceStation = false;
  maxPacketSpeed = 0;
  while( child )
    {
      Ship *readShip = new Ship();
     Ship *tempShip = new Ship();

      readShip->parseXML(child,gd);
     Ship *designShip = gd.getDesignByID(readShip->designhullID); //don't trust the player's copy
     //*tempShip = *designShip;
     *tempShip = *readShip;
     tempShip->objectId = readShip->objectId;
     if (!enemy)
         designShip->quantity += readShip->quantity; //so we know how many are existant this turn
     tempShip->quantity = readShip->quantity;
     tempShip->numberDamaged = readShip->numberDamaged;
     tempShip->percentDamaged = readShip->percentDamaged;
     tempShip->updateCostAndMass(gd);

     if (tempShip->maxPacketSpeed > maxPacketSpeed)
        maxPacketSpeed = tempShip->maxPacketSpeed;
      if (tempShip->hasColonizer)
        hasColonizer = true;
      if (tempShip->isSpaceStation)
        isSpaceStation = true;
      for (unsigned i=0;i<_numberOfMineTypes;i++)
        mineLayingRates[i] += tempShip->mineLayingRates[i];
      shipRoster.push_back(tempShip);
     delete readShip;
      child = mxmlFindElement(child, tree, "HULL", NULL, NULL, MXML_NO_DESCEND);
    }

  return true;
}

//clean up the fleet after battle
//the battle engine will set ship entries to 100% damage / zero ships
//if a battleboard token is destroyed.  This method goes back and removes
//ship slots if all ships in a stack are destroyed, and will delete the fleet
//if it has no ships remaining
void Fleet::clean(GameData &gameData)
{
    if (myType != gameObjectType_fleet) return;

    for (unsigned i = shipRoster.size(); i > 0;i--)
    {
         Ship *cShip = shipRoster[i-1];
         if (cShip->quantity == 0 || cShip->percentDamaged == FRACTIONAL_DAMAGE_DENOMINATOR)
        shipRoster.erase(shipRoster.begin() + (i-1));
    }

    if (shipRoster.size() == 0) //no ships left in fleet
        gameData.deleteFleet(objectId);
}

void Fleet::clearPrivateData(void)
{
	// inherited objects from GameObject
	name = "";

	// inherited objects from Placeable
	//coordinates; // visible
	cargoList.clear();
	penScanRadius = 0;
	nonPenScanRadius = 0;
	currentOrders.clear(); // shallow copy

	// local objects
	battleOrderID      = 0;
	totalCargoCapacity = 0;
	fuelCapacity       = 0;
	//mass; // visible
	maxPacketSpeed     = 0;
	//fleetID; // visible
	hasColonizer       = false;
	//isSpaceStation; // visible
	topSpeed           = 0;
	topSafeSpeed       = 0;
	//lastKnownSpeed; // visible
	//lastKnownDirection; // visible
	followDistance     = 0;

	for(unsigned j = 0; j < _numberOfMineTypes; ++j)
    mineLayingRates[j] = 0;

	for(unsigned i = shipRoster.size(); i > 0; --i)
		shipRoster[i-1]->clearPrivateData();
}

// updates mass, cargo space, fuel cap, safeSpeed, space station and packet status
void Fleet::updateFleetVariables(GameData &gameData)
{
	// these will be re-calculated
	isSpaceStation = false;
	fuelCapacity   = 0;
	maxPacketSpeed = 0;

	unsigned i; // MS bug
	for(i = 0; i < _numberOfMineTypes; ++i)
		mineLayingRates[i] = 0;

	unsigned massSum = 0;
	unsigned cargoSpace = 0;
	unsigned maxSafe = 10;

	for(i = 0; i < shipRoster.size(); ++i)
	{
		Ship *cShip = shipRoster[i];

		cShip->updateCostAndMass(gameData);

		for(unsigned i = 0; i < _numberOfMineTypes; ++i)
			mineLayingRates[i] += cShip->mineLayingRates[i];

		if( cShip->isSpaceStation )
			isSpaceStation = true;

                if (cShip->hasJumpGate)
                    hasJumpGame = true;

		if( maxPacketSpeed < cShip->maxPacketSpeed )
			maxPacketSpeed = cShip->maxPacketSpeed;

		massSum      += cShip->mass          * cShip->quantity;
		cargoSpace   += cShip->cargoCapacity * cShip->quantity;
		fuelCapacity += cShip->fuelCapacity  * cShip->quantity;

		if( cShip->warpEngine.maxSafeSpeed != 10 )
			maxSafe = 9;
	}

	mass = massSum + cargoList.getMass();

	if( getName().size() == 0 && shipRoster.size() )
	{
		std::ostringstream os;
		os << shipRoster[0]->getName() << " #" << objectId;
		setName(os.str());
	}

	totalCargoCapacity = cargoSpace;
	topSafeSpeed = maxSafe;
	topSpeed = 10;

	BattleOrder *bo;
	bo = gameData.getBattleOrderByID(battleOrderID);
	if( bo == NULL )
	{
		PlayerData *pd = gameData.getPlayerData(ownerID);
		if( pd->battleOrderList.size() == 0 )
			starsExit("Error, encountered empty battleOrderList",-1);

		battleOrderID = pd->battleOrderList[0]->objectId;
	}
}

void MassPacket::updateFleetVariables(GameData &gameData)
{
  Fleet::updateFleetVariables(gameData);
  //do nothing
}

void Fleet::toXMLString(string &theString) const
{
    theString = "<FLEET>\n";
    string temp;
    string parentData;
    PlaceableObject::toXMLStringGuts(parentData);

    theString += parentData;

    char tempString[2048];
    sprintf(tempString, "<BATTLE_ORDER_ID>%lu</BATTLE_ORDER_ID>\n",battleOrderID);
    theString += tempString;
    sprintf(tempString, "<FLEET_ID>%i</FLEET_ID>\n",fleetID);
    //cout<<"Processing fleet "<<fleetID<<" coord ("<<this->coordinates.getXDouble()<<","<<this->coordinates.getYdouble()<<")\n"; 
    theString += tempString;
    sprintf(tempString, "<CARGOCAPACITY>%i</CARGOCAPACITY>\n<FUELCAPACITY>%i</FUELCAPACITY>\n<TOPSPEED>%i</TOPSPEED>\n<SAFESPEED>%i</SAFESPEED>\n<LASTSPEED>%i</LASTSPEED>\n<LASTDIRECTION>%i</LASTDIRECTION>\n<MASS>%i</MASS>\n",
            totalCargoCapacity,fuelCapacity,topSpeed,topSafeSpeed,lastKnownSpeed,lastKnownDirection,mass);
    theString += tempString;
    theString += "<SHIPLIST>\n";
    for (unsigned u=0;u<shipRoster.size();u++)
      {
        shipRoster[u]->toXMLString(temp);
        theString += temp;
      }

    theString += "</SHIPLIST>\n";

    theString += "</FLEET>\n";
}

unsigned Fleet::getDockCapacity(void)
{
	unsigned temp = 0;

	// note that all this is redundant since currently space stations
	// are marked not fleetable -- but if ever they are not...
	for(unsigned i = 0; i < shipRoster.size(); ++i)
	{
		temp += shipRoster[i]->dockCapacity * shipRoster[i]->quantity;
	}
	return temp;
}

CargoManifest Fleet::scrapValue(const Race *whichRace, ScrapCase sc,
    unsigned *res) const
{
	// sum all the minerals in the fleet
	unsigned resTmp = 0;
	CargoManifest value;
//	string messageString="Fleet Scrap Data:\n";
//	string stemp;
//	char ctemp[4096];
//	sprintf(ctemp,"Fleet Scarp Data for race: %s fleet: %s\n",whichRace->singularName.c_str(),this->getName().c_str());
//	messageString = ctemp;
	for(size_t i = 0; i < shipRoster.size(); ++i)
	{
		Ship *ship = shipRoster[i];
//		ship->baseMineralCost.toCPPString(stemp);
//		sprintf(ctemp,"Adding a ship (%s) base:\n%s\nResource Cost:%d\nQuantity: %d\n",ship->getName().c_str(),stemp.c_str(),ship->baseResourceCost,ship->quantity);
//		messageString.append(ctemp);
		CargoManifest tValue = ship->baseMineralCost * ship->quantity;
		value = value + tValue;
		
		resTmp = resTmp + (ship->baseResourceCost * ship->quantity);
	}
	
//	cerr<<"Phase 1 scrap calculation\n"<<messageString;
	// scale for ultimate recycling and colonization
	bool ur = whichRace->lrtVector[ultimateRecycling_LRT] == 1;
//	if (ur) messageString+="UltimateRecycle LRT detected\n";
	switch( sc )
	{
	case SCRAP_COLONIZE:
		value = value * 75;
//		messageString+="Colonization Scrap: .75 \n";
		break;

	case SCRAP_STARBASE:
		if( ur )
		{
			value = value * 90;
			*res = (resTmp * 70) / 100;
		}
		else
			value = value * 80;
		break;

	case SCRAP_NO_STARBASE:
		if( ur )
		{
			value = value * 45;
			*res = (resTmp * 35) / 100;
		}
		else
			value = value * 33;
		break;
	}

	value = value / 100;

	return value;
}

// calculate total scanning power of the whole fleet
ScanComponent Fleet::getScan(const GameData &gameData) const
{
	ScanComponent ret; // return value

	const PlayerData *pd = gameData.getPlayerData(ownerID);
	// foreach ship in the fleet
	for(size_t i = 0; i < shipRoster.size(); ++i)
	{
		Ship *ship = shipRoster[i];
		ScanComponent scan = ship->getScan(*pd);
		if( scan.isActive )
		{
			ret.isActive = true;

			// pick the best elements from the fleet
			ret.baseScanRange = std::max(ret.baseScanRange, scan.baseScanRange);
			ret.penScanRange  = std::max(ret.penScanRange , scan.penScanRange);
		}
	}

	return ret;
}

bool Fleet::bombAction(GameData &gd)
{
	uint64_t hash = getX();
	hash = hash<<32;
	hash += getY();

	std::map<uint64_t, Planet*>::const_iterator plLoc =
	    gd.planetMap.find(hash);
	Planet *pl = NULL;
	if( plLoc != gd.planetMap.end() )
		pl = gd.planetMap[hash];

	if( pl == NULL )
		return true;

	PlayerData *pd = gd.getPlayerData(ownerID);

	std::map<unsigned, PlayerRelation>::const_iterator i =
		pd->playerRelationMap_.find(pl->ownerID);
	// allied players don't bomb each other
	//Ned? I think you don't bomb neutrals...
	if( i == pd->playerRelationMap_.end() )
	{
		pd->playerRelationMap_[pl->ownerID] = PR_NEUTRAL;
		return true;
	}

	if( i->second != PR_ENEMY )
 //if( i->second == PR_FRIEND )
		return true;

	pl->bombingList.push_back(this);

	return true;
}

//----------------------------------------------------------------------------
ShipSlot::ShipSlot(void)
{
	structuralElementCategory i;
	for(i = (structuralElementCategory)0;
	    i < numberOf_ELEMENT;
	    i = (structuralElementCategory)(i+1))
	{
		acceptableElements[i] = false;
	}
	numberPopulated = 0;
	maxItems = 0;
	slotID = 0;
}

//----------------------------------------------------------------------------
// loop through and read in all the Hulls, and store them in the hullList
bool parseHulls(mxml_node_t *tree, vector<Ship*> &list, GameData &gd)
{
  mxml_node_t *child = mxmlFindElement(tree, tree, "HULL", NULL,
                                       NULL, MXML_DESCEND);
  if( !child )
    return false;

  while( child )
    {
      Ship *tempShip = new Ship();
      tempShip->parseXML(child, gd);
      list.push_back(tempShip);

      child = mxmlFindElement(child, tree, "HULL", NULL, NULL, MXML_NO_DESCEND);
    }

  return true;
}

// loop through and read in all the Fleets, and store them in the fleetList
// we leave this list unsorted, since at this point I don't know what sort
// field would make the most sense.
bool parseFleets(mxml_node_t *tree, vector<Fleet*> &list, GameData &gd, bool enemy)
{
  mxml_node_t *child = mxmlFindElement(tree, tree, "FLEET", NULL,
                                       NULL, MXML_DESCEND);
  if( !child )
    return true;

  while( child )
    {
      Fleet *tempFleet = new Fleet();
      tempFleet->parseXML(child, gd, enemy);
      if (tempFleet->myType == gameObjectType_packet) //are we a mineral packet?
        {
          MassPacket *pack = new MassPacket(tempFleet);
          list.push_back(pack);
          delete tempFleet;
        }
     else if (tempFleet->myType == gameObjectType_debris) //are we a debris field?
        {
          Debris *df = new Debris(tempFleet);
          list.push_back(df);
          delete tempFleet;
        }

      else
        list.push_back(tempFleet);

      child = mxmlFindElement(child, tree, "FLEET", NULL, NULL, MXML_NO_DESCEND);
    }

  return true;
}

bool parseBattleOrders(mxml_node_t *tree, vector<BattleOrder*> &list, GameData &gd)
{
  mxml_node_t *const head = mxmlFindElement(tree, tree, "BATTLE_ORDER_LIST", NULL,
                                       NULL, MXML_DESCEND);
  mxml_node_t *child = NULL;
  if (head !=NULL)
  {
      child = mxmlFindElement(head->child, head, "BATTLE_ORDER", NULL,
                                       NULL, MXML_NO_DESCEND);
  }
  if( !child )
  {
    return true;
  }

  while( child )
    {
      BattleOrder *tempOrder = new BattleOrder();
      tempOrder->parseXML(child);
      list.push_back(tempOrder);

      child = mxmlFindElement(child, head, "BATTLE_ORDER", NULL, NULL, MXML_NO_DESCEND);
    }

  return true;
}

bool parseMineFields(mxml_node_t *tree, vector<Minefield*> &list, GameData &gd)
{
  mxml_node_t *child = mxmlFindElement(tree, tree, "MINEFIELD", NULL,
                                       NULL, MXML_NO_DESCEND);
  if( !child )
    return true;

  while( child )
    {
      Minefield *tempField = new Minefield();
      tempField->parseXML(child->child);
      list.push_back(tempField);
      child = mxmlFindElement(child, tree, "MINEFIELD", NULL, NULL, MXML_NO_DESCEND);
    }

  return true;
}

void fleetsToXMLString(vector<Fleet*> list, string &theString)
{
  theString = "";

  string tempString = "";
  for(unsigned i = 0; i < list.size(); i++)
    {
      Fleet *tempFleet = list[i];
      tempFleet->toXMLString(tempString);
      theString += tempString;
    }
}

void minefieldsToXMLString(vector<Minefield*> list, string &theString)
{
	theString = "";

	string tempString;
	for(unsigned i = 0; i < list.size(); i++)
	{
		Minefield *tempField = list[i];
		tempField->toXMLStringGuts(tempString);
		theString += tempString;
	}
}

//----------------------------------------------------------------------------
MassPacket::MassPacket(Fleet *flt)
{
  fuelCapacity = 0;
  lastKnownDirection = flt->lastKnownDirection;
  lastKnownSpeed = flt->lastKnownSpeed;
  mass = flt->mass;
  shipRoster.clear();
  topSafeSpeed = flt->topSafeSpeed;
  topSpeed = flt->topSpeed;
  totalCargoCapacity = flt->totalCargoCapacity;
  cargoList = flt->cargoList;
  coordinates = flt->coordinates;
  currentOrders = flt->currentOrders;
  name = flt->getName();
  objectId = flt->objectId;
  ownerID = flt->ownerID;
  myType = gameObjectType_packet;
}

void MassPacket::toXMLString(string &theString) const
{
    theString = "";
    if (mass == 0) return;
    Fleet::toXMLString(theString);
}

bool MassPacket::actionHandler(GameData &gd, const Action &what)
{
	bool tempResult = Fleet::actionHandler(gd,what);
	if( what.ID == fleetMove_action )
	{
	 //PlayerData *ownerData = gd.getPlayerData(ownerID);

		uint64_t targetID = currentOrders[0]->targetID;

		// cause decay
		unsigned packetDecayRate = 1; //units per 100

		// foreach cargo type
		for(unsigned i = 0; i < _numCargoTypes; ++i)
		{
			// how much is there
			const unsigned amt = cargoList.cargoDetail[i].amount;

			// how much gets lost
			unsigned decayAmount = amt/100 * packetDecayRate;
			if( decayAmount == 0 && amt > 0 )
				decayAmount = 1;

			cargoList.cargoDetail[i].amount = amt - decayAmount;
		}

		if( currentOrders.size() == 0 ) // we arrived this turn
		{
			Planet *targetPlanet = gd.getPlanetByID(targetID);
			if( targetPlanet == NULL )
			{
				std::string tempString;
				toXMLString(tempString);
				std::cerr << "Error, packet struck non-existant planet objectID("
				    << targetID << ")\n" << tempString;
				starsExit("fleet.cpp", -1);
			}

			targetPlanet->cargoList = targetPlanet->cargoList + cargoList;
			cargoList = cargoList - cargoList;
//deletes happen at xml gen time now...
//          gd.deleteFleet(objectId);
		}
	}
	return tempResult;
}

//----------------------------------------------------------------------------
Debris::Debris(Fleet *flt)
{
	myType             = gameObjectType_debris;

	fuelCapacity       = 0;

	lastKnownDirection = flt->lastKnownDirection;
	lastKnownSpeed     = flt->lastKnownSpeed;
	mass               = flt->mass;
	topSafeSpeed       = flt->topSafeSpeed;
	topSpeed           = flt->topSpeed;
	totalCargoCapacity = flt->totalCargoCapacity;
	cargoList          = flt->cargoList;
	coordinates        = flt->coordinates;
	currentOrders      = flt->currentOrders;
	name               = flt->getName();
	objectId           = flt->objectId;
	ownerID            = flt->ownerID;

	shipRoster.clear();
}

Debris::Debris(unsigned owner, Location place, CargoManifest stuff)
{
	name        = "Debris";
	objectId    = GameObject::nextObjectId;
	++GameObject::nextObjectId;

	ownerID     = owner;
	cargoList   = stuff;
	coordinates = place;
	mass        = stuff.getMass();
}

void Debris::toXMLString(std::string &theString) const
{
	if( mass == 0 )
	{
		theString = "";
		return;
	}

	Fleet::toXMLString(theString);
}

//----------------------------------------------------------------------------
BattleOrder::BattleOrder(void)
{
	attackNeutralsAndEnemies = attackEnemies = true;
	attackEveryone = attackNone = attackSpecificPlayer = false;
	targetPlayer = 0;
	primaryTarget = _anyTarget;
	secondaryTarget = _anyTarget;
	tactics = _maximizeDamageRatioTactic;
	dumpCargo = false;
}

bool BattleOrder::parseXML(mxml_node_t* tree)
{
	if( !GameObject::parseXML(tree) )
		return false;

	unsigned temp = 0;
	bool rVal = true;

	rVal &= parseXML_noFail(tree, "PRIMARY_TARGET", temp);
	primaryTarget = (TargetType)temp;

	rVal &= parseXML_noFail(tree, "SECONDARY_TARGET", temp);
	secondaryTarget = (TargetType)temp;

	rVal &= parseXML_noFail(tree, "TACTICS", temp);
	tactics = (TacticType)temp;

	rVal &= parseXML_noFail(tree, "ATTACK_NONE", attackNone);
	rVal &= parseXML_noFail(tree, "ATTACK_ENEMIES", attackEnemies);
	rVal &= parseXML_noFail(tree, "ATTACK_NEUTRALS_AND_ENEMIES",
	    attackNeutralsAndEnemies);
	rVal &= parseXML_noFail(tree, "ATTACK_EVERYONE", attackEveryone);
	rVal &= parseXML_noFail(tree, "ATTACK_SPECIFIC_PLAYER",
	    attackSpecificPlayer);
	rVal &= parseXML_noFail(tree, "ATTACK_PLAYER_ID", targetPlayer);
	rVal &= parseXML_noFail(tree, "DUMP_CARGO", dumpCargo);

	return rVal;
}

void BattleOrder::toXMLString(string &theString) const
{
    char cTemp[4096];
    string tempString = "";
    theString = "<BATTLE_ORDER>\n";
    GameObject::toXMLStringGuts(tempString);
    theString += tempString;
    sprintf(cTemp,"<PRIMARY_TARGET>%i</PRIMARY_TARGET>\n<SECONDARY_TARGET>%i</SECONDARY_TARGET>\n",primaryTarget,secondaryTarget);
    theString+=cTemp;
    unsigned aNone = attackNone?1:0;
    unsigned aEnem = attackEnemies?1:0;
    unsigned aN_En = attackNeutralsAndEnemies?1:0;
    unsigned aAll  = attackEveryone?1:0;
    unsigned aSpec = attackSpecificPlayer?1:0;
    sprintf(cTemp,"<TACTICS>%i</TACTICS>\n<ATTACK_NONE>%i</ATTACK_NONE>\n<ATTACK_ENEMIES>%i</ATTACK_ENEMIES>\n<ATTACK_NEUTRALS_AND_ENEMIES>%i</ATTACK_NEUTRALS_AND_ENEMIES>\n<ATTACK_EVERYONE>%i</ATTACK_EVERYONE>\n<ATTACK_SPECIFIC_PLAYER>%i</ATTACK_SPECIFIC_PLAYER>\n<ATTACK_PLAYER_ID>%i</ATTACK_PLAYER_ID>\n",tactics,aNone,aEnem,aN_En,aAll,aSpec,targetPlayer);
    theString += cTemp;
    sprintf(cTemp,"<DUMP_CARGO>%i</DUMP_CARGO>\n",dumpCargo);
    theString += cTemp;
    theString += "</BATTLE_ORDER>\n";
}

