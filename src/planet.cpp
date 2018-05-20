#ifdef MSVC
#pragma warning(disable: 4786)
#endif
/*****************************************************************************
                          planet.cpp -  description
                             -------------------
    begin                : 08 Jul 2004
    copyright            : (C) 2004-2007
                           All Rights Reserved.
 ****************************************************************************/

/*****************************************************************************
 This program is copyrighted, and may not be reproduced in whole or
 part without the express permission of the copyright holders.
 *****************************************************************************/
#include "planet.h"
#include "universe.h"
#include "newStars.h"
#include "classDef.h"
#include "utils.h"
#include "race.h"

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <sstream>

//----------------------------------------------------------------------------
// fractional amount of build completion we track (1000 = 1%)
const unsigned fractionalCompletionDenominator = 100000;

//----------------------------------------------------------------------------
// cycle through all planets and convert them into one XML formatted string
void planetsToXMLString(const vector<Planet*> &list, string &theString)
{
	theString = "";
	std::string ts;
	for(size_t i = 0; i < list.size(); ++i)
	{
		list[i]->toXMLString(ts);
		theString += ts;
	}
}

//----------------------------------------------------------------------------
Planet::Planet(void)
{
	myType = gameObjectType_planet;

	popRemainder_              = 0;
	currentResourceOutput      = 0; // resource output of the planet (calculated)
	resourcesRemainingThisTurn = 0;
	packetTarget               = 0; // objectID of any created packets
	routeTarget                = 0; // objectID of any route command target
	packetSpeed                = 0;
	yearDataGathered           = 0;
	starBaseID                 = 0; // ObjectID of this object's starbase fleet
	isHomeworld_               = false;
}

bool Planet::parseXML(mxml_node_t *tree)
{
	// parse parent class
	PlaceableObject::parseXML(tree);

	mxml_node_t *child = mxmlFindElement(tree, tree, "HAB", NULL, NULL, MXML_DESCEND);
	if( !child )
	{
		std::cerr << "Error, could not locate HAB keyword\n";
		return false;
	}
	child = child->child;

	// six for host file and three for universe def file
	int valArray[16];
	int valCounter = assembleNodeArray(child, valArray, 6);
	if( valCounter != 6 && valCounter != 3 )
	{
		cerr << "Error, expected 6 or 3 hab values for planet " << name
		     << ", found " << valCounter << '\n';
		return false;
	}

	originalEnvironment.grav = valArray[0];
	originalEnvironment.temp = valArray[1];
	originalEnvironment.rad  = valArray[2];
	if( valCounter == 6 )
	{
		// normal turn file
		currentEnvironment.grav = valArray[3];
		currentEnvironment.temp = valArray[4];
		currentEnvironment.rad  = valArray[5];
	}
	else // initial game definition, just copy
	{
		currentEnvironment = originalEnvironment;
	}

	// last of the data that _must_ be present
	child = mxmlFindElement(tree, tree, "INFRASTRUCTURE", NULL, NULL, MXML_DESCEND);
	if( !child )
	{
		cerr << "Error parsing file, expected INFRASTRUCTURE data for planet "
		     << name << endl;
		starsExit("planet.cpp", -1);
	}
	child = child->child;

	valCounter = assembleNodeArray(child, valArray, 4);
	if( valCounter != 4 )
	{
		cerr << "Error, expected 4 INFRASTRUCTURE DATA points for planet "
		     << name << ", found " << valCounter << '\n';
		return false;
	}

	infrastructure.mines     = valArray[0];
	infrastructure.factories = valArray[1];
	infrastructure.defenses  = valArray[2];
	infrastructure.scanner   = valArray[3];

	if( !mineralConcentrations.parseXML(tree, "MINERALCONCENTRATIONS") )
	{
		cerr << "Error parsing mineral concentrations for planet: "
		     << name << '\n';
		return false;
	}

	parseXML_noFail(tree, "POP_REMAINDER", popRemainder_);
	parseXML_noFail(tree, "RESOURCESGENERATED", currentResourceOutput);

	if( !parseXML_noFail(tree, "PACKETTARGET", packetTarget) )
	{
		cerr << "Error, could not parse node: PACKETTARGET for planet "
		     << name << '\n';
		starsExit("planet.cpp", -1);
	}

	if( !parseXML_noFail(tree, "ROUTETARGET", routeTarget) )
	{
		cerr << "Error, could not parse node: ROUTETARGET for planet "
		     << name << '\n';
		starsExit("planet.cpp", -1);
	}

	if( !parseXML_noFail(tree, "PACKETSPEED", packetSpeed) )
	{
		cerr << "Error, could not parse node: PACKETSPEED for planet "
		     << name << '\n';
		starsExit("planet.cpp", -1);
	}

	if( !parseXML_noFail(tree, "STARBASEID", starBaseID) )
		starBaseID = 0;

	child = mxmlFindElement(tree, tree, "MAXINFRASTRUCTURE", NULL, NULL, MXML_DESCEND);
	if( child )
	{
		child = child->child;

		valCounter = assembleNodeArray(child, valArray, 4);
		if( valCounter != 4 )
		{
			cerr << "Error, expected 4 INFRASTRUCTURE DATA points for planet "
			     << name << ", found " << valCounter << '\n';
			return false;
		}

		maxInfrastructure.mines     = valArray[0];
		maxInfrastructure.factories = valArray[1];
		maxInfrastructure.defenses  = valArray[2];
		maxInfrastructure.scanner   = valArray[3];
	}

	child = mxmlFindElement(tree, tree, "PLANETARYBUILDQUEUE", NULL, NULL, MXML_DESCEND);

	child = mxmlFindElement(child, child, "PLANETARYBUILDQUEUEENTRY", NULL, NULL, MXML_DESCEND);
	while( child )
	{
		BuildQueueEntry *tempOrder = new BuildQueueEntry();
		if( !tempOrder->parseXML(child) )
		{
			cerr << "Error parsing build orders for planet: " << name << endl;
			cerr << "Error occured while parsing order number "
			     << (planetBuildQueue.size() + 1) << endl;
			starsExit("planet.cpp", -1);
		}

		planetBuildQueue.push_back(tempOrder);

		child = mxmlFindElement(child, tree, "PLANETARYBUILDQUEUEENTRY", NULL, NULL, MXML_NO_DESCEND);
	}

	unsigned tmpU = 0;
	parseXML_noFail(tree, "IS_HOMEWORLD", tmpU);
	if( tmpU == 1 )
		isHomeworld_ = true;

	yearDataGathered = 0;
	parseXML_noFail(tree, "YEARDATAGATHERED", yearDataGathered);

	return true;
}

