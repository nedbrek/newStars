#ifdef MSVC
#pragma warning(disable: 4786)
#endif
//
// C++ Implementation: structuralElement
//
// Copyright: See COPYING file that comes with this distribution
//
#include "newStars.h"
#include "classDef.h"
#include "race.h"
#include "utils.h"
#include <iostream>

string PRT_NAMES[] =
    {
        "Jack of all Trades",
        "Alternate Reality",
        "Interstellar Traveller",
        "Packet Physicist",
        "Space Demolition",
        "Inner Strength",
        "Claim Adjuster",
        "Warmonger",
        "Super Stealth",
        "Hyper-Expansionist",
        "ERROR"
    };
string HULL_NAMES[]=
    {
        "Small Freighter",
        "Medium Freighter",
        "Large Freighter",
        "Super Freighter",
        "Scout",
        "Frigate",
        "Destroyer",
        "Cruiser",
        "BattleCruiser",
        "Battleship",
        "Dreadnought",
        "Privateer",
        "Rogue",
        "Galleon",
        "MiniColonizer",
        "Colonizer",
        "Mini-Bomber",
        "B-17 Bomber",
        "Stealth Bomber",
        "B-52 Bomber",
        "Midget-Miner",
        "Mini-Miner",
        "Miner",
        "Maxi-Minor",
        "Ultra-Minor",
        "Fuel Transport",
        "Super Fuel Transport",
        "Mini-Minelayer",
        "Super-Minelayer",
        "Nubian",
        "Metamorph",
        "ERROR"
    };

string LRT_NAMES[] =
    {
        "Improved Fuel Efficiency",
        "Total Terraforming",
        "Advanced Remote Mining",
        "Improced Starbases",
        "Generalized Research",
        "Ultimate Recycling",
        "Mineral Alchemy",
        "No Ram Scoop Engines",
        "Only Basic Remote Mining",
        "No Advanced Scanners",
        "Low Starting Population",
        "Bleeding Edge Technology",
        "Regenerating Shields",
        "ERROR"
    };

unsigned StructuralElement::nextID = 0;
char cTemp[4096];

