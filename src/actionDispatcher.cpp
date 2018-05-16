#ifdef MSVC
#pragma warning(disable: 4786)
#endif
//
// C++ Implementation: turnGen
//
// Copyright: See COPYING file that comes with this distribution
//
#include "universe.h"
#include "newStars.h"
#include "actionDispatcher.h"
#include "planet.h"

#include <set>
#include <assert.h>

//----------------------------------------------------------------------------
/// helper for calcScan
/// make player 'pd' see 'fleet'
/// (the knownDesigns set gives O(1) search of the known designs list)
static void seeFleet(Fleet *fleet, std::set<Ship*> *knownDesigns, PlayerData *pd, GameData *gd_)
{
	// push a clean copy into the player's detected fleet list
	Fleet *fleetCopy = new Fleet(*fleet);
	fleetCopy->clearPrivateData();
	pd->detectedEnemyFleetList.push_back(fleetCopy);

	//foreach ship type in the fleet
	for(size_t si = 0; si < fleet->shipRoster.size(); ++si)
	{
		Ship *ship = fleet->shipRoster[si];
		Ship *design = gd_->getDesignByID(ship->designhullID);

		// check the known designs set for this design
		if( knownDesigns->find(design) == knownDesigns->end() )
		{
			// add a copy of the design to the known designs list
			Ship *scanDesign = new Ship(*design);
			scanDesign->clearDesignData();
			pd->knownEnemyDesignList.push_back(scanDesign);
			knownDesigns->insert(design); // update the set
		}
	}

	// if player has not seen the owning player before now
	if( fleet->ownerID != 0 && !pd->hasSeenPlayer(fleet->ownerID) )
		pd->seePlayer(*(gd_->getRaceById(fleet->ownerID)));
}

//----------------------------------------------------------------------------
ActionDispatcher::ActionDispatcher(GameData *gd)
{
	gd_ = gd;
}

/// clear the working variables that propagate data through
/// the turn sequence as required
void ActionDispatcher::clearWorkingSet(void)
{
	//foreach player
	for(unsigned pi = 0; pi < gd_->playerList.size(); ++pi)
	{
		PlayerData *player = gd_->playerList[pi];

		// currently, only data accumulated is research
		player->resourcesAllocatedToResearch = 0;
	}
}

/// recalculate everything and get ready for outputting data
void ActionDispatcher::cleanEndOfTurn(void)
{
	//foreach player
	for(unsigned playerIndex = 0; playerIndex < gd_->playerList.size(); ++playerIndex)
	{
		PlayerData *player = gd_->playerList[playerIndex];

		// update the player's terraform tech
		player->race->updateTerraformAbility(*gd_);
	}

	planetaryUpdateVariables();
	fleetUpdateFuel();
}