// parse the planet information out of the XML root. if 'init' is true, then
// this is an initial read of a universe definition, and the function will not
// generate errors for items that should only be in the host file XML tree
bool parsePlanets(mxml_node_t *tree, std::vector<Planet*> &list, bool init)
{
	mxml_node_t *node = mxmlFindElement(tree, tree, "PLANET", NULL, NULL,
	         MXML_DESCEND_FIRST);

	// should be at least one planet...
	if( !node )
	{
		cerr << "Error, could not locate PLANET keyword\n";
		return false;
	}

	while( node )
	{
		Planet *newPlanet = new Planet();
		newPlanet->parseXML(node);

		list.push_back(newPlanet);

		node = mxmlFindElement(node, tree, "PLANET", NULL, NULL,
		          MXML_NO_DESCEND);
	}

	return true;
}

void Planet::toXMLString(string &theString) const
{
	std::ostringstream os;
	os << "<PLANET>\n";

	std::string tempString;
	PlaceableObject::toXMLStringGuts(tempString);
	os << tempString;

	const unsigned tmpU = isHomeworld() ? 1 : 0;
	os << "<IS_HOMEWORLD>" << tmpU << "</IS_HOMEWORLD>\n";

	mineralConcentrations.toXMLString(tempString, "MINERALCONCENTRATIONS", "SCALE=\"PartsPerUnit\"");
	os << tempString;

	os << "<INFRASTRUCTURE TYPE=\"mines factories defenses scanner\">"
	         << infrastructure.mines << ' ' << infrastructure.factories << ' '
	         << infrastructure.defenses << ' ' << infrastructure.scanner
	         << "</INFRASTRUCTURE>\n";

	os << "<MAXINFRASTRUCTURE TYPE=\"mines factories defenses scanner\">"
	         << maxInfrastructure.mines << ' ' << maxInfrastructure.factories << ' '
	         << maxInfrastructure.defenses << ' ' << maxInfrastructure.scanner
	         << "</MAXINFRASTRUCTURE>\n";

	os << "<HAB TYPE=\"gtr 1.0\" ORDER=\"ORIGINAL,CURRENT\">"
	   <<        originalEnvironment.grav
	   << ' ' << originalEnvironment.temp
	   << ' ' << originalEnvironment.rad
	   << ' ' << currentEnvironment.grav
	   << ' ' << currentEnvironment.temp
	   << ' ' << currentEnvironment.rad
	   << "</HAB>" << std::endl;

	os << "<POP_REMAINDER>"      << popRemainder_         << "</POP_REMAINDER>" << std::endl;
	os << "<RESOURCESGENERATED>" << currentResourceOutput << "</RESOURCESGENERATED>" << std::endl;
	os << "<PACKETSPEED>"        << packetSpeed           << "</PACKETSPEED>" << std::endl;
	os << "<PACKETTARGET>"       << packetTarget          << "</PACKETTARGET>" << std::endl;
	os << "<ROUTETARGET>"        << routeTarget           << "</ROUTETARGET>" << std::endl;
	os << "<STARBASEID>"         << starBaseID            << "</STARBASEID>" << std::endl;

	os << "<PLANETARYBUILDQUEUE>" << std::endl;

	for(unsigned i = 0; i < planetBuildQueue.size(); ++i)
	{
		planetBuildQueue[i]->toXMLString(tempString);
		os << tempString;
	}

	os << "</PLANETARYBUILDQUEUE>" << std::endl;

	os << "<YEARDATAGATHERED>" << yearDataGathered << "</YEARDATAGATHERED>" << std::endl;

	os << "</PLANET>" << std::endl;

	theString = os.str();
}

// handle actions for a planet
bool Planet::actionHandler(GameData &gd, const Action &what)
{
	//cerr << "Planet " << name << ": Processing order - " << what.description << endl;

	Race *const ownerRace = ownerID != 0 ? gd.getRaceById(ownerID) : NULL;

	switch( what.ID )
	{
	case planetaryPopulationGrowth_action: // increase local pop
		{
			// calculate growth race
			const double growthRate = ownerRace->getGrowthRate(this);

			// read-modify-write the population
			double tmpPop = cargoList.cargoDetail[_colonists].amount * populationCargoUnit;
			//Ned, Stars! bug... fraction does not grow
			// add in fraction of a cargo unit
			//tmpPop += popRemainder_;

			// add in growth
			tmpPop += tmpPop * growthRate / 100.0;
			// add in fraction of a cargo unit
			tmpPop += popRemainder_;

			// div-mod
			popRemainder_ = unsigned(fmod(tmpPop, 100.0));
			cargoList.cargoDetail[_colonists].amount = unsigned(tmpPop / 100.0);

			return true;
		}

	case unload_cargo_action:
	case gui_unload_action:
		return unloadAction(gd, what.ID == gui_unload_action);

	case load_cargo_action:
	case gui_load_action:
		return loadAction(gd, what.ID == gui_load_action);

	case planetaryUpdatePlanetVariables_action:
		{
			currentResourceOutput = ownerRace->getResourceOutput(this);

			ownerRace->getMaxInfrastructure(this, maxInfrastructure);

			// foreach player
			for(size_t playerIndex = 0; playerIndex < gd.playerList.size();
			    ++playerIndex)
			{
				PlayerData *owner = gd.playerList[playerIndex];

				bool found = false;

				// foreach planet in player's list
				for(size_t pIndex = 0; !found && pIndex < owner->planetList.size();
				    ++pIndex)
				{
					// player's planet
					Planet *pPlanet = owner->planetList[pIndex];

					// if player's planet is same as server planet
					if( pPlanet->objectId == objectId )
					{
						found = true;

						if( owner->race->ownerID == ownerID )
						{
							// player owns the planet, reveal everything
							if( pPlanet != this )
								*pPlanet = *this;
						}
						//else Update scan data here???
					}
				}
			}
			return true;
		}

	case planetaryExtractMineralWealth_action:
		{
			// no operator+=
			cargoList = cargoList + ownerRace->getMineralsExtracted(this);
			return true;
		}

	case planetaryProcessBuildQueue_action:
		resourcesRemainingThisTurn = currentResourceOutput;
		processBuildQueue(gd);
		return true;

	case resolve_ground_combat_action:
		resolveGroundCombat(gd);
		return true;

	case standard_bomb_planet_action:
	case smart_bomb_planet_action:
		resolveBombingRuns(gd, what.ID == smart_bomb_planet_action);
		return true;

	case                  base_action: // not a real action
	case            scrapFleet_action: // planets can't be scrapped
	case             fleetMove_action: // planets don't move
	case          follow_fleet_action: // planets don't follow
	case              colonize_action: // planets can't colonize
	case             lay_mines_action: // planets don't lay minefields
	case lay_mines_before_move_action: // planets don't lay minefields
	case           decay_mines_action: // planets are not minefields
		;
	}

	cerr << "Error, Planet object " << name << '(' << objectId
	     << "), was sent an unactionable Action request of type "
	     << what.description << '(' << what.ID << ")\n";
	return false;
}