bool StructuralElement::parseXML(mxml_node_t *tree)
{
    if( !parseXML_noFail(tree, "NAME", name) )
    {
        cerr << "Error parsing component, no name found\n";
        return false;
    }

    if( !techLevel.parseXML(tree,"STARSTECHLEVEL") )
    {
        cerr << "Error parsing component " << name << ", no tech level found\n";
        return false;
    }

    if( !mineralCostBase.parseXML(tree,"MINERALCOSTBASE") )
    {
        cerr << "Error parsing component " << name << ", no mineral cost found\n";
        return false;
    }

    if( !parseXML_noFail(tree, "RESOURCECOSTBASE", armor) )
    {
        cerr << "Error parsing component " << name << ", no resource cost found\n";
        return false;
    }

    if( !parseXML_noFail(tree,"ELEMENTTYPE",(int&)elementType) )
    {
        cerr << "Error parsing component " << name << ", no element type found\n";
        return false;
    }

    parseXML_noFail(tree, "ARMOR",armor);
    parseXML_noFail(tree, "SHIELDS",shields);
    parseXML_noFail(tree, "FUELCAPACITY",fuelCapacity);
    parseXML_noFail(tree, "CARGOCAPACITY",cargoCapacity);
    parseXML_noFail(tree, "MASS",mass);
    parseXML_noFail(tree, "BASEINITIATIVE",baseInitiative);
    parseXML_noFail(tree, "CLOAK",cloak);
    parseXML_noFail(tree, "JAM",jam);
    parseXML_noFail(tree, "REMOTEMINEING",remoteMineing);
    parseXML_noFail(tree, "FUELGEN",fuelGen);
    parseXML_noFail(tree, "PACKETACCELERATION",packetAcceleration);
    parseXML_noFail(tree, "BEAMREFLECTION",beamReflection);
    parseXML_noFail(tree, "ENERGYCAPACITANCE",energyCapacitance);
    parseXML_noFail(tree, "ACCURACYIMPROVEMENT",accuracyImprovement);
    parseXML_noFail(tree, "ENERGYDAMPENING",energyDampening);
    parseXML_noFail(tree, "TACHYONDETECTION",tachyonDetection);
    parseXML_noFail(tree, "FLEETTHEFTCAPABLE",fleetTheftCapable);
    parseXML_noFail(tree, "PLANETTHEFTCAPABLE",planetTheftCapable);
    parseXML_noFail(tree, "REMOTETERRAFORMCAPABLE",remoteTerraformCapable);
    parseXML_noFail(tree, "THRUST",thrust);
    parseXML_noFail(tree, "TECHTREEID",techTreeID);
    parseXML_noFail(tree, "DEFENSE",defense);
    parseXML_noFail(tree, "PLANETMINERATE",planetMineRate);
    parseXML_noFail(tree, "RESOURCECOSTBASE",resourceCostBase);
    parseXML_noFail(tree, "RESOURCEGENRATE",resourceGenRate);
    parseXML_noFail(tree, "MINERALGENRATE",mineralGenRate);

    // parse terraforming ability
    mxml_node_t *child = mxmlFindElement(tree,tree,"TFABILITY",NULL,NULL,MXML_DESCEND);
    if( !child )
    {
        terraformAbility.grav = 0;
        terraformAbility.temp = 0;
        terraformAbility.rad  = 0;
    }
    else
    {
        unsigned valArray[128];
        int valCounter = 0;

        valCounter = assembleNodeArray(child->child, valArray, 3);
        if( valCounter != 3 )
        {
            std::cerr << "Error, found " << valCounter
				    << " terraformAbility values (expecting 3) for element:"
					 << name << std::endl;
            return false;
        }
        terraformAbility.grav = valArray[0];
        terraformAbility.temp = valArray[1];
        terraformAbility.rad  = valArray[2];
    }

    child = mxmlFindElement(tree, tree, "ACCEPTABLEHULLS", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        cerr << "Error, missing acceptable hull list for component: " << name << endl;
        return false;
    }
    child = child->child;

    unsigned valArray[numberOf_hull];
    int tempInt = assembleNodeArray(child, valArray, numberOf_hull);
    if( tempInt != numberOf_hull )
    {
        cerr << "Error, incomplete acceptable hull list for component: " << name << endl;
        cerr << "Expecting " << numberOf_hull << " entries, found " << tempInt << endl;
        return false;
    }
    for(tempInt = 0; tempInt < numberOf_hull; tempInt++)
        acceptableHulls.push_back(valArray[tempInt]);

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
    if( tempInt != numberOf_LRT )
    {
        cerr << "Error, incomplete acceptable LRT list for component: " << name << endl;
        cerr << "Expecting "<<numberOf_LRT<<" entries, found " << tempInt << endl;
        return false;
    }

    for(tempInt = 0;tempInt < numberOf_LRT; tempInt++)
        acceptableLRTs.push_back(valArray[tempInt]);

    minelayer.parseXML(tree);
    engine.parseXML(tree);
    beam.parseXML(tree);
    missle.parseXML(tree);
    scanner.parseXML(tree);
    jumpgate.parseXML(tree);
    bomb.parseXML(tree);
    colonizerModule.parseXML(tree);

    return true;
}

bool ColonizerModuleComponent::parseXML(mxml_node_t *tree)
{
    isActive = false;

    tree = mxmlFindElement(tree, tree, "COLONIZERMODULECOMPONENT",
                           NULL, NULL, MXML_DESCEND);
    if( !tree )
        return true;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;

    if( !isActive )
        return true;

    unsigned tmp;
    isActive = parseXML_noFail(tree, "COLONIZERTYPE", tmp);
    colonizerType = static_cast<ColonizerType>(tmp);

    return isActive;
}
void ColonizerModuleComponent::toXMLString(string &theString)
{
    sprintf (cTemp,"<COLONIZERMODULECOMPONENT>\n\t<ISACTIVE>%i</ISACTIVE>\n\t<COLONIZERTYPE>%i</COLONIZERTYPE>\n</COLONIZERMODULECOMPONENT>\n", isActive,colonizerType);
    theString   = cTemp;
}


bool JumpgateComponent::parseXML(mxml_node_t *tree)
{
    isActive = false;
    tree = mxmlFindElement(tree, tree, "JUMPGATECOMPONENT", NULL, NULL, MXML_DESCEND);
    if( !tree )
        return true;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;
    if( !isActive )
        return true;

    bool flag = true;

    flag = flag && parseXML_noFail(tree,"DISTANCELIMIT",distanceLimit);
    flag = flag && parseXML_noFail(tree,"INFINITEDISTANCE",infiniteDistance);
    flag = flag && parseXML_noFail(tree,"INFINITEMASS",infiniteMass);
    flag = flag && parseXML_noFail(tree,"MASSLIMIT",massLimit);
    flag = flag && parseXML_noFail(tree,"USETARGETGATESPECS",useTargetGateSpecs);
    isActive = flag;
    return flag;
}
void JumpgateComponent::toXMLString(string &theString)
{
    sprintf(cTemp,"<JUMPGATECOMPONENT>\n\t<ISACTIVE>%i</ISACTIVE>\n\t<DISTANCELIMIT>%i</DISTANCELIMIT>\n\t<INFINITEDISTANCE>%i</INFINITEDISTANCE>\n\t<INFINITEMASS>%i</INFINITEMASS>\n\t<MASSLIMIT>%i</MASSLIMIT>\n\t<USETARGETGATESPECS>%i</USETARGETGATESPECS>\n\t</JUMPGATECOMPONENT>\n", isActive, distanceLimit,  infiniteDistance, infiniteMass, massLimit, useTargetGateSpecs);
    theString = cTemp;
}

