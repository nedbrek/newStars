#ifdef MSVC
#pragma warning(disable: 4786)
#endif
/*
   reconcile.cpp

   This file contains code required to reconcile the game state changes that players
   make in their copy of the universe against the known good universe file.

   Eventually it should verify all transactions
*/
#include "universe.h"
#include "newStars.h"
#include "planet.h"
#include "race.h"
#include "utils.h"
#include "reconcile.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>

std::map<uint64_t, bool> verifiedLocations;
class shipMap
{
	public:
	   uint64_t designID;
	   uint64_t num;
	   uint64_t totalDamage;

	   shipMap()
      {
         designID = 0;
         num = 0;
         totalDamage = 0;
      }
};

///@return true if fleetA could represent the same player object as fleetB
bool isIdentical(Fleet *fleetA, Fleet *fleetB)
{
	if( !fleetA || !fleetB )
		return false; // invalid fleets are not identical

	bool rval = true; // return value, start as identical

	// check the grandparent (GameObject) members
	//name can change freely
	rval &= fleetA->objectId           == fleetB->objectId;
	rval &= fleetA->ownerID            == fleetB->ownerID;
	rval &= fleetA->myType             == fleetB->myType;

	// check the parent (PlaceableObject) members
	rval &= fleetA->coordinates        == fleetB->coordinates;
	rval &= fleetA->cargoList          == fleetB->cargoList;
	//skip scanner cache
	//orders can change freely

	// check the fleet members
	//battleOrderID can be changed freely
	rval &= fleetA->totalCargoCapacity == fleetB->totalCargoCapacity;
	rval &= fleetA->fuelCapacity       == fleetB->fuelCapacity;
	rval &= fleetA->mass               == fleetB->mass;
	rval &= fleetA->maxPacketSpeed     == fleetB->maxPacketSpeed;
	rval &= fleetA->fleetID            == fleetB->fleetID;
	rval &= fleetA->hasColonizer       == fleetB->hasColonizer;
	rval &= fleetA->isSpaceStation     == fleetB->isSpaceStation;
	rval &= fleetA->topSpeed           == fleetB->topSpeed;
	rval &= fleetA->topSafeSpeed       == fleetB->topSafeSpeed;
	rval &= fleetA->lastKnownSpeed     == fleetB->lastKnownSpeed;
	rval &= fleetA->lastKnownDirection == fleetB->lastKnownDirection;
	//followDistance can change freely

	unsigned i; // MS bug

	// check minelayers
	for(i = 0; rval && i < _numberOfMineTypes; ++i)
		rval &= fleetA->mineLayingRates[i] == fleetB->mineLayingRates[i];

	rval &= fleetA->shipRoster.size()  == fleetB->shipRoster.size();

	if( !rval )
		return false;

	std::map<uint64_t, Ship*> mapA;

	for(i = 0; i < fleetA->shipRoster.size(); ++i)
	{
		Ship *cShip = fleetA->shipRoster[i];
		mapA[cShip->objectId] = cShip;
	}

	for(i = 0; i < fleetB->shipRoster.size(); ++i)
	{
		Ship *cShipB = fleetB->shipRoster[i];
		Ship *cShipA = mapA[cShipB->objectId];
		if( cShipA == NULL   ) return false;
		if( cShipA != cShipB ) return false;
	}
	return true;
}