void Planet::resolveGroundCombat(GameData &gd)
{
	// check the list of people invading
	if( waypointDropRecords.size() == 0 )
		return; // done

	// is planet initially empty?
	const bool empty = getPopulation() == 0;

	// all enemy cargo unloads get collected here
	std::vector<WaypointDropDetail*> remainingDrops;

	size_t i; // MS bug
	for(i = 0; i < waypointDropRecords.size(); ++i)
	{
		// self unloads bolster populaton
		if( ownerID == waypointDropRecords[i]->theRace->ownerID )
		{
			cargoList.cargoDetail[_colonists].amount +=
			           waypointDropRecords[i]->number;
		}
		else // enemy unload
			remainingDrops.push_back(waypointDropRecords[i]);
	}

	std::string message;
	for(i = 0; i < remainingDrops.size(); ++i)
	{
		// fight!
		//Ned, wrong algore
		if( remainingDrops[i]->number > getPopulation() )
		{
			unsigned pPlanetIter = 0; // MS bug
			PlayerData *pd;

			// if not newly colonized
			if( !empty )
			{
				message = "You have lost a pitched battle for control of planet " + name;
				gd.playerMessage(ownerID, 0, message);

				// update old owner data structure
				pd = gd.getPlayerData(ownerID);
				for(pPlanetIter = 0; pPlanetIter < pd->planetList.size(); pPlanetIter++)
				{
					Planet *pPlanet = pd->planetList[pPlanetIter];
					if( pPlanet->objectId == objectId )
					{
						pPlanet->ownerID = remainingDrops[i]->theRace->ownerID;
						break;
					}
				}
			}

			// transfer ownership to new owner
			ownerID = remainingDrops[i]->theRace->ownerID;
			pd = gd.getPlayerData(ownerID);
			for(pPlanetIter = 0; pPlanetIter < pd->planetList.size(); ++pPlanetIter)
			{
				Planet *pPlanet = pd->planetList[pPlanetIter];
				if( pPlanet->objectId == objectId )
				{
					pPlanet->ownerID = remainingDrops[i]->theRace->ownerID;
					break;
				}
			}

			if( empty )
			{
				message = "You have colonized ";
			}
			else
			{
				message = "You have won a pitched battle for control of planet ";
			}
			message += name;

			gd.playerMessage(ownerID, 0, message);
			cargoList.cargoDetail[_colonists].amount = remainingDrops[i]->number -
			          cargoList.cargoDetail[_colonists].amount;
		}
		else
		{
			// notify invader he lost
			message = "You have lost a pitched battle for control of planet " + name;
			gd.playerMessage(remainingDrops[i]->theRace->ownerID, 0, message);

			cargoList.cargoDetail[_colonists].amount -= remainingDrops[i]->number;

			// notify owner planet is still his
			message = "You have slaughered some attackers at planet " + name;
			gd.playerMessage(ownerID, 0, message);
		}
	}

	// clean up memory
	for(i = 0; i < waypointDropRecords.size(); ++i)
	{
		WaypointDropDetail *temp = waypointDropRecords[i];
		delete temp;
		waypointDropRecords[i] = NULL;
	}

	waypointDropRecords.clear();
}

unsigned Planet::getPopulation(void) const
{
	return cargoList.cargoDetail[_colonists].amount;
}

CargoManifest Planet::getMineralConcentrations(void) const
{
	return mineralConcentrations;
}

unsigned Planet::getMines(void) const
{
	return infrastructure.mines;
}

unsigned Planet::getFactories(void) const
{
	return infrastructure.factories;
}