bool BombComponent::parseXML(mxml_node_t *tree)
{
    isActive = false;
    tree = mxmlFindElement(tree, tree, "BOMBCOMPONENT", NULL, NULL, MXML_DESCEND);
    if( !tree )
        return false;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;
    if( !isActive )
        return true;

    bool flag = true;

    flag = flag && parseXML_noFail(tree, "INFRASTRUCTUREKILLRATE", infrastructureKillRate);
    flag = flag && parseXML_noFail(tree, "ISSMART"    , isSmart);
    flag = flag && parseXML_noFail(tree, "MINPOPKILL" , minPopKill);
    flag = flag && parseXML_noFail(tree, "POPKILLRATE", popKillRate);
    isActive = flag;
    return flag;
}

void BombComponent::toXMLString(string &theString)
{
    sprintf(cTemp,"<BOMBCOMPONENT>\n\t<ISACTIVE>%i</ISACTIVE>\n\t<INFRASTRUCTUREKILLRATE>%i</INFRASTRUCTUREKILLRATE>\n\t<ISSMART>%i</ISSMART>\n\t<MINPOPKILL>%i</MINPOPKILL>\n\t<POPKILLRATE>%i</POPKILLRATE>\n</BOMBCOMPONENT>\n",isActive,infrastructureKillRate,isSmart,minPopKill,popKillRate);
    theString = cTemp;
}

bool ScanComponent::parseXML(mxml_node_t *tree)
{
    isActive = false;
    tree = mxmlFindElement(tree,tree,"SCANCOMPONENT",NULL,NULL,MXML_DESCEND);
    if( !tree )
        return false;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;
    if( !isActive )
        return true;

    bool flag = true;

    flag = flag && parseXML_noFail(tree, "BASESCANRANGE", baseScanRange);
    flag = flag && parseXML_noFail(tree, "PENSCANRANGE" , penScanRange);
    isActive = flag;
    return flag;
}

void ScanComponent::toXMLString(string &theString)
{
    sprintf(cTemp,"<SCANCOMPONENT>\n\t<ISACTIVE>%i</ISACTIVE>\n\t<BASESCANRANGE>%i</BASESCANRANGE>\n\t<PENSCANRANGE>%i</PENSCANRANGE>\n\t</SCANCOMPONENT>\n",isActive,baseScanRange,penScanRange);
    theString = cTemp;
}

bool MissleWeaponComponent::parseXML(mxml_node_t *tree)
{
    isActive = false;
    tree = mxmlFindElement(tree, tree, "MISSLEWEAPONCOMPONENT", NULL, NULL, MXML_DESCEND);
    if( !tree )
        return false;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;
    if( !isActive )
        return true;

    bool flag = true;

    flag = flag && parseXML_noFail(tree, "ACCURACY", accuracy);
    flag = flag && parseXML_noFail(tree, "RANGE"   , range);
    flag = flag && parseXML_noFail(tree, "DAMAGE"  , damage);
    flag = flag && parseXML_noFail(tree, "ISCAPITALSHIPMISSLE", isCapitalShipMissle);
    isActive = flag;
    return flag;
}

void MissleWeaponComponent::toXMLString(string &theString)
{
    sprintf(cTemp,"<MISSLEWEAPONCOMPONENT>\n\t<ISACTIVE>%i</ISACTIVE>\n\t<ACCURACY>%i</ACCURACY>\n\t<RANGE>%i</RANGE>\n\t<DAMAGE>%i</DAMAGE>\n\t<ISCAPITALSHIPMISSLE>%i</ISCAPITALSHIPMISSLE>\n\t</MISSLEWEAPONCOMPONENT>\n", isActive,accuracy,range,damage,isCapitalShipMissle);
    theString = cTemp;
}