/// verify that incoming player data is consistant with master state
/// exits with an error if players try something sneaky
static
void validateNewFleet(GameData *gameData, PlayerData *playerData,
    Fleet *whichFleet)
{
	const unsigned whichPlayer = whichFleet->ownerID;
	if( playerData->race->ownerID != whichPlayer ) // something bad happened
		starsExit("Fleet Owner/playerData id mismatch in validateNewFleet()", -1);

	Location fleetLocation = whichFleet->coordinates;
	if( ::verifiedLocations[fleetLocation.getHash()] )
		return; // all fleets at this location are done
	// at this point, we will either exit with an error, or sign off on this
	// location
	::verifiedLocations[fleetLocation.getHash()] = true;

	unsigned u; // MS bug

	// map player and game copy fleets at this location
	std::map<uint64_t, Fleet*> gameFleetMap;
	std::vector<uint64_t>      gameFleetList;

	for(u = 0; u < gameData->fleetList.size(); ++u)
	{
		Fleet *cFleet = gameData->fleetList[u];
		if( cFleet->ownerID == whichPlayer &&
		    cFleet->coordinates == fleetLocation )
		{
			gameFleetMap[cFleet->objectId] = cFleet;
			gameFleetList.push_back(cFleet->objectId);
		}
	}

	std::map<uint64_t, Fleet*> playerFleetMap;
	std::vector<uint64_t>      playerFleetList;

	for(u = 0; u < playerData->fleetList.size(); ++u)
	{
		Fleet *cFleet = playerData->fleetList[u];
		if( cFleet->ownerID == whichPlayer &&
		    cFleet->coordinates == fleetLocation )
		{
			playerFleetMap[cFleet->objectId] = cFleet;
			playerFleetList.push_back(cFleet->objectId);
		}
	}

	// figure out which fleets in each list don't match perfectly

	std::vector<Fleet*> nonMatchingGameFleetList;
	for(u = 0; u < gameFleetList.size(); ++u)
	{
      Fleet *cGFleet = gameFleetMap[gameFleetList[u]];
      Fleet *cPFleet = playerFleetMap[cGFleet->objectId];
      if( !isIdentical(cPFleet, cGFleet) )
         nonMatchingGameFleetList.push_back(cGFleet);
   }

	std::vector<Fleet*> nonMatchingPlayerFleetList;
   for(u = 0; u < playerFleetList.size(); ++u)
   {
      Fleet *cPFleet = playerFleetMap[playerFleetList[u]];
      Fleet *cGFleet = gameFleetMap[cPFleet->objectId];
      if( !isIdentical(cPFleet, cGFleet) )
         nonMatchingPlayerFleetList.push_back(cPFleet);
   }

	CargoManifest gameCargo; // collect master view of cargo
   std::map<uint64_t, shipMap*> gameMap;

	//foreach fleet in the master
   for(u = 0; u < nonMatchingGameFleetList.size(); ++u)
   {
      Fleet *cFleet = nonMatchingGameFleetList[u];

		// add in this fleet's cargo
      gameCargo = gameCargo + cFleet->cargoList;

		//foreach ship group in the fleet
      for(unsigned v = 0; v < cFleet->shipRoster.size(); ++v)
      {
         Ship *cShip = cFleet->shipRoster[v];

			// create or reuse an entry for this design
         shipMap *mapEntry = gameMap[cShip->designhullID];
         if( !mapEntry )
         {
            mapEntry = new shipMap();
            gameMap[cShip->designhullID] = mapEntry;
         }

			// update the map entry
         mapEntry->designID = cShip->designhullID;
         mapEntry->num += cShip->quantity;

         uint64_t incomingDamage = cShip->numberDamaged;
			incomingDamage *= cShip->percentDamaged;
         mapEntry->totalDamage += incomingDamage;
      }
   }

	CargoManifest playerCargo; // player view of cargo

	//foreach player fleet
   for(u = 0; u < nonMatchingPlayerFleetList.size(); ++u)
   {
      Fleet *cFleet = nonMatchingPlayerFleetList[u];

      playerCargo = playerCargo + cFleet->cargoList;

		//foreach ship group
      for(unsigned v = 0; v < cFleet->shipRoster.size(); ++v)
      {
         Ship *cShip = cFleet->shipRoster[v];

			// find the corresponding map entry for this design
         shipMap *mapEntry = gameMap[cShip->designhullID];
         if( !mapEntry )
         {
            // no ships of this type exist in the game map.  error
            starsExit("Fatal Error: player/master fleet data contradiction.",-1);
         }

			// subtract the player count from the total master count
			// (count should reach zero when they are reconciled)
         if( cShip->quantity > mapEntry->num )
            starsExit("Fatal Error: player/master fleet data contradiction.",-1);

         mapEntry->num -= cShip->quantity;

			// subtract off the player damage as well
         uint64_t outgoingDamage = cShip->numberDamaged;
			outgoingDamage *= cShip->percentDamaged;
         mapEntry->totalDamage -= outgoingDamage;
		}
	}

   if( playerCargo != gameCargo )
      starsExit("Fatal Error: player/master fleet cargo data contradiction.", -1);

	//foreach design map entry
   std::map<uint64_t,shipMap*>::iterator it = gameMap.begin();
   while( it != gameMap.end() )
   {
      shipMap *mapEntry = it->second;

      if( mapEntry->num != 0 )
         starsExit("Fatal Error: player/master fleet data contradiction.",-1);

      if( (mapEntry->totalDamage + 100) > 200 )
         starsExit("Fatal Error: player/master fleet data contradiction.",-1);

      delete mapEntry;
      it->second = NULL;

      ++it;
   }
}