void Planet::processBuildQueue(GameData &gd)
{
	if( planetBuildQueue.size() == 0 )
		return; // done

	// hold back some resources for research
	const unsigned resourceHold =
	         gd.getPlayerData(ownerID)->researchTechHoldback *
	         resourcesRemainingThisTurn / 100;
	gd.getPlayerData(ownerID)->resourcesAllocatedToResearch += resourceHold;
	resourcesRemainingThisTurn -= resourceHold;
	// cerr<<"Planet "<<name<<" processing queue with "<<resourcesRemainingThisTurn<<" resources/\n";

	Race *const ownerRace = gd.getRaceById(ownerID);

	std::vector<BuildQueueEntry *>::iterator processIndex =
	    planetBuildQueue.begin();

	bool packetInProgress = false; // loop-carry

	bool processBuildOrders = resourcesRemainingThisTurn > 0;
	while( processBuildOrders )
	{
		BuildQueueEntry *currentEntry = *processIndex;

		// get the information for the current item to build
		StructuralElement *theElement = NULL;

		unsigned unitResourcesRequired = 0;
		CargoManifest unitMineralResources;

		std::string componentName;
		switch( currentEntry->itemType )
		{
		case _noBuild_BUILDTYPE:
			cerr << "Error, NOBUILDTYPE queue entry in build queue for planet " << name << ". \n\tHow did this happen?\n";
			starsExit("planet.cpp", -1);
			break;

		case _ship_BUILDTYPE:
			{
				Ship *design = gd.getDesignByID(currentEntry->designID);
				if( !design->isSpaceStation && starBaseID == 0 )
				{
					// not a SB, and no SB available
					std::string message = "Planet " + name +
					    " attempted to build a ship without first having a starbase.  Build queue processing was aborted.";
					gd.playerMessage(ownerID, 0, message);
					return;
				}

				// we are building a base or we have a base -- check mass requirement
				if( starBaseID != 0 )
				{
					Fleet *baseFleet = gd.getFleetByID(starBaseID);
					if( !baseFleet->isSpaceStation )
					{
						std::cerr << "Error, a non-base fleet has somehow been assigned as a base to planet "
						          << name << " fleet ID " << baseFleet->objectId << std::endl;
						starsExit("planet.cpp", -1);
					}

					const unsigned baseCap = baseFleet->getDockCapacity();
					if( baseCap < design->mass )
					{
						// not a SB, and no SB available
						char massStr[64];
						char capacityStr[64];
						sprintf(massStr, " %i ", design->mass);
						sprintf(capacityStr, " %i ", baseCap);
						std::string message = "Planet " + name +
						              " attempted to build a ship too massive for its starbase. Mass = "
						              + massStr + "capacity = " + capacityStr + "\n";
						gd.playerMessage(ownerID, 0, message);
						return;
					}
				}

				unitResourcesRequired = design->baseResourceCost;
				unitMineralResources  = design->baseMineralCost;
				break;
			}

		case _defense_BUILDTYPE:
			componentName = "Defense";
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _factory_BUILDTYPE:
			componentName = "Factory";
			theElement = gd.componentMap[componentName];
			//unitResourcesRequired = theElement->resourceCostBase;
			unitResourcesRequired = ownerRace->factoryResourceCost;
			//unitMineralResources = theElement->mineralCostBase;
			unitMineralResources = ownerRace->factoryMineralCost;
			break;

		case _mine_BUILDTYPE:
			componentName = "Mine";
			theElement = gd.componentMap[componentName];
			//unitResourcesRequired = theElement->resourceCostBase;
			unitResourcesRequired = ownerRace->mineResourceCost;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _scanner_BUILDTYPE:
			componentName = "Planetary Scanner";
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _ironiumPacket_BUILDTYPE:
			if( getFlingerSpeed(gd) < packetSpeed )
			{
				// flinger not good enough
				char reqSpeed[64];
				char availSpeed[64];
				sprintf(reqSpeed, " %i ", packetSpeed);
				sprintf(availSpeed, " %i ", getFlingerSpeed(gd));
				string message = "Planet " + name + " attempted to build a packet @ speed = Warp" + reqSpeed + ". Available accelerator capacity = Warp" + availSpeed + "\n";
				gd.playerMessage(ownerID, 0, message);
				return;
			}

			componentName = "Ironium Packet";
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _boraniumPacket_BUILDTYPE:
			if( getFlingerSpeed(gd) < packetSpeed )
			{
				// flinger not good enough
				char reqSpeed[64];
				char availSpeed[64];
				sprintf(reqSpeed, " %i ", packetSpeed);
				sprintf(availSpeed, " %i ", getFlingerSpeed(gd));
				string message = "Planet " + name + " attempted to build a packet @ speed = Warp" + reqSpeed + ". Available accelerator capacity = Warp" + availSpeed + "\n";
				gd.playerMessage(ownerID, 0, message);
				return;
			}

			packetInProgress = true;
			componentName = "Boranium Packet";
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _germaniumPacket_BUILDTYPE:
			if( getFlingerSpeed(gd) < packetSpeed )
			{
				// flinger not good enough
				char reqSpeed[64];
				char availSpeed[64];
				sprintf(reqSpeed, " %i ", packetSpeed);
				sprintf(availSpeed, " %i ", getFlingerSpeed(gd));
				string message = "Planet " + name + " attempted to build a packet @ speed = Warp" + reqSpeed + ". Available accelerator capacity = Warp" + availSpeed + "\n";
				gd.playerMessage(ownerID, 0, message);
				return;
			}

			packetInProgress = true;
			componentName = "Germanium Packet";
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _combinedPacket_BUILDTYPE:
			if( getFlingerSpeed(gd) < packetSpeed )
			{
				// flinger not good enough
				char reqSpeed[64];
				char availSpeed[64];
				sprintf(reqSpeed, " %i ", packetSpeed);
				sprintf(availSpeed, " %i ", getFlingerSpeed(gd));
				string message = "Planet " + name + " attempted to build a packet @ speed = Warp" + reqSpeed + ". Available accelerator capacity = Warp" + availSpeed + "\n";
				gd.playerMessage(ownerID, 0, message);
				return;
			}

			packetInProgress = true;
			componentName = "Combined Packet";
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _minTerraform_BUILDTYPE:
		case _maxTerraform_BUILDTYPE:
			componentName = "Terraform";
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _mineralAlchemy_BUILDTYPE:
			theElement = gd.componentMap[componentName];
			unitResourcesRequired = theElement->resourceCostBase;
			unitMineralResources = theElement->mineralCostBase;
			break;

		case _numBuildableItemTypes_BUILDTYPE:
		default:
			cerr << "Error, undefined build type in queue entry in build queue for planet " << name << ". \n\tHow did this happen?\n";
			starsExit("planet.cpp", -1);
		}

		const unsigned percentRemaining = fractionalCompletionDenominator -
		          currentEntry->percentComplete;
		unsigned fractionalResourcesRequired = (uint64_t(unitResourcesRequired) * percentRemaining + fractionalCompletionDenominator/2) /
		          fractionalCompletionDenominator;
		CargoManifest fractionalMineralResources;
		for(unsigned i = 0; i < _numCargoTypes; ++i)
		{
			fractionalMineralResources.cargoDetail[i].amount =
			           (uint64_t(unitMineralResources.cargoDetail[i].amount) * percentRemaining + fractionalCompletionDenominator/2) /
			           fractionalCompletionDenominator;
		}

		// Should we build one?
		bool actOnEntry = true;

		// check for planetary max
		if( currentEntry->itemType == _factory_BUILDTYPE ||
		    currentEntry->itemType ==    _mine_BUILDTYPE ||
		    currentEntry->itemType == _defense_BUILDTYPE )
		{
			actOnEntry =
			           ((currentEntry->itemType == _factory_BUILDTYPE) &&
			           (infrastructure.factories < maxInfrastructure.factories))
			           ||
			           ((currentEntry->itemType == _mine_BUILDTYPE) &&
			           (infrastructure.mines < maxInfrastructure.mines))
			           ||
			           ((currentEntry->itemType == _defense_BUILDTYPE) &&
			            (infrastructure.defenses < maxInfrastructure.defenses));
		}

		if( actOnEntry )
		{
			// if we can finish the job
			if( fractionalResourcesRequired <= resourcesRemainingThisTurn &&
			    !fractionalMineralResources.anyGreaterThan(cargoList) )
			{
				// yes, did we start a packet?
				if( currentEntry->itemType ==   _ironiumPacket_BUILDTYPE ||
				    currentEntry->itemType ==  _boraniumPacket_BUILDTYPE ||
				    currentEntry->itemType == _germaniumPacket_BUILDTYPE ||
				    currentEntry->itemType ==  _combinedPacket_BUILDTYPE )
				{
					packetInProgress = true;
				}

				// pay for it
				resourcesRemainingThisTurn -= fractionalResourcesRequired;
				cargoList = cargoList - fractionalMineralResources;

				// build it!
				buildOneFromQueueEntry(currentEntry, gd);

				// update the build queue
				currentEntry->percentComplete = 0;
				currentEntry->remainingBuildQuantity--;

				// are we finished with this entry?
				if( currentEntry->remainingBuildQuantity == 0 )
				{
					if( currentEntry->autoBuild )
					{
						currentEntry->remainingBuildQuantity = currentEntry->originalBuildQuantity;
						processIndex++;
					}
					else
					{
						// done (queue is cleaned at end)
						++processIndex;
					}
				}
			}
			else // how much can we build?
			{
				unsigned resourcePercent = 0;
				if( unitResourcesRequired > 0 )
					resourcePercent = uint64_t(fractionalCompletionDenominator) * resourcesRemainingThisTurn / unitResourcesRequired;

				unsigned mineralsPercent = fractionalCompletionDenominator;
				unsigned i; // MS bug
				for(i = 0; i < _numCargoTypes; ++i)
				{
					unsigned cPercent = 0;

					if( unitMineralResources.cargoDetail[i].amount != 0 )
						cPercent = (uint64_t(fractionalCompletionDenominator) * cargoList.cargoDetail[i].amount) / unitMineralResources.cargoDetail[i].amount;
					else if( unitMineralResources.cargoDetail[i].amount == 0 )
						cPercent = fractionalCompletionDenominator;

					if( cPercent < mineralsPercent )
						mineralsPercent = cPercent;
				}

				unsigned percentCompleteable = resourcePercent;
				if( mineralsPercent < percentCompleteable )
					percentCompleteable = mineralsPercent;

				// how much will it cost?
				unsigned resourcesConsumed = (uint64_t(percentCompleteable) * unitResourcesRequired + fractionalCompletionDenominator/2) / fractionalCompletionDenominator;

				// minerals are more complicated, have to account for fractions accumulating
				// For example, building a factory (10 res) 1 res at a time
				// any given 10% is 0 g, but we must charge for the g somewhere...
				// Note, we would have a similar problem for any item costing > fractionalCompletionDenominator resources

				// how much has the player paid so far?
				CargoManifest currMnrl;
				for(i = 0; i < _numCargoTypes; ++i)
					currMnrl.cargoDetail[i].amount = (uint64_t(currentEntry->percentComplete) * unitMineralResources.cargoDetail[i].amount + fractionalCompletionDenominator/2) / fractionalCompletionDenominator;

				// how much would he have to pay to reach the current percent?
				CargoManifest finalMnrl;
				for(i = 0; i < _numCargoTypes; ++i)
					finalMnrl.cargoDetail[i].amount = (uint64_t(currentEntry->percentComplete+percentCompleteable) * unitMineralResources.cargoDetail[i].amount + fractionalCompletionDenominator/2) / fractionalCompletionDenominator;

				// take the delta
				CargoManifest mineralsConsumed = finalMnrl - currMnrl;

				// spend the cost
				resourcesRemainingThisTurn -= resourcesConsumed;
				cargoList = cargoList - mineralsConsumed;

				currentEntry->percentComplete += percentCompleteable;
				if( currentEntry->percentComplete >= fractionalCompletionDenominator )
				{
					std::cerr << "Unexpected fractional completion of build order for planet "
					          << name << std::endl;
					std::string tempString;
					currentEntry->toXMLString(tempString);
					std::cerr << " While processing order:\n" << tempString << std::endl;
					starsExit("planet.cpp", -1);
				}

				// auto build is non-blocking
				if( !currentEntry->autoBuild )
					processBuildOrders = false;
				else
					++processIndex;

			} //else could only build a fraction of the thing...
		} // actOnEntry
		else //don't act, skip
		{
			++processIndex;
		}

		if( processIndex == planetBuildQueue.end() )
			processBuildOrders = false;

	} //while( processBuildOrders )

	if( packetInProgress )
	{
		MassPacket *newPacket = new MassPacket();
		char cTemp[64];

		string newPname = "Mineral Packet - " + name;
		sprintf(cTemp, " %i", gd.theUniverse->gameYear);
		newPname += cTemp;
		newPacket->setName(newPname);

		newPacket->cargoList = packetBuild;
		newPacket->topSafeSpeed = packetSpeed;
		newPacket->lastKnownSpeed = packetSpeed;
		newPacket->coordinates = coordinates;
		newPacket->fuelCapacity = 0;
		newPacket->objectId = nextObjectId++;
		newPacket->ownerID = ownerID;

		Planet *target = gd.getPlanetByID(packetTarget);

		Order *packetMoveOrders = new Order();
		packetMoveOrders->destination = target->coordinates;
		packetMoveOrders->targetID = packetTarget;
		packetMoveOrders->orderDescriptorString = "Move to target planet";
		packetMoveOrders->playerNumber = ownerID;
		packetMoveOrders->myOrderID = move_order;
		packetMoveOrders->objectId = newPacket->objectId;
		packetMoveOrders->orderNumber = 1;
		packetMoveOrders->warpFactor = packetSpeed;
		newPacket->currentOrders.push_back(packetMoveOrders);
		newPacket->coordinates.moveToward(target->coordinates, (packetSpeed * packetSpeed / 2));
		gd.fleetList.push_back(newPacket);
		std::string mess = "Planet " + name + " produced a mass packet and sent it to planet " + target->name;
		gd.playerMessage(ownerID, 0, mess);
	}

	CargoManifest zeroPacket;
	packetBuild = zeroPacket;

	cleanBuildQueue(gd);
}