bool BeamWeaponComponent::parseXML(mxml_node_t *tree)
{
    isActive = false;

    tree = mxmlFindElement(tree, tree, "BEAMWEAPONCOMPONENT", NULL, NULL, MXML_DESCEND);
    if( !tree )
        return false;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;

    if( !isActive )
        return true;

    bool flag = true;

    flag = flag && parseXML_noFail(tree,"ISGATLINGCAPABLE",isGatlingCapable);
    flag = flag && parseXML_noFail(tree,"ISSHIELDSAPPER",isShieldSapper);
    flag = flag && parseXML_noFail(tree,"RANGE",range);

    if( !flag )
        return false;

    int *valArray = (int*)malloc(sizeof(int)*(range+1));
    mxml_node_t *child=NULL;

    child = mxmlFindElement(tree, tree, "DAMAGEFACTORS", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        cerr << "Error, missing  DAMAGEFACTORS list for component\n";
        flag = false;
    }
    else
    {
        child = child->child;
        unsigned tempU = assembleNodeArray(child, valArray, range+1);
        if( tempU != range+1 )
        {
            cerr << "Error, incomplete DAMAGEFACTORS list for component:\n";
            cerr << "Expecting " << (range+1) << " entries, found " << tempU << endl;
            flag = false;
        }
        else
            for(tempU = 0; tempU < (range+1); tempU++)
                damageFactors.push_back(valArray[tempU]);
    }

    free(valArray);
    isActive = flag;
    return flag;
}

void BeamWeaponComponent::toXMLString(string &theString)
{
    string damageFactorString="";
    if (isActive)
    {
        for (unsigned i = 0;i<=range;i++)
        {
            sprintf(cTemp,"%i ",damageFactors[i]);
            damageFactorString += cTemp;
        }
    }
    else
        damageFactorString = "0";
    sprintf(cTemp,"<BEAMWEAPONCOMPONENT>\n\t<ISACTIVE>%i</ISACTIVE>\n\t<ISGATLINGCAPABLE>%i</ISGATLINGCAPABLE>\n\t<ISSHIELDSAPPER>%i</ISSHIELDSAPPER>\n\t<RANGE>%i</RANGE>\n\t<DAMAGEFACTORS>%s</DAMAGEFACTORS>\n\t</BEAMWEAPONCOMPONENT>\n",isActive,isGatlingCapable,isShieldSapper,range,damageFactorString.c_str());
    theString = cTemp;
}

bool MinelayerComponent::parseXML(mxml_node_t* tree)
{
    isActive = false;
    tree = mxmlFindElement(tree, tree, "MINELAYERCOMPONENT", NULL, NULL, MXML_DESCEND);
    if( !tree )
        return false;

    mineLayingRates[0] = 0;
    mineLayingRates[1] = 0;
    mineLayingRates[2] = 0;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;
    if( !isActive )
        return true;

    mxml_node_t *child;

    child = mxmlFindElement(tree, tree, "MINELAYINGRATES", NULL, NULL, MXML_DESCEND);
    if( !child )
        return false;

    child = child->child;
    int tempInt = assembleNodeArray(child, mineLayingRates, _numberOfMineTypes);
    if( tempInt != _numberOfMineTypes )
    {
        cerr << "Error, incomplete minelayer rate vector \n";
        cerr << "Expecting "<< _numberOfMineTypes
        << " entries, found " << tempInt << endl;
        mineLayingRates[0] = 0;
        mineLayingRates[1] = 0;
        mineLayingRates[2] = 0;
    }
    isActive = true;
    return true;
}

void MinelayerComponent::toXMLString(string &theString)
{
    string mineRatesString="";
    if (isActive)
    {
        for (unsigned i = 0;i<=(unsigned) _numberOfMineTypes;i++)
        {
            sprintf(cTemp,"%i ",mineLayingRates[i]);
            mineRatesString += cTemp;
        }
    }
    else
        mineRatesString = "0";

    sprintf(cTemp,"<MINELAYERCOMPONENT>\n\t<ISACTIVE>%i</ISACTIVE>\n\t<MINELAYINGRATES>%s</MINELAYINGRATES>\n</MINELAYERCOMPONENT>\n",isActive,mineRatesString.c_str());
    theString = cTemp;
}

bool WarpEngineComponent::parseXML(mxml_node_t* tree)
{
    isActive = false;
    tree = mxmlFindElement(tree, tree, "WARPENGINECOMPONENT", NULL, NULL, MXML_DESCEND);
    if( !tree )
        return false;

    if( !parseXML_noFail(tree, "ISACTIVE", isActive) )
        return false;
    if( !isActive )
        return true;

    battleSpeed = 0;
    maxSafeSpeed = 0;
    bool flag = true;
    if( !parseXML_noFail(tree, "BATTLESPEED" , battleSpeed) )
        flag = false;
    if( !parseXML_noFail(tree, "MAXSAFESPEED", maxSafeSpeed) )
        flag = false;
    if( !parseXML_noFail(tree, "ISSCOOP"     , isScoop) )
        flag = false;
    if( !parseXML_noFail(tree, "DANGEROUSRADIATION", dangerousRadiation) )
        flag = false;

    mxml_node_t *child = NULL;

    child = mxmlFindElement(tree, tree, "FUELCONSUMPTIONVECTOR", NULL, NULL, MXML_DESCEND);
    if( !child )
        flag = false;
    else
    {
        child = child->child;
        int tempInt = assembleNodeArray(child, fuelConsumptionVector, MAXPLAYERSPEED);
        if( tempInt != MAXPLAYERSPEED )
        {
            cerr << "Error, incomplete fuel consumption vector\n";
            cerr << "Expecting " << MAXPLAYERSPEED << " entries, found " << tempInt << endl;
            flag = false;
        }
        for(int i = MAXPLAYERSPEED; i > 0; i--)
            fuelConsumptionVector[i] = fuelConsumptionVector[i-1];
        fuelConsumptionVector[0] = 0;
    }

    if( !flag )
    {
        maxSafeSpeed = 0;
    }

    isActive = flag;
    return flag;
}