bool validateDesign(GameData *gameData,Ship *theShip)
//primarily only validates that the design is OK in terms of slot usage, not
//meant as buildable
{
  

    unsigned HID = theShip->baseHullID;
    Ship *baseHull = gameData->hullList[HID-1];

    if (baseHull->slotList.size()!= theShip->slotList.size())
        return false;

    for (unsigned u=0;u<theShip->slotList.size();u++)
    {
        ShipSlot *base = baseHull->slotList[u];
        ShipSlot *design = theShip->slotList[u];
		  if( design->numberPopulated == 0 )
			  continue;

        if (design->numberPopulated> base->maxItems)
            return false;
        for (int i=0;i<numberOf_ELEMENT;i++)
        {
            if (base->acceptableElements[i]!=design->acceptableElements[i])
                return false;
        }
        StructuralElement *element = gameData->componentList[theShip->slotList[u]->structureID];

        //if (element->acceptableHulls[HID-1]==0)
            //return 0;

        structuralElementCategory tempVal = element->elementType;
        if (design->acceptableElements[tempVal]==false)
            return false;
    }

    return true;
}
/// attempt to reconcile and integrate player file changes
bool reconcilePlayerGameView(GameData *gameData, PlayerData *playerData)
{
	// not the same as playerData (server version)
	PlayerData *uCopy = gameData->getPlayerData(playerData->race->ownerID);

	unsigned i = 0; // MSVC bug

	//--- reconcile the design database----------------------------------------

	//foreach player design
	for(i = 0; i < playerData->designList.size(); ++i)
	{
		Ship *cDesign = playerData->designList[i];

                                if (!validateDesign(gameData,cDesign))
                                {

                                    cerr<<"Error: Player "
                                        <<playerData->race->singularName
                                        <<"("<<playerData->race->ownerID<<")"
                                        <<"design "
                                        <<cDesign->getName().c_str()
                                        <<"("<<cDesign->objectId<<") is invalid\n";
                                    starsExit("reconcile.cpp - validate existing design",-1);
                                }
		// seach the global list
		bool found = false;
		unsigned j = 0;
		while( j < gameData->playerDesigns.size() && !found )
		{
			if( ((Ship*)gameData->playerDesigns[j])->ownerID == playerData->race->ownerID &&
			    ((Ship*)gameData->playerDesigns[j])->objectId == cDesign->objectId )
			{
				//Ned? why?
				// replace existing design -- assume correct for now
				// add validation code here

				Ship *temp = gameData->playerDesigns[j];
			
                                string master;
                                string player;
                                temp->toXMLString(master);
                                cDesign->toXMLString(player);

                                if (master.compare(player)!=0) //are they different?
                                {
                                    if (gameData->getNumBuiltOfDesign(temp->objectId)==0)
                                    {
                                        gameData->playerDesigns[j] = cDesign;
                                        delete temp; // free the universe original copy
                                    }
                                    else
                                        cerr<<"Warning: Player "
                                            <<playerData->race->singularName
                                            <<"("<<playerData->race->ownerID<<")"
                                            <<"design "
                                            <<temp->getName().c_str()
                                            <<"("<<temp->objectId<<") is to a class with existing ships constructed!\n";
                                }
				found = true;
			}
			++j;
		}

		if( found ) // old design
			continue;
		//else new design

		// check object id
		if( !GameObject::isValidNewRange(cDesign->objectId,
		    playerData->race->ownerID) )
		{
			cerr << "Error processing client side player "
			     << playerData->race->ownerID << " designs\n"
			     << "Client side new object ID outside of valid range: "
			     << cDesign->objectId <<endl;
			starsExit("reconcile.cpp", -1);
		}

		// allocate a new id
		const uint64_t newID = GameObject::nextObjectId;
		++GameObject::nextObjectId;

		// set up the remapping
		gameData->objectIDRemapTable[cDesign->objectId] = newID;

		playerData->remappedObjectVector.push_back(cDesign->objectId);

		uCopy->remappedObjectVector.push_back(cDesign->objectId);

		// change the id
		cDesign->objectId = newID;
		cDesign->designhullID = newID;

		// accept it into the global design list
		//Ned? should it be a copy?
		gameData->playerDesigns.push_back(cDesign);
	}

	//--- planet data----------------------------------------------------------
	for(i = 0; i < playerData->planetList.size(); ++i)
	{
		Planet *pPlanet = playerData->planetList[i];
		Planet *gPlanet = gameData->getPlanetByID(pPlanet->objectId);
		// if player really owns planet
		if( gPlanet->ownerID != playerData->race->ownerID )
		{
			// if player thinks he owns it or master thinks the player should
			// know who owns it, but player doesn't
			if( pPlanet->ownerID == playerData->race->ownerID ||
			    (pPlanet->ownerID != gPlanet->ownerID &&
			     pPlanet->yearDataGathered == gameData->theUniverse->gameYear) )
			{
				cerr << "Error: owner ID mismatch when reconciling game data\n";
				starsExit("reconcile.cpp", -1);
			}
		}
		else // the player owns the planet, and the master agrees
		{
			// check cargo manifest
			if( pPlanet->cargoList != gPlanet->cargoList )
				starsExit("Fatal Error: player/master planet cargo contradiction.", -1);

			// propagate changes from 'pPlanet' to 'gPlanet'
			pPlanet->propagateData(gameData->objectIDRemapTable, gPlanet);
		}
	}

	//--- fleets---------------------------------------------------------------
	verifiedLocations.clear();

	// foreach player fleet
	for(i = 0; i < playerData->fleetList.size(); ++i)
	{
		Fleet *pFleet = playerData->fleetList[i];

                //validate ship designs embedded in the fleet match the actual design

                for (unsigned shipLCV=0;shipLCV<pFleet->shipRoster.size();shipLCV++)
                {
                        Ship *fleetHull = pFleet->shipRoster[shipLCV];
                        Ship *designHull = gameData->getDesignByID(fleetHull->designhullID);

                        if (fleetHull->slotList.size()!= designHull->slotList.size())
  			{
				cerr << "Error: slotsize mismatch in instanced ship\n"
                                     << "FleetID: "<<pFleet->objectId<<endl
                                     << "Ship: "<<fleetHull->getName()<<endl;
				starsExit("reconcile.cpp", -1);
			}

                        for (unsigned u=0;u<fleetHull->slotList.size();u++)
                        {
                            ShipSlot *base = designHull->slotList[u];
                            ShipSlot *design = fleetHull->slotList[u];

                            if (design->numberPopulated> base->numberPopulated)
                            {
				cerr << "Error: slot population count mismatch in instanced ship\n"
                                     << "FleetID: "<<pFleet->objectId<<endl
                                     << "Ship: "<<fleetHull->getName()<<endl;
				starsExit("reconcile.cpp", -1);
                            }
                            if (design->structureID> base->structureID)
                            {
				cerr << "Error: slot component mismatch in instanced ship\n"
                                     << "FleetID: "<<pFleet->objectId<<endl
                                     << "Ship: "<<fleetHull->getName()<<endl;
				starsExit("reconcile.cpp", -1);
                            }
                        }
                }

		// look for a global fleet
		bool found = false;
		unsigned j = 0;
		while( j < gameData->fleetList.size() && !found )
		{
			Fleet *gFleet = gameData->fleetList[j];

			if( gFleet->ownerID == playerData->race->ownerID &&
			    gFleet->objectId == pFleet->objectId )
			{
				found = true;

				// can't change debris or packets
				if( gFleet->myType == gameObjectType_fleet )
				{
					// check changes
					validateNewFleet(gameData, playerData, pFleet);

					gameData->fleetList[j] = pFleet;
					PlayerData *globPD = gameData->getPlayerData(pFleet->ownerID);
					size_t fid = 0;
					for(; fid < globPD->fleetList.size(); ++fid)
					{
						if( globPD->fleetList[fid]->objectId == pFleet->objectId )
							break;
					}
					globPD->fleetList[fid] = pFleet;

					pFleet->updateFleetVariables(*gameData);
					delete gFleet;
				}
			}
			++j;
		}

		if( found ) // old fleet
			continue;
		//else new fleet

		validateNewFleet(gameData, playerData, pFleet);

		// check id
		if( !GameObject::isValidNewRange(pFleet->objectId,
		    playerData->race->ownerID) )
		{
			cerr << "Error processing client side player "
			     << playerData->race->ownerID << " designs\n"
			     << "Client side new object ID outside of valid range: "
			     << pFleet->objectId <<endl;
			starsExit("reconcile.cpp", -1);
		}

		// allocate new id
		const uint64_t newID = GameObject::nextObjectId;
		++GameObject::nextObjectId;

		// set remapping
		gameData->objectIDRemapTable[pFleet->objectId] = newID;

		playerData->remappedObjectVector.push_back(pFleet->objectId);
		PlayerData *uCopy = gameData->getPlayerData(playerData->race->ownerID);
		uCopy->remappedObjectVector.push_back(pFleet->objectId);

		pFleet->objectId = newID;
		gameData->fleetList.push_back(pFleet);
	}

	//---research orders-------------------------------------------------------
	if( playerData->researchTechHoldback <= 100 )
		uCopy->researchTechHoldback = playerData->researchTechHoldback;

	if( playerData->researchTechID < numberOf_TECH )
		uCopy->researchTechID = playerData->researchTechID;
        if (playerData->researchNextID <numberOf_TECH)
            uCopy->researchNextID = playerData->researchNextID;

	const bool isGr = uCopy->race->lrtVector[generalizedResearch_LRT] == 1;
	const unsigned otherAmt = isGr ? 10 : 0;
	bool foundMain = false;
	for(i = 0; i < numberOf_TECH; ++i)
	{
		if( !foundMain && playerData->researchAllocation.techLevels[i] >= 50 )
		{
			foundMain = true;
			uCopy->researchAllocation.techLevels[i] = isGr ? 50 : 100;
		}
		else
			uCopy->researchAllocation.techLevels[i] = otherAmt;
	}
	if( !foundMain )
		starsExit("No primary research field given!", -1);

	//---player alliances------------------------------------------------------

	//---battle orders---------------------------------------------------------

	return true;
}