// make one fleet per design
static
Fleet* getBuildFleet(const Location &coordinates, BuildQueueEntry *currentEntry, GameData &gd)
{
	PlayerData* pd = gd.getPlayerData(currentEntry->ownerID);
	Ship *design = gd.getDesignByID(currentEntry->designID);

	std::map<Location, Fleet*>::const_iterator mi = pd->newBuildMap_.find(coordinates);
	if( mi != pd->newBuildMap_.end() )
	{
		return mi->second;
	}

	Fleet *newFleet = new Fleet();
	newFleet->objectId = IdGen::getInstance()->makeFleet(currentEntry->ownerID, false, pd->objectMap);
	pd->objectMap[newFleet->objectId] = newFleet;

	newFleet->ownerID = currentEntry->ownerID;
	newFleet->coordinates = coordinates;

	std::ostringstream os;
	os << design->getName() << ' ' << '#' << newFleet->objectId;
	newFleet->setName(os.str());

	gd.fleetList.push_back(newFleet);
	pd->fleetList.push_back(newFleet);
	pd->newBuildMap_[coordinates] = newFleet;

	return newFleet;
}

/// build the item specified by 'currentEntry'
void Planet::buildOneFromQueueEntry(BuildQueueEntry *currentEntry, GameData &gd)
{
	switch( currentEntry->itemType )
	{
	case _noBuild_BUILDTYPE:
		std::cerr << "Error, NOBUILDTYPE queue entry in build queue for planet "
		          << name << ". \n\tHow did this happen?\n";
		starsExit("planet.cpp", -1);
		break;

	case _ship_BUILDTYPE:
	{
		Ship *design = gd.getDesignByID(currentEntry->designID);

		PlayerData* pd = gd.getPlayerData(currentEntry->ownerID);
		if( !design->isBuildable(pd) )
		{
			std::string msg = "Your attempt to build a  " + design->getName();
			msg += "at planet " + getName() + " ended in failure!";

			gd.playerMessage(ownerID, 0, msg);
			break;
		}

		// the ship needs a fleet
		Fleet *newFleet = getBuildFleet(coordinates, currentEntry, gd);

		// build the design into a ship
		Ship *newShip = newFleet->hasShip(design->designhullID);
		if( !newShip )
		{
			newShip = new Ship(*design);

			newShip->objectId = newFleet->objectId;
			newShip->quantity = 0;

			newFleet->shipRoster.push_back(newShip);
		}

		++newShip->quantity;
		newShip->updateCostAndMass(gd);

		newFleet->updateFleetVariables(gd);
		newFleet->cargoList.cargoDetail[_fuel].amount = newFleet->fuelCapacity;

		if( newFleet->isSpaceStation )
			starBaseID = newFleet->objectId;
	}
		break;

	case _defense_BUILDTYPE: infrastructure.defenses++;  break;

	case _factory_BUILDTYPE: infrastructure.factories++; break;

	case    _mine_BUILDTYPE: infrastructure.mines++;     break;

	case _scanner_BUILDTYPE: infrastructure.scanner = 1; break;

	case   _ironiumPacket_BUILDTYPE:
	case  _boraniumPacket_BUILDTYPE:
	case _germaniumPacket_BUILDTYPE:
	case  _combinedPacket_BUILDTYPE:
	{
		string packetType = "Combined Packet";
		if( currentEntry->itemType == _ironiumPacket_BUILDTYPE )
			packetType = "Ironium Packet";
		if( currentEntry->itemType == _boraniumPacket_BUILDTYPE )
			packetType = "Boranium Packet";
		if( currentEntry->itemType == _germaniumPacket_BUILDTYPE )
			packetType = "Germanium Packet";

		StructuralElement *theElement = gd.componentMap[packetType];
		packetBuild = packetBuild + theElement->mineralCostBase;
	}
		break;

	case _minTerraform_BUILDTYPE:
	case _maxTerraform_BUILDTYPE:
	{
		Race *ownerRace = gd.getRaceById(ownerID);
		Environment delta = ownerRace->getTerraformAmount(this, false, gd);
		currentEnvironment = currentEnvironment - delta;
	}
		break;

	case _mineralAlchemy_BUILDTYPE:
		cerr << "Error, I don't know how to do mineral alchemy  yet!  planet =  " << name << ". \n\tHow did this happen?\n";
		starsExit("planet.cpp", -1);
		break;

	case _numBuildableItemTypes_BUILDTYPE:
	default:
		cerr << "Error, undefined build type in queue entry in build queue for planet "
		     << name << ". \n\tHow did this happen?\n";
		starsExit("planet.cpp", -1);
	}
}