bool StructuralElement::parseTechnicalComponents(mxml_node_t *tree,
        std::vector<StructuralElement*> &list)
{
    mxml_node_t *node = mxmlFindElement(tree, tree, "COMPONENT", NULL, NULL,
                                        MXML_DESCEND);
    if( !node )
    {
        fprintf(stderr, "***Error, could not parse buildable technology componets list!\n");
        return false;
    }

    while( node )
    {
        StructuralElement *theElement = new StructuralElement();

        theElement->structureID = nextID;
        nextID++;
        theElement->parseXML(node);

        list.push_back(theElement);
        seElementNameMap[theElement->name] = theElement->structureID;

        node = mxmlFindElement(node, tree, "COMPONENT", NULL, NULL,
                               MXML_DESCEND);
    }

    return true;
}

void ComplexComponent::toCPPString(string &theString)
{
    theString = "Base complex component string";
}
void ComplexComponent::toXMLString(string &theString)
{
    theString = "Error, ComplexComponent toXML() should never be called\n";
}
bool ComplexComponent::parseXML(mxml_node_t *tree,string nodeName)
{
    cerr<< "Error, ComplexComponent parseXML() should never be called\n";
    return false;
}

void StructuralElement::toCPPString(string& theString)
{
    theString = "";
    char ctemp[4096];
    string tempString;

    sprintf(ctemp,"***** %s *****\n",name.c_str());
    theString = ctemp;
    bool firstPrint = false;
    sprintf(ctemp, "ComponentID = %i\t TypeID = %i (%s)\n", structureID,
            elementType, structuralElementCategoryStrings[elementType].c_str());
    theString += ctemp;

    tempString = "";
    bool tempFlag = true;
    int i;
    prtLrtCPPString(tempString,acceptablePRTs,acceptableLRTs);
    theString += tempString;

    tempString = "HULLs:";
    tempFlag = true;
    firstPrint = true;
    for(i = 0; i < numberOf_hull; i++)
    {
        if (acceptableHulls[i]==1)
        {
            if (firstPrint)
                tempString += " ";
            else
                tempString += ", ";
            tempString += HULL_NAMES[i];
            firstPrint=false;
        }
        else
            tempFlag = false;
    }
    tempString += "\n";

    if (tempFlag)
        tempString = "HULLs: ALL\n";
    theString += tempString;

    techLevel.toCPPString(tempString);
    sprintf (ctemp,"TechLevel: %s\n",tempString.c_str());

    theString += ctemp;

    mineralCostBase.toCPPString(tempString);
    theString += tempString;

    sprintf(ctemp,"Armor: %i \tShields: %i \tMass: %i\n",armor,shields,mass);
    theString += ctemp;

    sprintf(ctemp,"FuelCap: %i \tCargoCap: %i \tFuelGen: %i\n",fuelCapacity,cargoCapacity,fuelGen);
    theString += ctemp;

    sprintf(ctemp,"Cloak: %i \tJam: %i \tReflection: %i \tCapcitance: %i\n",cloak, jam, beamReflection, energyCapacitance);
    theString += ctemp;

    sprintf(ctemp,"RemoteMine: %i \tRemoteTForm: %i \tEnergyDampener: %i\n",remoteMineing, remoteTerraformCapable, energyDampening);
    theString += ctemp;

    sprintf(ctemp,"Pickpocket: %i \tRobberbaron: %i \tTachyonDetection: %i\n", fleetTheftCapable, planetTheftCapable, tachyonDetection);
    theString += ctemp;

    sprintf(ctemp,"PacketAccelertion: %i \tAccuracyImprv: %i \tThrust: %i\n", packetAcceleration, accuracyImprovement, thrust);
    theString += ctemp;

    sprintf(ctemp,"BaseInit: %i \tTechtree ID: %i\tResourceCostBase: %i\n", baseInitiative, techTreeID, resourceCostBase);

    theString += ctemp;

    sprintf(ctemp,"Defense: %i \tPlanet Mine Rate: %i \t Res Gen Rate: %i \tMin Gen Rate: %i\n",
            defense, planetMineRate, resourceGenRate, mineralGenRate);
    theString += ctemp;

    sprintf(ctemp, "Terraform ability (grav, temp, rad) =\t%i\t%i\t%i\n",terraformAbility.grav, terraformAbility.temp, terraformAbility.rad);
    theString += ctemp;

    minelayer.toCPPString(tempString);
    theString += tempString;

    engine.toCPPString(tempString);
    theString += tempString;

    beam.toCPPString(tempString);
    theString += tempString;

    missle.toCPPString(tempString);
    theString += tempString;

    scanner.toCPPString(tempString);
    theString += tempString;

    jumpgate.toCPPString(tempString);
    theString += tempString;

    bomb.toCPPString(tempString);
    theString += tempString;

    colonizerModule.toCPPString(tempString);
    theString += tempString;
}