/// calculate which planets can be seen per each player
void ActionDispatcher::calcScan(void)
{
	//foreach player
	for(unsigned player = 0; player < gd_->playerList.size(); ++player)
	{
		PlayerData *pd = gd_->playerList[player];
		const unsigned pyId = pd->race->ownerID; // player id

		// rebuild the detected fleet list (Ned, leak?)
		pd->detectedEnemyFleetList.clear();

		//---build planet bins
		std::vector<Planet*> myPlanets;
		std::set   <Planet*> unscannedPlanets;

		//foreach planet
		for(size_t i = 0; i < gd_->planetList.size(); ++i)
		{
			Planet *planet = gd_->planetList[i];

			// if my planet
			if( pyId == planet->ownerID )
			{
				myPlanets.push_back(planet);

				// data is current
				pd->getPlayerPlanet(planet)->yearDataGathered =
				    gd_->theUniverse->gameYear;
			}
			else // will try to scan it
				unscannedPlanets.insert(planet);
		}

		//---fleet and ship detection
		std::set<Ship*> knownDesigns;
		for(size_t ed = 0; ed < pd->knownEnemyDesignList.size(); ++ed)
			knownDesigns.insert(gd_->getDesignByID(pd->knownEnemyDesignList[ed]->designhullID));

		std::set<Fleet*> unscannedFleets;
		// foreach fleet
		for(size_t fid = 0; fid < gd_->fleetList.size(); ++fid)
		{
			Fleet *f = gd_->fleetList[fid];
			if( f->ownerID != pyId )
				unscannedFleets.insert(f);
		}

		//---and now, the scanning!

		//Ned, should be handled better...
		// handle NAS by making a copy of the scanner and doubling locally
		ScanComponent planScan = pd->getBestPlanetScan()->scanner;
		if( pd->race->lrtVector[noAdvancedScanners_LRT] == 1 )
			planScan.baseScanRange *= 2;

		const bool havePlanPen = planScan.penScanRange > 0;

		// find what my planets can see
		for(size_t j = 0; j < myPlanets.size(); ++j)
		{
			Planet *myPlan = myPlanets[j];

			// check for enemy fleets
			for(std::set<Fleet*>::iterator fi = unscannedFleets.begin();
				fi != unscannedFleets.end(); )
			{
				const unsigned dist =
				    (**fi).coordinates.distanceFrom(myPlan->coordinates);

				// is it hiding behind a planet?
				const uint64_t fleetHash = formXYhash((**fi).getX(), (**fi).getY());
				std::map<uint64_t, Planet*>::const_iterator plLoc =
				    gd_->planetMap.find(fleetHash);

				// maybe normal scan can pick it up
				unsigned effScan = planScan.baseScanRange;
				bool     inOrbit = false; // orbiting scanning planet?

				// if at planet
				if( plLoc != gd_->planetMap.end() )
				{
					// if planet is us, we will see it
					if( plLoc->second->objectId == myPlan->objectId )
						inOrbit = true;
					else // not us
						effScan = planScan.penScanRange; // need pen scan
				}

				if( inOrbit || dist <= effScan )
				{
					// spotted 'em!
					seeFleet(*fi, &knownDesigns, pd, gd_);

					unscannedFleets.erase(fi++);
				}
				else
					++fi;
			}

			if( havePlanPen )
			{
				// check planet pen to planets
				for(std::set<Planet*>::iterator i = unscannedPlanets.begin();
				    i != unscannedPlanets.end();)
				{
					Planet *scanPlan = *i;

					const unsigned dist =
					    scanPlan->coordinates.distanceFrom(myPlan->coordinates);
					if( dist <= planScan.penScanRange )
					{
						scanPlan->yearDataGathered = gd_->theUniverse->gameYear;

						unscannedPlanets.erase(i++);
					}
					else
						++i;
				}
			}
		} //foreach of my planets -> scan

		// planets with "no scan" fleets in orbit (notify player of their oops)
		std::set<Planet*> oopsPlanets;

		// calculate what my fleets can see
		for(size_t k = 0; k < pd->fleetList.size(); ++k)
		{
			Fleet *fleet = pd->fleetList[k];

			const uint64_t fleetHash = formXYhash(fleet->getX(), fleet->getY());
			std::map<uint64_t, Planet*>::const_iterator plLoc =
			    gd_->planetMap.find(fleetHash);
			const bool atPlanet = plLoc != gd_->planetMap.end();

			// check for picking up player info
			if( atPlanet && plLoc->second->ownerID != 0 &&
			    !pd->hasSeenPlayer(plLoc->second->ownerID) )
			{
				pd->seePlayer(*(gd_->getRaceById(plLoc->second->ownerID)));
			}

			//Ned, NAS?
			ScanComponent fleetScan = fleet->getScan(*gd_);
			assert( fleetScan.isActive ||
			        (fleetScan.baseScanRange == 0 && fleetScan.penScanRange == 0) );

			// check for enemy fleets seen by my fleet (includes same x,y)
			for(std::set<Fleet*>::iterator fi = unscannedFleets.begin();
				fi != unscannedFleets.end(); )
			{
				const unsigned dist =
				    (**fi).coordinates.distanceFrom(fleet->coordinates);

				// what is our scanning range to this fleet
				unsigned effScan = fleetScan.baseScanRange;

				// is fleet behind planet? need pen scan
				uint64_t fleetHash = formXYhash((**fi).getX(), (**fi).getY());
				std::map<uint64_t, Planet*>::const_iterator plLoc =
				    gd_->planetMap.find(fleetHash);
				if( plLoc != gd_->planetMap.end() )
					effScan = fleetScan.penScanRange;

				if( dist <= effScan )
				{
					seeFleet(*fi, &knownDesigns, pd, gd_);

					unscannedFleets.erase(fi++);
				}
				else
					++fi;
			}

			if( !fleetScan.isActive ) // if fleet has no scanner
			{
				if( atPlanet &&
				    unscannedPlanets.find(plLoc->second) != unscannedPlanets.end() )
				{
					// we're at an unscanned planet

					oopsPlanets.insert(plLoc->second); // tag it for player message
				}
				continue;
			}
			//else fleet has a scanner

			if( fleetScan.penScanRange == 0 ) // but it has no pen
			{
				// can only detect planets by being there
				if( atPlanet )
				{
					std::set<Planet*>::iterator unscanI = unscannedPlanets.find(plLoc->second);
					if( unscanI == unscannedPlanets.end() )
						continue;

					Planet *playerPlanet = pd->getPlayerPlanet(plLoc->second);
					*playerPlanet = *plLoc->second;
					playerPlanet->clearScanData();
					playerPlanet->yearDataGathered = gd_->theUniverse->gameYear;

					unscannedPlanets.erase(unscanI);
				}
				continue;
			}
			//else check fleet to planet pen scan
			for(std::set<Planet*>::iterator i = unscannedPlanets.begin();
			    i != unscannedPlanets.end();)
			{
				Planet *planet = *i;

				const unsigned dist =
				   fleet->coordinates.distanceFrom(planet->coordinates);

				if( fleetScan.penScanRange >= dist )
				{
					// check for picking up player info
					if( planet->ownerID != 0 && !pd->hasSeenPlayer(planet->ownerID) )
						pd->seePlayer(*(gd_->getRaceById(planet->ownerID)));

					Planet *playerPlanet = pd->getPlayerPlanet(planet);
					*playerPlanet = *planet;
					playerPlanet->clearScanData();
					playerPlanet->yearDataGathered = gd_->theUniverse->gameYear;

					unscannedPlanets.erase(i++);
				}
				else
				{
					if( fleetScan.baseScanRange >= dist )
					{
						// check for picking up player info
						if( planet->ownerID != 0 && !pd->hasSeenPlayer(planet->ownerID) )
							pd->seePlayer(*(gd_->getRaceById(planet->ownerID)));
					}

					++i;
				}
			}
		}
	}
}