/// clean the build queue after a turn of building
void Planet::cleanBuildQueue(GameData &gd)
{
	std::vector<BuildQueueEntry*> tempQ;
	bool needPush = false;
	// split the partial autobuilds, and reset autobuild remaingQ
	for(unsigned i = 0; i < planetBuildQueue.size(); ++i)
	{
		BuildQueueEntry *cqe = planetBuildQueue[i];

		if( cqe->autoBuild )
		{
			if( cqe->percentComplete != 0 )
			{
				needPush = true; // push the queue around due to an add

				BuildQueueEntry *nqe = new BuildQueueEntry();
				nqe->autoBuild = false;
				nqe->designID = cqe->designID;
				nqe->itemType = cqe->itemType;
				nqe->originalBuildQuantity = 1;
				nqe->ownerID = cqe->ownerID;
				nqe->percentComplete = cqe->percentComplete;
				nqe->planetID = cqe->planetID;
				nqe->remainingBuildQuantity = 1;

				tempQ.push_back(nqe);
			}
			cqe->remainingBuildQuantity = cqe->originalBuildQuantity;
			cqe->percentComplete = 0;
		}
		else
		{
			if( cqe->remainingBuildQuantity == 0 )
				needPush = true;
		}
	}

	if( needPush )
	{
		unsigned i; // MS bug
		for(i = 0; i < planetBuildQueue.size(); ++i)
		{
			if( planetBuildQueue[i]->remainingBuildQuantity != 0 )
				tempQ.push_back(planetBuildQueue[i]);
		}

		planetBuildQueue.clear();

		for(i = 0; i < tempQ.size(); ++i)
		{
			planetBuildQueue.push_back(tempQ[i]);
		}
	}
}

unsigned Planet::getFlingerSpeed(GameData &gd)
{
	if( starBaseID == 0 )
		return 0;

	return gd.getFleetByID(starBaseID)->maxPacketSpeed;
}