void MinelayerComponent::toCPPString(string &theString)
{
    theString = "";

    if( !isActive )
        theString = "Minelaying: Incapable\n";
    else
    {
        char ctemp[4096];
        for (unsigned u=0; u< (unsigned) _numberOfMineTypes; u++)
        {
            sprintf(ctemp,"Minelaying: %i %s per year\n",mineLayingRates[u],mineTypeArray[u].c_str());
            theString += ctemp;
        }
    }
}

void BombComponent::toCPPString(string &theString)
{
    theString = "";
    if( !isActive )
        theString = "Bombing: Incapable\n";
    else
    {
        char ctemp[4096];

        string smartStatus = "Smart Bomb";
        if( !isSmart )
            smartStatus = "Dumb Bomb";

        sprintf(ctemp,"%s: %i infrastructure, %i %% population(%i min)\n", smartStatus.c_str(), infrastructureKillRate, popKillRate, minPopKill);
        theString = ctemp;
    }
}
void BeamWeaponComponent::toCPPString(string& theString)
{
    if( !isActive )
    {
        theString = "Beam Attack: Incapable\n";
        return;
    }
    //else

    theString = "";

    string tempString = "(shields only)";
    if( !isShieldSapper  )
        tempString = "";
    if( isGatlingCapable )
        tempString +="(Gatling)";

    char ctemp[4096];
    sprintf(ctemp, "Beam Attack %s: Range: %i Damage Rates: ",
            tempString.c_str(), range);
    theString += ctemp;

    for(unsigned i = 0; i < damageFactors.size(); i++)
    {
        sprintf(ctemp,"%i ",damageFactors[i]);
        theString += ctemp;
    }

    theString += "\n";
}

void MissleWeaponComponent::toCPPString(string &theString)
{
    theString = "";
    if( !isActive )
        theString = "Missle Attack: Incapable\n";
    else
    {
        char ctemp[4096];
        string tempString = "(Capital Ship)";
        if (!isCapitalShipMissle)
            tempString = "";

        sprintf(ctemp,"Missle Attack: Range: %i Damage: %i %s Accuracy: %i\n", range, damage, tempString.c_str(),accuracy);
        theString = ctemp;
    }
}

void ScanComponent::toCPPString(string& theString)
{
    theString = "";
    if (!isActive)
        theString = "Scanning: Incapable\n";
    else
    {
        char ctemp[4096];

        sprintf(ctemp,"Scanning Range: Non-Pen: %i \tPen: %i\n", baseScanRange,penScanRange);
        theString = ctemp;
    }
}

void JumpgateComponent::toCPPString(string& theString)
{
    theString = "";
    if (!isActive)
        theString = "Jumpgate: none\n";
    else
    {
        char ctemp[4096];
        if (useTargetGateSpecs)
            sprintf(ctemp,"Jumpgate: Jump allowed, using target gate specifications\n");
        else
            sprintf(ctemp,"Jumpdate: %i ly. /%i kT\n",distanceLimit,massLimit);
        theString = ctemp;
    }
}