void ActionDispatcher::scrapFleets(void)
{
	const Action a(scrapFleet_action, "Scrap Fleet Action");

	bool someDeletes = false;
	for(size_t i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];

		if( f->actionHandler(*gd_, a) )
			someDeletes = true;
	}

	if( someDeletes )
		gd_->processFleetDeletes();
}

// call all the individual action dispatchers in the correct order as
// specified by the game mechanics
//Ned? where is concentration decay due to mining?
void ActionDispatcher::dispatchAll(void)
{
	clearWorkingSet();
	scrapFleets();

	// waypoint 0 GUI based load tasks are first
	waypointUnload(true);
	waypointLoad  (true);

	// now unloads inc. colonization, all at once gui/tile based
	waypointUnload(false);
	waypointColonize();
	resolveGroundCombat();

	// moves
	packetMove();
	fleetMove();

	// planet update
	planetaryUpdateVariables();
	planetaryExtractMineralWealth();
	planetaryProcessBuildQueue();

	performResearch();
	stealResearch();

	planetaryPopulationGrowth();

	// battle
	fleetBattles();
	bombingRuns();

	waypointUnload(false);
	waypointColonize();
	resolveGroundCombat();
	waypointLoad(false);

	mineLayingPhase(1);
	decayMinefields();

	cleanEndOfTurn();

	gd_->incYear();

	calcScan();
}

/// population growth for all planets
void ActionDispatcher::planetaryPopulationGrowth(void)
{
	const Action a(planetaryPopulationGrowth_action, "Planetary Growth Action");

	for(unsigned i = 0; i < gd_->planetList.size(); ++i)
	{
		Planet *p = gd_->planetList[i];
		if( p->ownerID != 0 )
			p->actionHandler(*gd_, a);
	}
}