void Planet::resolveBombingRuns(GameData &gd, bool smart)
{
	// if no bombers, or defensive starbase, or dead planet
	if( bombingList.size() == 0 || starBaseID != 0 || ownerID == 0 )
		return; // done

	// calculate damage
	unsigned KIA = BombCalculations::popKilledBombing(gd, bombingList, this, !smart);
	PlanetaryInfrastructure DIA = BombCalculations::infraKilledBombing(gd, bombingList, this, !smart);

	// no damage
	//Ned? can you have 0 pop, but non-zero infra?
	if( KIA == 0 )
		return;

	char kiaString[80];
	char diaString[256];
	sprintf(kiaString, "%i", KIA);
	sprintf(diaString, "%i factories, %i mines, and %i defenses.", DIA.factories, DIA.mines, DIA.defenses);

	std::string bombTypeString = "standard bombs";
	if( smart ) bombTypeString = "smart bombs";

	// notify the bombing players
	for(unsigned i = 0; i < bombingList.size(); ++i)
	{
		Fleet *tempFleet = bombingList[i];
		if( tempFleet->ownerID == ownerID )
			continue;

		std::ostringstream os;
		os << "Fleet: " << tempFleet->getName()
		   << '(' << tempFleet->objectId << ')' << " bombed planet "
		   << name << " with " << bombTypeString
		   << ".  Total destruction wraught by all fleets involved killed "
		   << kiaString << " colonists and destroyed " << diaString;
		gd.playerMessage(tempFleet->ownerID, 0, os.str());
	}

	// notify the planet owner
	std::string msg = "Planet " + name;
	msg += " was bombed by your enemies using " + bombTypeString;
	msg += "  Total destruction wraught by all fleets involved killed ";
	msg += kiaString;
	msg += " colonists and destroyed ";
	msg += diaString;

	if( cargoList.cargoDetail[_colonists].amount < KIA )
		msg += "  Your hapless colonists were completely bombed into oblivion.";

	gd.playerMessage(ownerID, 0, msg);

	// apply the damage
	if( cargoList.cargoDetail[_colonists].amount < KIA )
		abandonPlanet();
	else
		cargoList.cargoDetail[_colonists].amount -= KIA;

	infrastructure = infrastructure - DIA;
}

/// clear off the planet from any owner influence
void Planet::abandonPlanet(void)
{
	ownerID = 0;
	cargoList.cargoDetail[_colonists].amount = 0;
	starBaseID = 0;
	currentEnvironment = originalEnvironment;
}

/// clear information that is not visible in a basic scan
void Planet::clearPrivateData(void)
{
	currentEnvironment.clear();
	originalEnvironment.clear();
	mineralConcentrations.clear();
	ownerID = 0;
	isHomeworld_ = false;
	starBaseID = 0;
	yearDataGathered = 0;

	clearScanData();
}

/// clear information that is not visible in a pen scan
void Planet::clearScanData(void)
{
	packetBuild.clear();

	planetBuildQueue.clear();
	infrastructure.clear();
	maxInfrastructure.clear();
	cargoList.clear();

	currentResourceOutput = 0;
	packetTarget = 0;
	routeTarget = 0;
	packetSpeed = 0;
}

// 'target' will be overwritten by 'this'
// 'target' should be a pointer to the server planet
// 'this' should be the player planet
void Planet::propagateData(const std::map<uint64_t, uint64_t> &remapTbl,
                           Planet *target)
{
	// Note, need to check partial build complete status for correctness
	// add check code here

	// update course information
	target->packetSpeed  = packetSpeed;
	target->packetTarget = packetTarget;
	target->routeTarget  = routeTarget;

	// remove target's build queue
	while( target->planetBuildQueue.size() > 0 )
	{
		delete target->planetBuildQueue.back();
		target->planetBuildQueue.pop_back();
	}

	// convert player build queue to server
	for(unsigned i = 0; i < planetBuildQueue.size(); ++i)
	{
		BuildQueueEntry *bqep = planetBuildQueue[i];
		if( bqep->itemType == _ship_BUILDTYPE )
		{
			std::map<uint64_t, uint64_t>::const_iterator rmi =
			           remapTbl.find(bqep->designID);

			if( rmi != remapTbl.end() )
				bqep->designID = rmi->second;
		}

		//Ned? copy?
		target->planetBuildQueue.push_back(planetBuildQueue[i]);
	}
}

//----------------------------------------------------------------------------
BuildQueueEntry::BuildQueueEntry(void)
{
	designID = 0;
	itemType = _noBuild_BUILDTYPE;
	ownerID = 0;
	percentComplete = 0;
	planetID = 0;
	originalBuildQuantity = 0;
	remainingBuildQuantity = 0;
	autoBuild = false;
}

bool BuildQueueEntry::parseXML(mxml_node_t *tree)
{
	bool parseOK = true;
	parseOK &= parseXML_noFail(tree, "AUTOBUILD", autoBuild);
	parseOK &= parseXML_noFail(tree, "DESIGNID", designID);

	unsigned tempU = 0;
	parseOK &= parseXML_noFail(tree, "ITEMTYPE", tempU);

	if( tempU >= _numBuildableItemTypes_BUILDTYPE )
		parseOK = false;
	else
		itemType = buildableItemType(tempU);

	parseOK &= parseXML_noFail(tree, "ORIGINALBUILDQUANTITY", originalBuildQuantity);
	parseOK &= parseXML_noFail(tree, "OWNERID", ownerID);
	parseOK &= parseXML_noFail(tree, "PERCENTCOMPLETE", percentComplete);
	parseOK &= parseXML_noFail(tree, "PLANETID", planetID);
	parseOK &= parseXML_noFail(tree, "REMAININGBUILDQUANTITY", remainingBuildQuantity);

	return parseOK;
}

void BuildQueueEntry::toXMLString(string &theString)
{
	std::ostringstream os;
	os << "<PLANETARYBUILDQUEUEENTRY>" << std::endl;

	unsigned utemp1 = 0;
	if( autoBuild )
		utemp1 = 1;
	os << "<AUTOBUILD>" << utemp1 << "</AUTOBUILD>" << std::endl;
	os << "<DESIGNID>"  << designID << "</DESIGNID>" << std::endl;

	unsigned utemp0 = unsigned(itemType);
	os << "<ITEMTYPE>"  << utemp0 << "</ITEMTYPE>" << std::endl;

	os << "<ORIGINALBUILDQUANTITY>" << originalBuildQuantity << "</ORIGINALBUILDQUANTITY>" << std::endl;
	os << "<OWNERID>" << ownerID << "</OWNERID>" << std::endl;
	os << "<PERCENTCOMPLETE>" << percentComplete << "</PERCENTCOMPLETE>" << std::endl;
	os << "<PLANETID>" << planetID << "</PLANETID>" << std::endl;
	os << "<REMAININGBUILDQUANTITY>" << remainingBuildQuantity << "</REMAININGBUILDQUANTITY>" << std::endl;

	os << "</PLANETARYBUILDQUEUEENTRY>" << std::endl;

	theString = os.str();
}

//----------------------------------------------------------------------------
double BombCalculations::defensePopulation(unsigned defenseValue,
                       unsigned quantity, bool isSmartAttack, bool isInvasion)
{
	// = 1- ((1-dValue)^quantity)
	double eTerm = 1.0 - ((double) defenseValue / 10 / 100 - 0.0001);
	if( isSmartAttack )
		eTerm = 1.0 - ((double) defenseValue / 10 / 100 - 0.0001)*0.5;

	if( quantity > 100 )
		quantity = 100;

	double term2 = pow(eTerm, (double) quantity);

	return 1.0 - term2;
}