void WarpEngineComponent::toXMLString(string &theString)
{
    char cTemp[255];
    theString = "<WARPENGINECOMPONENT>\n";

    sprintf(cTemp,"\t<ISACTIVE>%i</ISACTIVE>\n",isActive);
    theString += cTemp;
    sprintf(cTemp,"\t<BATTLESPEED>%i</BATTLESPEED>\n",battleSpeed);
    theString += cTemp;
    sprintf(cTemp,"\t<MAXSAFESPEED>%i</MAXSAFESPEED>\n",maxSafeSpeed);
    theString += cTemp;

    if (!isActive)
    {
        theString += "\t<FUELCONSUMPTIONVECTOR>0</FUELCONSUMPTIONVECTOR>\n";
    }
    else
    {
        theString += "<FUELCONSUMPTIONVECTOR>";
        for (unsigned i = 1;i<MAXPLAYERSPEED +1;i++)
        {
            sprintf(cTemp,"%i",fuelConsumptionVector[i]);
            theString+=cTemp;
            if (i<MAXPLAYERSPEED)
                theString += " ";
        }
        theString += "</FUELCONSUMPTIONVECTOR>\n";
    }
    sprintf(cTemp,"\t<ISSCOOP>%i</ISSCOOP>\n",isScoop);
    theString += cTemp;
    sprintf(cTemp,"\t<DANGEROUSRADIATION>%i</DANGEROUSRADIATION>\n",dangerousRadiation);
    theString += cTemp;

    theString += "</WARPENGINECOMPONENT>\n";
}

void WarpEngineComponent::toCPPString(string& theString)
{
    theString = "";
    if (!isActive)
        theString = "Engine Capability: None\n";
    else
    {
        char ctemp[4096];
        string temp = "(Scoop Engine)";
        if (dangerousRadiation)
            temp += "(Radiation Hazard)";
        sprintf(ctemp,"Engine Capability %s: Safe Speed %i BattleSpeed %i\n",temp.c_str(), maxSafeSpeed, battleSpeed);
        theString += ctemp;
        theString += "Warp Factor  1   2   3   4   5   6   7   8   9   10\n";
        theString += "Fuel Usage  ";
        for (int i=1;i<=MAXPLAYERSPEED;i++)
        {
            sprintf(ctemp,"%3i ",fuelConsumptionVector[i]);
            theString += ctemp;
        }
        theString += "\n";
    }
}

void ColonizerModuleComponent::toCPPString(string& theString)
{
    theString = "";
    if (!isActive)
        theString = "Colonization: Incapable\n";
    else
    {
        sprintf(cTemp,"Colonization: %s\n",ColonizerDescriptions[colonizerType].c_str());
        theString = cTemp;
    }
}

void GameData::technicalComponentsToString(string &theString)
{
    theString = "";

    string tempString;
    for(unsigned i = 0; i < componentList.size(); i++)
    {
        StructuralElement *el = componentList[i];
        theString += "\n";
        el->toCPPString(tempString);
        theString += tempString;
    }
}

void GameData::hullListToString(string &theString)
{
    theString = "";
    string tempString;
    for (unsigned i = 0;i<hullList.size();i++)
    {
        Ship *theShip = hullList[i];
        theString += "\n";
        theShip->toCPPString(tempString,*this);
        theString += tempString;
    }
}

Race * GameData::getRaceById(unsigned id)
{
	if( raceList[0]->ownerID == id )
		return raceList[0];

	// races are sorted by ownerID, which starts at 1.
	// thus id-1 is the index in the raceList
	if( id > raceList.size() || raceList[id-1]->ownerID != id )
		return NULL;
	return raceList[id-1];
}

Fleet * GameData::getFleetByID(uint64_t targetID)
{
    for (unsigned i = 0; i<fleetList.size();i++)
    {
        Fleet *cf = fleetList[i];
        if (cf->objectId == targetID)
            return cf;
    }
    cerr <<"Error, could not find fleet ID "<<targetID<<endl;
//    starsExit("structuralElement.cpp",-1);
   return NULL; //compiler warning otherwise
}

Fleet * GameData::getLastYearFleetByID(uint64_t targetID)
{
    for (unsigned i = 0; i<previousYearFleetList.size();i++)
    {
        Fleet *cf = previousYearFleetList[i];
        if (cf->objectId == targetID)
            return cf;
    }
    cerr <<"Error, could not find fleet ID "<<targetID<<endl;
    starsExit("structuralElement.cpp",-1);
   return NULL; //compiler warning otherwise
}