/// fleet move for all fleets
void ActionDispatcher::fleetMove(void)
{
	Action a(fleetMove_action, "Fleet Move Action");
	unsigned i; // MS bug

	// build a list of fleets with follow orders
	std::vector<Fleet*> followList;
	for(i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];
		if( f->currentOrders.size() != 0 )
		{
			Order *ord = f->currentOrders[0];
			if( ord->myOrderID == follow_fleet_order )
			{
				followList.push_back(f);
			}
		}
	}

	// process move normally
	for(i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];
		f->actionHandler(*gd_, a);
	}

	// now, process fleet follows
	a.ID = follow_fleet_action;

	// assume all fleets are following each other
	// go straight to phased resolution
	for(unsigned phase = 1; phase <= 100; ++phase)
	for(unsigned i = 0; i < followList.size(); ++i)
	{
		gd_->followPhase = phase;
		Fleet *f = followList[i];

		// This was a bug.  This would stop fleet A from following fleet B if it caught up
		// but if fleetB later moved (say it was following something) Fleet A would be left behind
//		if (!moveIgnoreMap[f])              //if false, send it a follow action
//		if( !f->actionHandler(*gd_, a) )
//			moveIgnoreMap[f] = true;    //reached destination, no more follow actions
		f->actionHandler(*gd_,a);
	}

	for(i = 0; i < gd_->fleetList.size(); i++)
	{
		Fleet *f = gd_->fleetList[i];
		f->coordinates.finalizeLocation();
	}

	gd_->processFleetDeletes(); // fleets may have been destroyed, packets caught, etc.
}

void ActionDispatcher::packetMove(void)
{
	const Action a(fleetMove_action, "Fleet Move Action");

	for(size_t i = 0; i < gd_->packetList.size(); ++i)
	{
		Fleet *f = gd_->packetList[i];
		f->actionHandler(*gd_, a);
	}
}

void ActionDispatcher::waypointColonize(void)
{
	const Action a(colonize_action, "Colonization Action");

	for(size_t i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];
		f->actionHandler(*gd_, a);
	}

	gd_->processFleetDeletes();
}

void ActionDispatcher::performResearch(void)
{
	for(unsigned i = 0; i < gd_->playerList.size(); ++i)
		gd_->playerList[i]->normalResearch();
}

void ActionDispatcher::stealResearch(void)
{
	for(unsigned i = 0; i < gd_->playerList.size(); ++i)
		gd_->playerList[i]->stealResearch();
}

void ActionDispatcher::fleetUpdateFuel(void)
{
	//foreach fleet in the game
	for(size_t i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];

		//Ned, fuel exporters...

		// check for star base refueling
		uint64_t hash = formXYhash(f->getX(), f->getY());
		std::map<uint64_t, Planet*>::const_iterator pi = gd_->planetMap.find(hash);
		if( pi == gd_->planetMap.end() ) // not at a planet
			continue;

		uint64_t sid = pi->second->starBaseID;
		if( sid == 0 ) // no starbase
			continue;

		Fleet *sbfleet = gd_->getFleetByID(sid);
		if( !sbfleet ) // shouldn't fail here...
			continue;

		// only sb that can build can refuel
		if( sbfleet->getDockCapacity() == 0 ) // check for build cap
			continue;

		f->cargoList.cargoDetail[_fuel].amount = f->fuelCapacity;
	}
}

void ActionDispatcher::planetaryUpdateVariables(void)
{
	const Action a(planetaryUpdatePlanetVariables_action, "Update planet's variable data (resources, max infra, etc)");

	for(unsigned i = 0; i < gd_->planetList.size(); ++i)
	{
		Planet *p = gd_->planetList[i];
		if( p->ownerID != 0 )
			p->actionHandler(*gd_, a);
	}
}

void ActionDispatcher::planetaryExtractMineralWealth(void)
{
	const Action a(planetaryExtractMineralWealth_action, "Extract planet's minerals(mining)");

	for(unsigned i = 0; i < gd_->planetList.size(); ++i)
	{
		Planet *p = gd_->planetList[i];
		if( p->ownerID != 0 )
			p->actionHandler(*gd_, a);
	}
}

void ActionDispatcher::planetaryProcessBuildQueue(void)
{
	const Action a(planetaryProcessBuildQueue_action, "Planetary build queue processing");

	for(unsigned i = 0; i < gd_->planetList.size(); ++i)
	{
		Planet *p = gd_->planetList[i];
		if( p->ownerID != 0 )
			p->actionHandler(*gd_, a);
	}
}

void ActionDispatcher::resolveGroundCombat(void)
{
	const Action a(resolve_ground_combat_action, "Resolve all pending ground combat and colonize orders");

	for(unsigned i = 0; i < gd_->planetList.size(); ++i)
	{
		Planet *p = gd_->planetList[i];
		p->actionHandler(*gd_, a);
	}
}

/// there are two phases of minefields
void ActionDispatcher::mineLayingPhase(int phase)
{
	Action a(lay_mines_action, "Lay Minefields");
	if( phase == 0 )
		a.ID = lay_mines_before_move_action;

	for(unsigned i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];
		f->actionHandler(*gd_, a);
	}
}