double BombCalculations::defenseBuilding(unsigned defenseValue,
        unsigned quantity, bool isSmartAttack, bool isInvasion)
{
	return defensePopulation(defenseValue, quantity, isSmartAttack, isInvasion) * 0.5;
}

unsigned BombCalculations::popKilledBombing(GameData &gd,
   vector<Fleet*> bombingFleetList, Planet *defendingPlanet, bool normalBombs)
{
	if( !bombingFleetList.size() ) return 0;

	double percentSum = 0.0;
	if( !normalBombs )
		percentSum = 1.0;

	unsigned minKill = 0;

	//foreach fleet in the bombing list
	for(unsigned fleetIndex = 0; fleetIndex < bombingFleetList.size(); ++fleetIndex)
	{
		Fleet *tempFleet = bombingFleetList[fleetIndex];

		//foreach ship in the bombing fleet
		for(unsigned shipIndex = 0; shipIndex < tempFleet->shipRoster.size(); ++shipIndex)
		{
			Ship *tempShip = tempFleet->shipRoster[shipIndex];

			//foreach slot on the bomber
			for(unsigned slotIndex = 0; slotIndex < tempShip->slotList.size(); ++slotIndex)
			{
				ShipSlot *slot = tempShip->slotList[slotIndex];
				// retrieve the bomb stats
				StructuralElement *el = gd.componentList[slot->structureID];

				if( !el->bomb.isActive ) continue; // oh, not a bomb

				// check the bomb type with the type of pass requested
				if( !el->bomb.isSmart && normalBombs )
				{
					percentSum += ((double) el->bomb.popKillRate / 10.0 * (double) (slot->numberPopulated * tempShip->quantity));
					minKill += el->bomb.minPopKill * slot->numberPopulated;
				}
				else if( el->bomb.isSmart && !normalBombs )
				{
					double temp = 1 - (double) el->bomb.popKillRate / 10.0;
					temp = pow(temp, (double) (slot->numberPopulated * tempShip->quantity));
					percentSum *= temp;
					// smart bombs have no min kill
				}
			}
		}
	}

	if( !normalBombs )
		percentSum = 1 - percentSum;

	PlayerData *pData = gd.getPlayerData(defendingPlanet->ownerID);
	unsigned defenseVal = pData->getDefenseValue();
	unsigned numDef = defendingPlanet->infrastructure.defenses;

	double defPop = defensePopulation(defenseVal, numDef, !normalBombs, false);

	double fpKills = percentSum * (1.0 - defPop) * defendingPlanet->cargoList.cargoDetail[_colonists].amount;
	unsigned pKills = (unsigned) floor(fpKills);

	unsigned mKills = (unsigned) floor(minKill * (1.0 - defPop));

	if( mKills > pKills )
		return mKills;

	return pKills;
}

///@return the amount of infrastructure destroyed by the bombing fleet
PlanetaryInfrastructure BombCalculations::infraKilledBombing(GameData& gd,
   vector<Fleet*> bombingFleetList, Planet* defendingPlanet, bool normalBombs)
{
	PlanetaryInfrastructure infraLoss;
	if( !normalBombs )
		return infraLoss; // smart bombs do no damage to infrastructure

	// calculate the raw damage
	unsigned totalDestroyed = 0;
	//foreach fleet
	for(unsigned fleetIndex = 0; fleetIndex < bombingFleetList.size(); fleetIndex++)
	{
		Fleet *tempFleet = bombingFleetList[fleetIndex];

		//foreach ship in the fleet
		for(unsigned shipIndex = 0; shipIndex < tempFleet->shipRoster.size(); shipIndex++)
		{
			Ship *tempShip = tempFleet->shipRoster[shipIndex];

			//foreach slot
			for(unsigned slotIndex = 0; slotIndex < tempShip->slotList.size(); slotIndex++)
			{
				ShipSlot *slot = tempShip->slotList[slotIndex];
				StructuralElement *el = gd.componentList[slot->structureID];
				if( el->bomb.isActive )
					totalDestroyed += el->bomb.infrastructureKillRate * slot->numberPopulated;
			}
		}
	}

	// calculate the owner planet defensive coverage
	PlayerData *pData = gd.getPlayerData(defendingPlanet->ownerID);
	unsigned defenseVal = pData->getDefenseValue();
	unsigned numDef = defendingPlanet->infrastructure.defenses;
	double defBldg = defenseBuilding(defenseVal, numDef, !normalBombs, false);

	// scale the damage by the defensive coverage
	totalDestroyed = (unsigned) floor((1 - defBldg) * totalDestroyed);

	// find the fraction of all installations destroyed
	unsigned totalInstallations = defendingPlanet->infrastructure.defenses +
	                              defendingPlanet->infrastructure.mines +
	                              defendingPlanet->infrastructure.factories;

	if( totalDestroyed >= totalInstallations )
	{
		// all
		infraLoss = defendingPlanet->infrastructure;

		// except the scanner, it cannot be destroyed
		infraLoss.scanner = 0;
	}
	else
	{
		const double percentDestroyed = (double) totalDestroyed / (double) totalInstallations;

		infraLoss.defenses  = (unsigned) floor(defendingPlanet->infrastructure.defenses  * percentDestroyed);
		infraLoss.mines     = (unsigned) floor(defendingPlanet->infrastructure.mines     * percentDestroyed);
		infraLoss.factories = (unsigned) floor(defendingPlanet->infrastructure.factories * percentDestroyed);
	}

	return infraLoss;
}

//----------------------------------------------------------------------------
///@return a - b, saturate at 0
unsigned subSat(unsigned a, unsigned b)
{
	if( b > a ) return 0;
	return a - b;
}

PlanetaryInfrastructure PlanetaryInfrastructure::operator-(const PlanetaryInfrastructure &param)
{
	PlanetaryInfrastructure temp;
	temp.mines     = subSat(mines, param.mines);
	temp.factories = subSat(factories, param.factories);
	temp.defenses  = subSat(defenses, param.defenses);
	temp.scanner   = subSat(scanner, param.scanner);

	return temp;
}

PlanetaryInfrastructure PlanetaryInfrastructure::operator+(const PlanetaryInfrastructure &param)
{
	PlanetaryInfrastructure temp;
	temp.mines = mines + param.mines;
	temp.factories = factories + param.factories;
	temp.defenses = defenses + param.defenses;
	temp.scanner = scanner + param.scanner;
	return temp;
}

