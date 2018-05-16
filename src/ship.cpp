#ifdef MSVC
#pragma warning(disable: 4786)
#endif
/*****************************************************************************
                          ship.cpp -  description
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
#include <iostream>
#include <algorithm>
#include <cassert>
#include <math.h>

Ship::Ship(void)
{
   fleetable      = false;
   hasColonizer   = false;
   isSpaceStation = false;
   hasJumpGate = false;
   quantity       = 0;

   combatMove        = 0;
   combatRating      = 0;
   shieldMax         = 0;
   shields           = 0;
   armor             = 0;
   fuelCapacity      = 0;
   cargoCapacity     = 0;
   mass              = 0;
   baseResourceCost  = 0;
   totalResourceCost = 0;
   initiative        = 0;
   maxPacketSpeed    = 0;
   cloak             = 0;
   jam               = 0;
   fuelGen           = 0;
   damageRepair      = 0;
	numEngines        = 0;

   hullClass      = 0;
   baseHullID     = 0;
   designhullID   = 0;
   dockCapacity   = 0;
   percentDamaged = 0;
   numberDamaged  = 0;

   myType = gameObjectType_ship;
   for(unsigned i = 0; i < _numberOfMineTypes; i++)
      mineLayingRates[i]=0;
}

Ship::Ship(const Ship &b): GameObject(b)
{
	copyFrom(b, true);
}

Ship& Ship::operator=(const Ship &b)
{
	if( this == &b ) return *this;
	copyFrom(b, false);
	return *this;
}

void Ship::copyFrom(const Ship &b, bool newSlots)
{
	if( slotList.size() != b.slotList.size() )
	{
		for_each(slotList.begin(), slotList.end(), deleteT<ShipSlot>());
		slotList.clear();
		newSlots = true;
	}

	size_t i = 0;
   for(std::vector<ShipSlot*>::const_iterator vi = b.slotList.begin();
       vi != b.slotList.end(); ++vi, ++i)
	{
		if( newSlots )
	      slotList.push_back(new ShipSlot(**vi));
		else
			*(slotList[i]) = **vi;
	}

	name              = b.name;
	objectId          = b.objectId;
	ownerID           = b.ownerID;
	myType            = b.myType;
   designhullID      = b.designhullID;
   baseHullID        = b.baseHullID;
   quantity          = b.quantity;
   percentDamaged    = b.percentDamaged;
   numberDamaged     = b.numberDamaged;
   fleetable         = b.fleetable;
   hasColonizer      = b.hasColonizer;
   isSpaceStation    = b.isSpaceStation;
   combatMove        = b.combatMove;
   combatRating      = b.combatRating;
   shieldMax         = b.shieldMax;
   shields           = b.shields;
   armor             = b.armor;
   fuelCapacity      = b.fuelCapacity;
   cargoCapacity     = b.cargoCapacity;
   mass              = b.mass;
   baseResourceCost  = b.baseResourceCost;
   totalResourceCost = b.totalResourceCost;
   baseMineralCost   = b.baseMineralCost;
   totalMineralCost  = b.totalMineralCost;
   initiative        = b.initiative;
   baseInitiative    = b.baseInitiative;
   maxPacketSpeed    = b.maxPacketSpeed;
   cloak             = b.cloak;
   jam               = b.jam;
   fuelGen           = b.fuelGen;
   damageRepair      = b.damageRepair;
   warpEngine        = b.warpEngine;
   numEngines        = b.numEngines;
   dockCapacity      = b.dockCapacity;
   hullClass         = b.hullClass;
   acceptablePRTs    = b.acceptablePRTs;
   acceptableLRTs    = b.acceptableLRTs;
   techLevel         = b.techLevel;

   for(unsigned i = 0; i < _numberOfMineTypes; i++)
      mineLayingRates[i] = b.mineLayingRates[i];
}

void Ship::clearDesignData(void)
{
	clearPrivateData();
	quantity = 0;
}

void Ship::clearPrivateData(void)
{
	name = ""; // show no internal scan with empty name

   for(std::vector<ShipSlot*>::const_iterator vi = slotList.begin();
       vi != slotList.end(); ++vi)
	{
		 (**vi).numberPopulated = 0;
		 (**vi).structureID     = 0;
	}

   //quantity; // visible
	percentDamaged    = 0;
   numberDamaged     = 0;
   combatMove        = 0.0;
   combatRating      = 0;
   shieldMax         = 0;
   shields           = 0;
   armor             = 0;
   fuelCapacity      = 0;
   cargoCapacity     = 0;
   baseResourceCost  = 0;
   totalResourceCost = 0;
   baseMineralCost.clear();
   totalMineralCost.clear();
   initiative        = 0;
   baseInitiative    = 0;
   maxPacketSpeed    = 0;
   cloak             = 0;
   jam               = 0;
   fuelGen           = 0;
   damageRepair      = 0;
	warpEngine.clear();
   numEngines        = 0;
   for(unsigned i = 0; i < _numberOfMineTypes; i++)
      mineLayingRates[i] = 0;
}

bool Ship::parseHullSlots(mxml_node_t *tree, GameData &gd)
{
  mxml_node_t *child = NULL;
  child = mxmlFindElement(tree,tree,"HULLSLOTS",NULL,NULL,MXML_DESCEND);
  child = child->child;
  string sList[MAX_NUM_SLOTS];
  int i=0;
  int tempVal = 0;

  int numEl = assembleNodeArray(child, sList, MAX_NUM_SLOTS);
  if (numEl == -1)
  {
      cerr<<"Error Parsing Hull Slots for ship: "<<name<<endl;
      return false;
  }


  ShipSlot *newSlot = NULL;
  int parserState = 0; //0 - newSlot; 1 - parse number ; 2 - parse slot types
  int numSlots = 1;

  for(unsigned j = 0; j < _numberOfMineTypes; j++)
     mineLayingRates[j] = 0;
  shieldMax = 0;

  i = 0;
  while( i < numEl )
    {
      switch (parserState)
        {
        case 0:
          newSlot = new ShipSlot();
          parserState = 1;
          newSlot->slotID = numSlots++;
          break;
        case 1:
          tempVal = atoi(sList[i].c_str());
          newSlot->numberPopulated = tempVal;
          parserState = 2;
          i++;
          break;
        case 2:
          {
            newSlot->structureID = 0;
            if (newSlot->numberPopulated != 0)
            {
                tempVal = atoi(sList[i].c_str());
                newSlot->structureID = tempVal;
                StructuralElement *el = gd.componentList[tempVal];
                if (el->colonizerModule.isActive)
                hasColonizer = true;
                if (maxPacketSpeed < el->packetAcceleration)
                    maxPacketSpeed = el->packetAcceleration;
                if (el->engine.isActive) //did we just parse the engine?
                    numEngines = newSlot->numberPopulated; //save numEngines for mine collision use
                if (el->minelayer.isActive)
                for (unsigned i=0;i<_numberOfMineTypes;i++)
                    mineLayingRates[i]+= el->minelayer.mineLayingRates[i]*newSlot->numberPopulated;
            shieldMax += el->shields * newSlot->numberPopulated;
            }
            parserState = 3;
            i++;
            break;
          }
        case 3:
          tempVal = atoi(sList[i].c_str());
          newSlot->maxItems = tempVal;
          parserState = 4;
          i++;
          break;
        case 4:
          if(sList[i] == ":")
            {
              parserState = 0;
              slotList.push_back(newSlot);
            }
          else if (sList[i]!="")
            {
              tempVal = structuralElementCodeMap[sList[i].c_str()];
              newSlot->acceptableElements[(structuralElementCategory)tempVal] = true;
              if (tempVal == orbital_ELEMENT)
                isSpaceStation = true;
            }

          i++;
          break;
        default:
          cerr<<"State machine error in parsing Hull Slots for ship: "<<name<<endl;
          return false;
        }
    }
  slotList.push_back(newSlot); //push in the last one, it is non terminated by a :

  return true;
}

bool Ship::parseXML(mxml_node_t *tree, GameData &gd)
{
  mxml_node_t *child = NULL;
  unsigned valArray[numberOf_hull];
  int tempInt = 0;

  string tempString;


  GameObject::parseXML(tree);
  if (name == "base game object")
    return false;

  parseXML_noFail(tree,"HULLCLASS",hullClass);
  parseXML_noFail(tree,"BASEHULLID",baseHullID);
  parseXML_noFail(tree,"DESIGNHULLID",designhullID);
  parseXML_noFail(tree,"PERCENTDAMAGED",percentDamaged);
  parseXML_noFail(tree,"NUMBERDAMAGED",numberDamaged);

  child = mxmlFindElement(tree,tree,"ACCEPTABLEPRTS",NULL,NULL,MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, missing acceptable PRT list for component: " << name << endl;
      return false;
    }
  child = child->child;

  tempInt = assembleNodeArray(child, valArray, numberOf_PRT);
  if( tempInt != numberOf_PRT )
    {
      cerr << "Error, incomplete acceptable PRT list for component: " << name << endl;
      cerr << "Expecting "<<numberOf_PRT<<" entries, found " << tempInt << endl;
      return false;
    }

  for(tempInt = 0; tempInt < numberOf_PRT; tempInt++)
    acceptablePRTs.push_back(valArray[tempInt]);

  //------------------
  child = mxmlFindElement(tree, tree, "ACCEPTABLELRTS", NULL, NULL, MXML_DESCEND);
  if( !child )
    {
      cerr << "Error, missing acceptable LRT list for component: " << name << endl;
      return false;
    }
  child = child->child;

  tempInt = assembleNodeArray(child, valArray, numberOf_LRT);
  for(tempInt = 0;tempInt < numberOf_LRT; tempInt++)
    acceptableLRTs.push_back(valArray[tempInt]);
  if( tempInt != numberOf_LRT )
    {
      cerr << "Error, incomplete acceptable LRT list for component: " << name << endl;
      cerr << "Expecting "<<numberOf_LRT<<" entries, found " << tempInt << endl;
      acceptableLRTs.push_back(2);//patch up here as a fix for cheap engine ommision.  should be removed after
      //the fixup is complete.
//      return false;
    }


  parseXML_noFail(tree, "ARMOR",armor);

  parseXML_noFail(tree, "INITIATIVE",initiative);
  parseXML_noFail(tree, "BASEINITIATIVE",baseInitiative);
  parseXML_noFail(tree, "CARGOCAPACITY",cargoCapacity);
  parseXML_noFail(tree, "CLOAK",cloak);
  parseXML_noFail(tree, "FUELCAPACITY",fuelCapacity);
  parseXML_noFail(tree, "MASS",mass);
  baseMineralCost.parseXML(tree,"MINERALCOSTBASE");
  totalMineralCost.parseXML(tree,"TOTALMINERALCOST");
  parseXML_noFail(tree, "BASERESOURCECOST",baseResourceCost);
  parseXML_noFail(tree,"TOTALRESOURCECOST",totalResourceCost);
  techLevel.parseXML(tree,"STARSTECHLEVEL");
  parseXML_noFail(tree, "DOCKCAPACITY",dockCapacity);
  parseXML_noFail(tree, "FLEETABLE",fleetable);
  parseXML_noFail(tree,"QUANTITY", quantity);
  parseXML_noFail(tree,"COMBATMOVE",combatMove);
  parseXML_noFail(tree,"COMBATRATING",combatRating);
  parseXML_noFail(tree,"SHIELDMAX",shieldMax);
  parseXML_noFail(tree,"SHIELDS",shields);
  parseXML_noFail(tree,"JAM",jam);
  parseXML_noFail(tree,"FUELGEN",fuelGen);
  parseXML_noFail(tree,"DAMAGEREPAIR",damageRepair);
  warpEngine.parseXML(tree);

  //      <HULLSLOTS>ENG : 1 S A : 1 S E M : 1</HULLSLOTS>

  //parse the slots, figure out things like do I have a flinger,
  //am I a space station, etc,etc
  parseHullSlots(tree, gd);

  return true;
}

void Ship::toCPPString(string &theString, GameData &gameData)
{
  string tempString;
  theString = "Ship Hull:\t\t";

  theString += name;
  theString += "\n";
  char ctemp[1024];

  sprintf(ctemp,"ObjectID: %lu\tOwnerID: %i\tHullClass: %i\n",objectId,ownerID,hullClass);
  theString += ctemp;

  prtLrtCPPString(tempString,acceptablePRTs,acceptableLRTs);
  theString += tempString;

  sprintf(ctemp,"Armor DPs: %i\tShield Max: %i\tShields: %i\n",armor,shieldMax,shields);
  theString += ctemp;

  sprintf(ctemp,"Jam: %i\tCargoCapacity: %i\tCloakBase: %i\n",jam,cargoCapacity,cloak);
  theString += ctemp;

  sprintf(ctemp,"Quantity: %i\tCombatMove: %1.2f\tCombatRating: %i\n",quantity,combatMove,combatRating);
  theString += ctemp;

  sprintf(ctemp,"FuelCap: %i\tFuelGen: %i\tMass: %i\n",fuelCapacity,fuelGen,mass);
  theString += ctemp;

  sprintf(ctemp,"BaseInit: %i\tInit: %i\n",baseInitiative,initiative);
  theString += ctemp;

  theString += "Base Mineral Cost\n";
  baseMineralCost.toCPPString(tempString);
  theString += tempString;

  theString += "Total Mineral Cost\n";
  totalMineralCost.toCPPString(tempString);
  theString += tempString;

  techLevel.toCPPString(tempString);
  theString += tempString;

  sprintf(ctemp,"Base Res. Cost: %i\tTotalResourceCost: %i\n",baseResourceCost,totalResourceCost);
  theString += ctemp;

  sprintf(ctemp,"Repair: %i\tTechTreeID(BaseHullID): %i\tDesignHullID: %lu\n",damageRepair,baseHullID,designhullID);
  theString += ctemp;

  sprintf(ctemp,"Docking Cap.: %i\tFleeting Status: %i\tOwnerID: %i\n",dockCapacity,fleetable,ownerID);
  theString += ctemp;

  theString += "HULLSLOTS: ";
  for (unsigned i = 0;i<slotList.size();i++)
    {
      ShipSlot *slot = slotList[i];
      string comName = "empty";
      if (slot->numberPopulated > 0 )
        comName = gameData.componentList[slot->structureID]->name;
      sprintf(ctemp,"%i %i(%s) of %i ",slot->numberPopulated, slot->structureID, comName.c_str() ,slot->maxItems);
      theString += ctemp;
      for (int j = 0;j< (int) numberOf_ELEMENT;j++)
        if (slot->acceptableElements[j] == true)
          {
            theString += structuralElementCategoryStrings[j];
            theString += " ";
          }
      if (i+1 <slotList.size())
        theString+=", ";
    }
  theString += "\n";
  warpEngine.toCPPString(tempString);
  theString += tempString;
  theString += "\n\n";
}


void Ship::toXMLString(string &theString)
{
  char tChar[256];

  theString = "";
  string tempString = "<HULL>\n";

  theString += tempString;

  GameObject::toXMLStringGuts(tempString);
  theString += tempString;

  theString += "<FLEETABLE>";
  int tempInt = 0;
  if (fleetable)
    tempInt = 1;
  sprintf(tChar,"%i ",tempInt);
  theString += tChar;
  theString += "</FLEETABLE>\n";

  sprintf(tChar,"<QUANTITY>%i</QUANTITY>\n",quantity);
  theString += tChar;
  sprintf(tChar,"<COMBATMOVE>%1.2f</COMBATMOVE>\n",combatMove);
  theString += tChar;
  sprintf(tChar,"<COMBATRATING>%i</COMBATRATING>\n",combatRating);
  theString += tChar;
  sprintf(tChar,"<SHIELDMAX>%i</SHIELDMAX>\n",shieldMax);
  theString += tChar;
  sprintf(tChar,"<SHIELDS>%i</SHIELDS>\n",shields);
  theString += tChar;

  sprintf(tChar,"<ARMOR>%i</ARMOR>\n",armor);
  theString += tChar;
  sprintf(tChar,"<FUELCAPACITY>%i</FUELCAPACITY>\n",fuelCapacity);
  theString += tChar;
  sprintf(tChar,"<CARGOCAPACITY>%i</CARGOCAPACITY>\n",cargoCapacity);
  theString += tChar;
  sprintf(tChar,"<MASS>%i</MASS>\n",mass);
  theString += tChar;
  sprintf(tChar,"<BASERESOURCECOST>%i</BASERESOURCECOST>\n",baseResourceCost);
  theString += tChar;
  sprintf(tChar,"<TOTALRESOURCECOST>%i</TOTALRESOURCECOST>\n",totalResourceCost);
  theString += tChar;
  baseMineralCost.toXMLString(tempString,"MINERALCOSTBASE","SCALE=\"KiloTons\"");
  theString += tempString;
  totalMineralCost.toXMLString(tempString,"TOTALMINERALCOST","SCALE=\"KiloTons\"");
  theString += tempString;
  sprintf(tChar,"<INITIATIVE>%i</INITIATIVE>\n",initiative);
  theString += tChar;
  sprintf(tChar,"<BASEINITIATIVE>%i</BASEINITIATIVE>\n",baseInitiative);
  theString += tChar;


  sprintf(tChar,"<CLOAK>%i</CLOAK>\n",cloak);
  theString += tChar;
  sprintf(tChar,"<JAM>%i</JAM>\n",jam);
  theString += tChar;
  sprintf(tChar,"<FUELGEN>%i</FUELGEN>\n",fuelGen);
  theString += tChar;
  sprintf(tChar,"<DAMAGEREPAIR>%i</DAMAGEREPAIR>\n",damageRepair);
  theString += tChar;

  warpEngine.toXMLString(tempString);
  theString += tempString;

  sprintf(tChar,"<HULLCLASS>%i</HULLCLASS>\n",hullClass);
  theString += tChar;
  sprintf(tChar,"<BASEHULLID>%i</BASEHULLID>\n",baseHullID);
  theString += tChar;
  sprintf(tChar,"<DESIGNHULLID>%lu</DESIGNHULLID>\n",designhullID);
  theString += tChar;
  sprintf(tChar,"<PERCENTDAMAGED>%i</PERCENTDAMAGED>\n",percentDamaged);
  theString += tChar;
  sprintf(tChar,"<NUMBERDAMAGED>%i</NUMBERDAMAGED>\n",numberDamaged);
  theString += tChar;
  sprintf(tChar,"<DOCKCAPACITY>%i</DOCKCAPACITY>\n",dockCapacity);
  theString += tChar;

  techLevel.toXMLString(tempString,"STARSTECHLEVEL");
  theString += tempString;

  theString += "<HULLSLOTS>";
  for (unsigned i = 0;i<slotList.size();i++)
    {
      if (i>0)
        theString += " : ";
      ShipSlot *slot = slotList[i];
      sprintf(tChar,"%i %i %i ",slot->numberPopulated, slot->structureID, slot->maxItems);
      theString += tChar;
      bool preSpace = false;
      for (int j = 0;j< (int) numberOf_ELEMENT;j++)
        if (slot->acceptableElements[j] == true)
          {
            if (preSpace)
              theString += " ";
            preSpace = true;
            theString += structuralElementCategoryCodes[j];
          }
    }
  theString += "</HULLSLOTS>\n";


  vectorToString(tempString,acceptablePRTs);
  theString += "<ACCEPTABLEPRTS>" + tempString + "</ACCEPTABLEPRTS>\n";

  vectorToString(tempString,acceptableLRTs);
  theString += "<ACCEPTABLELRTS>" + tempString + "</ACCEPTABLELRTS>\n";


  theString += "</HULL>\n";
}

//update mineral and resource costs...
void Ship::updateCostAndMass(GameData &gameData)
{
   Ship *baseHull = gameData.hullList[baseHullID-1];

   CargoManifest mineralCost = baseHull->baseMineralCost;

   unsigned resourceCost = baseHull->baseResourceCost;
   unsigned massSum      = baseHull->mass;
   unsigned armorSum     = baseHull->armor;
   unsigned shieldSum    = baseHull->shields;
   unsigned thrustBonus  = 0; //bonus for thrusters, jets, etc
   unsigned initSum      = baseHull->baseInitiative;

   fleetable      = baseHull->fleetable;
   isSpaceStation = baseHull->isSpaceStation;
   fuelCapacity   = baseHull->fuelCapacity;
   cargoCapacity  = baseHull->cargoCapacity;
   fuelGen        = 0;
   damageRepair   = baseHull->damageRepair;
   dockCapacity   = baseHull->dockCapacity;

   //need to add cloak and Jam sum here
   //need to add combat rating sum here

   //should be redundant in the final game -- only for updating
   //xml files as the code evolves...
   hullClass = baseHull->hullClass;

	const Race *const owner = gameData.getRaceById(ownerID);
	const bool isWM = owner ? owner->prtVector[warMonger_PRT] != 0 : false;

   for(unsigned i = 0; i < slotList.size(); i++)
   {
      ShipSlot *cSlot = slotList[i];
      StructuralElement * element = gameData.componentList[cSlot->structureID];
		//Ned, need PRT cost adjustments...

		mineralCost   = mineralCost + element->mineralCostBase  * cSlot->numberPopulated; //Ned, need +=

		resourceCost  += element->resourceCostBase * cSlot->numberPopulated;
      massSum       += element->mass             * cSlot->numberPopulated;
      armorSum      += element->armor            * cSlot->numberPopulated;
      shieldSum     += element->shields          * cSlot->numberPopulated;
      thrustBonus   += element->thrust           * cSlot->numberPopulated;
      fuelGen       += element->fuelGen          * cSlot->numberPopulated;
      fuelCapacity  += element->fuelCapacity     * cSlot->numberPopulated;
      cargoCapacity += element->cargoCapacity    * cSlot->numberPopulated;

      if( element->engine.isActive )
      {
         numEngines = cSlot->numberPopulated;
         warpEngine = element->engine;
      }

      if( !(element->beam.isActive) && !(element->missle.isActive) )
         initSum += element->baseInitiative * cSlot->numberPopulated;
      if (element->jumpgate.isActive==true)
          hasJumpGate = true;
   }

   baseMineralCost  = mineralCost;
   baseResourceCost = resourceCost;
   mass             = massSum;
   armor            = armorSum;
   shields          = shieldSum;
   shieldMax        = shieldSum;
   initiative       = initSum;

	combatMove = 0.0;
   if( numEngines != 0 )
	{
	   double massMoveComponent = ((double)mass / 280.0 )/(double)numEngines;

		combatMove = ((double)warpEngine.battleSpeed - 4.0)/4.0 -
	                  massMoveComponent + (double)thrustBonus/4.0 + 0.5;
		combatMove *= 4.0;
	   combatMove = ceil(combatMove);
		combatMove /= 4.0;
		combatMove = std::max(0.5, combatMove);
		if( isWM ) combatMove += 0.5;
	}

   //now normally we would do some scaling for tech levels here.
   //but this isn't in yet...

   totalMineralCost = mineralCost;
   totalResourceCost = resourceCost;

	if( isSpaceStation )
	{
		// costs are halved
		totalMineralCost  = totalMineralCost / 2;
		baseMineralCost   = baseMineralCost  / 2;

		totalResourceCost >>= 1;
		baseResourceCost  >>= 1;

		mass = 0; // no mass
	}
}

bool Ship::isBuildable(const PlayerData *pd) const
{
	//Ned, code copied from StructuralElement::isBuildable
	// make sure the user has enough tech
	if( !pd->currentTechLevel.meetsTechLevel(techLevel) )
		return false;

	// check PRT limitations
	for(size_t i = 0; i < numberOf_PRT; ++i)
	{
		// if we find our PRT, and that PRT is not acceptable
		if( pd->race->prtVector[i] == 1 && acceptablePRTs[i] == 0 )
			return false;
	}

	// check LRT limitations
	for(size_t j = 0; j < numberOf_LRT; ++j)
	{
		switch( acceptableLRTs[j] )
		{
		case 0: // you have this LRT, you are forbidden
			if( pd->race->lrtVector[j] == 1 )
				return false;
			break;

		case 1: // you must have this LRT to build it
			if( pd->race->lrtVector[j] != 1 )
				return false;
			break;

		case 2: // don't care
			break;

		default:
			std::cerr << "Component with bad lrt vector value: "
			    << acceptableLRTs[j] << '\n';
		}
	}

   return true;
}

ScanComponent Ship::getScan(const PlayerData &pd) const
{
	bool hasScan = false;
	double scan = 0.0;
	double penScan = 0.0;

	if( pd.race->primaryTrait == jackOfAllTrades_PRT )
	{
		HullTypes ht = HullTypes(baseHullID - 1);
		// joat get built in scanner on scout, frigate, and destroyer
		if( ht == frigate_hull || ht == scout_hull ||
		    ht == destroyer_hull )
		{
			hasScan = true;

			// fourth power, for later quarter root
			scan = pd.currentTechLevel.techLevels[electronics_TECH] * 20;
			scan *= scan;
			scan *= scan;

			penScan = pd.currentTechLevel.techLevels[electronics_TECH] * 10;
			penScan *= penScan;
			penScan *= penScan;
		}
	}

	// foreach slot on this ship type (regardless of count)
	for(size_t j = 0; j < slotList.size(); ++j)
	{
		ShipSlot *ss = slotList[j];
		if( ss->numberPopulated == 0 ) // if empty slot
			continue;

		// get the component
		StructuralElement *sep = pd.gd_->componentList[ss->structureID];
		if( !sep->scanner.isActive ) // if not a scanner
			continue;

		hasScan = true; // any scanner is enough
		// combine at fourth power for later fourth root
		scan += sep->scanner.baseScanRange * sep->scanner.baseScanRange *
		    sep->scanner.baseScanRange * sep->scanner.baseScanRange;
		penScan += sep->scanner.penScanRange * sep->scanner.penScanRange *
		    sep->scanner.penScanRange * sep->scanner.penScanRange;
	}

	ScanComponent ret;
	ret.isActive = hasScan;

	// scanners combine at the fourth root
	ret.baseScanRange = unsigned(sqrt(sqrt(scan)));
	ret.penScanRange  = unsigned(sqrt(sqrt(penScan)));

	if( pd.race->lrtVector[noAdvancedScanners_LRT] == 1 )
		ret.baseScanRange *= 2;

	return ret;
}

int Ship::calcFuel(const Location &coord, const Location &dest,
    unsigned spd, unsigned cargo) const
{
	Location fuelLimit = coord;
	Location oldPlace  = coord;

	//Ned? scoops?
	const unsigned engEff = warpEngine.fuelConsumptionVector[spd];

	int fuelCon = 0;
	do
	{
		fuelLimit.moveToward(dest, spd * spd);
		fuelLimit.finalizeLocation();

		double dist = fuelLimit.actualDistanceFrom(oldPlace);
		oldPlace = fuelLimit;

		// fuel consumption formula (fixed from manual)
		fuelCon += int(
		    (double(mass + cargo) * engEff * dist / 200.0 + 9.0) / 100.0);
	} while( dest != fuelLimit );

	return fuelCon;
}

bool Ship::operator==(const Ship &param) const
{
   bool rval = true;
   rval = rval && (baseHullID     == param.baseHullID);
   rval = rval && (quantity       == param.quantity);
   rval = rval && (numberDamaged  == param.numberDamaged);
   rval = rval && (percentDamaged == param.percentDamaged);

   return rval;
}