void ActionDispatcher::decayMinefields(void)
{
	const Action a(decay_mines_action, "Decay MF");

	for(unsigned i = 0; i < gd_->minefieldList.size(); ++i)
	{
		Minefield *mf = gd_->minefieldList[i];
		mf->actionHandler(*gd_, a);
	}
}

void ActionDispatcher::fleetBattles(void)
{
	unsigned i; // MS bug

	std::map<uint64_t,std::vector<Fleet*>*> fleetLocationMap;
	std::vector<uint64_t> locationVector; // a list of keys in the map, Ned? use iterator?

	// hash all the fleet locations, build vectors of fleets at all occupied locations
	for(i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet* cf = gd_->fleetList[i];
		// minefields, debris, and packets can live in the fleet list
		if( cf->myType == gameObjectType_fleet )
		{
			const uint64_t hash = formXYhash(cf->getX(), cf->getY());
			std::vector<Fleet*> *localFleetVector = fleetLocationMap[hash];
			if( !localFleetVector )
			{
				localFleetVector = new std::vector<Fleet*>;
				fleetLocationMap[hash] = localFleetVector;
				locationVector.push_back(hash);
			}
			localFleetVector->push_back(cf);
		}
	}

	// process location vector to perform fleet battles
	for(i = 0; i < locationVector.size(); ++i)
	{
		std::vector<Fleet*> *localFleetVector = fleetLocationMap[locationVector[i]];
		if( localFleetVector->size() > 1 )
			gd_->processBattle(*localFleetVector, *gd_);

		for(unsigned u = 0; u < localFleetVector->size(); ++u)
		{
			Fleet *cf = (*localFleetVector)[u];
			cf->clean(*gd_);
		}
	}

	gd_->processFleetDeletes(); // possible fleet destruction
}

void ActionDispatcher::bombingRuns(void)
{
	Action a(standard_bomb_planet_action, "Bomb -- queue fleets to planets"); // avoid loop overhead

	// bombing runs are resolved player by player, normal bombs first, then smart bombs
	for(unsigned playerIndex = 0; playerIndex < gd_->playerList.size(); ++playerIndex)
	{
		PlayerData *pd = gd_->playerList[playerIndex];
		const unsigned pNum = pd->race->ownerID;

		for(unsigned fleetIndex = 0; fleetIndex < gd_->fleetList.size(); ++fleetIndex)
		{
			a.ID = standard_bomb_planet_action;
			a.description = "Bomb -- queue fleets to planets";

			Fleet *flt = gd_->fleetList[fleetIndex];
			if( flt->ownerID == pNum )
				flt->actionHandler(*gd_, a);
		}

		for(unsigned planetIndex = 0; planetIndex < gd_->planetList.size(); ++planetIndex)
		{
			a.ID = standard_bomb_planet_action;
			a.description = "Standard Bombing Run";

			Planet *pl = gd_->planetList[planetIndex];
			pl->actionHandler(*gd_,a);

			a.ID = smart_bomb_planet_action;
			a.description = "Smart Bombing Run";
			pl->actionHandler(*gd_,a);

			// now clear the bombing list of this players ships
			pl->bombingList.clear();
		}
	}
}

void ActionDispatcher::waypointLoad(bool guiBased)
{
	Action a(load_cargo_action, "Load Cargo Action");
	if( guiBased )
		a.ID = gui_load_action;

	unsigned i; // MS bug
	for(i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];
		f->actionHandler(*gd_, a);
	}

	for(i = 0; i < gd_->planetList.size(); ++i)
	{
		Planet *p = gd_->planetList[i];
		if( p->ownerID != 0 )
			p->actionHandler(*gd_, a);
	}
}

void ActionDispatcher::waypointUnload(bool guiBased)
{
	Action a(unload_cargo_action, "Unload Cargo Action");
	if( guiBased )
		a.ID = gui_unload_action;

	unsigned i; // MS bug
	for(i = 0; i < gd_->fleetList.size(); ++i)
	{
		Fleet *f = gd_->fleetList[i];
		f->actionHandler(*gd_, a);
	}

	for(i = 0; i < gd_->planetList.size(); ++i)
	{
		Planet *p = gd_->planetList[i];
		if( p->ownerID != 0 )
			p->actionHandler(*gd_, a);
	}
}