bool StructuralElement::isBuildable(Race *whichRace,
                                    StarsTechnology whichTechLevel)
{
	// make sure the user has enough tech
	if( !whichTechLevel.meetsTechLevel(techLevel) )
		return false;

	// check PRT limitations
	for(size_t i = 0; i < numberOf_PRT; ++i)
	{
		// if we find our PRT, and that PRT is not acceptable
		if( whichRace->prtVector[i] == 1 && acceptablePRTs[i] == 0 )
			return false;
	}

	// check LRT limitations
	for(size_t j = 0; j < numberOf_LRT; ++j)
	{
		switch( acceptableLRTs[j] )
		{
		case 0: // you have this LRT, you are forbidden
			if( whichRace->lrtVector[j] == 1 )
				return false;
			break;

		case 1: // you must have this LRT to build it
			if( whichRace->lrtVector[j] != 1 )
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

void StructuralElement::toXMLString(string &theString)
{
    string tempString = "";
    vectorToString(tempString,acceptableHulls);
    theString = "<COMPONENT>\n";

    vectorToString(tempString,acceptableHulls);
    theString += "\t<ACCEPTABLEHULLS>"+tempString+"</ACCEPTABLEHULLS>\n";

    vectorToString(tempString,acceptablePRTs);
    theString += "\t<ACCEPTABLEPRTS>"+tempString+"</ACCEPTABLEPRTS>\n";

    vectorToString(tempString,acceptableLRTs);
    theString += "\t<ACCEPTABLELRTS>"+tempString+"</ACCEPTABLELRTS>\n";

    sprintf(cTemp,"<ACCURACYIMPROVEMENT>%i</ACCURACYIMPROVEMENT>\n", accuracyImprovement);
    theString += cTemp;

    sprintf(cTemp,"\t<ARMOR>%i</ARMOR>\n\t<BASEINITIATIVE>%i</BASEINITIATIVE>\n\t<BEAMREFLECTION>%i</BEAMREFLECTION>\n\t<CARGOCAPACITY>%i</CARGOCAPACITY>\n\t<CLOAK>%i</CLOAK>\n",armor,baseInitiative,beamReflection,cargoCapacity,cloak);
    theString += cTemp;

    sprintf(cTemp,"\t<DEFENSE>%i</DEFENSE>\n\t<ELEMENTTYPE>%i</ELEMENTTYPE>\n\t<ENERGYCAPACITANCE>%i</ENERGYCAPACITANCE>\n\t<ENERGYDAMPENING>%i</ENERGYDAMPENING>\n\t<FLEETTHEFTCAPABLE>%i</FLEETTHEFTCAPABLE>\n", defense, elementType, energyCapacitance, energyDampening, fleetTheftCapable);
    theString += cTemp;

    sprintf(cTemp,"\t<FUELCAPACITY>%i</FUELCAPACITY>\n\t<FUELGEN>%i</FUELGEN>\n\t<JAM>%i</JAM>\n\t<MASS>%i</MASS>\n", fuelCapacity, fuelGen, jam, mass);
    theString += cTemp;

    mineralCostBase.toXMLString(tempString,"MINERALCOSTBASE","SCALE=\"KiloTons\"");
    theString += "\t" + tempString;

    sprintf (cTemp, "\t<MINERALGENRATE>%i</MINERALGENRATE>\n\t<NAME>%s</NAME>\n\t<PACKETACCELERATION>%i</PACKETACCELERATION>\n\t<PLANETTHEFTCAPABLE>%i</PLANETTHEFTCAPABLE>\n\t<PLANETMINERATE>%i</PLANETMINERATE>\n", mineralGenRate, name.c_str(),packetAcceleration, planetTheftCapable, planetMineRate);
    theString += cTemp;

    sprintf (cTemp, "\t<REMOTEMINEING>%i</REMOTEMINEING>\n\t<REMOTETERRAFORMCAPABLE>%i</REMOTETERRAFORMCAPABLE>\n\t<RESOURCECOSTBASE>%i</RESOURCECOSTBASE>\n\t<RESOURCEGENRATE>%i</RESOURCEGENRATE>\n\t<SHIELDS>%i</SHIELDS>\n", remoteMineing, remoteTerraformCapable, resourceCostBase, resourceGenRate, shields);
    theString +=cTemp;

    techLevel.toXMLString(tempString,"STARSTECHLEVEL");
    theString += "\t" + tempString;

    sprintf(cTemp, "\t<TACHYONDETECTION>%i</TACHYONDETECTION>\n\t<TECHTREEID>%i</TECHTREEID>\n\t<THRUST>%i</THRUST>\n", tachyonDetection, techTreeID, thrust);
    theString += cTemp;

    sprintf(cTemp,"<TFABILITY>%i %i %i</TFABILITY>\n",terraformAbility.grav, terraformAbility.temp, terraformAbility.rad);
    theString += cTemp;

    beam.toXMLString(tempString);
    theString += tempString;

    bomb.toXMLString(tempString);
    theString += tempString;

    colonizerModule.toXMLString(tempString);
    theString += tempString;

    jumpgate.toXMLString(tempString);
    theString += tempString;

    minelayer.toXMLString(tempString);
    theString += tempString;

    missle.toXMLString(tempString);
    theString += tempString;

    scanner.toXMLString(tempString);
    theString += tempString;

    engine.toXMLString(tempString);
    theString += tempString;

    theString += "</COMPONENT>";
}
