// C++ Implementation: race
//
// Copyright: See COPYING file that comes with this distribution
#include "utils.h"
#include "race.h"
#include "classDef.h"
#include "newStars.h"

#include <cstdio>
#include <cmath>
#include <iostream>
#include <fstream>

using namespace std;

#define firstPlayerID 1
//#define jdebug

// number of factories needed to generate their unit output
// should be moved into race for more race flexibility
static const unsigned factoriesPerUnitOutput = 10;

static inline double sqr(double d)
{
	return d * d;
}

void racesToXMLString(const vector<Race*> &list, std::string &theString)
{
    string tempString="";
    theString = "<RACELIST>\n";
    for(unsigned i = 0; i < list.size(); i++)
    {
        const Race *tempRace = list[i];
        tempRace->toXMLString(tempString);
        theString += tempString;
    }
    theString += "</RACELIST>\n";
}

// loop through and read in all the races,
// store them in 'list' sorted by ownerID
bool parseRaces(mxml_node_t *tree, vector<Race*> &list)
{
    mxml_node_t *child = mxmlFindElement(tree, tree, "RACE", NULL, NULL,
                                         MXML_DESCEND);
    if( !child )
        return false;

    // put the races into a temp list as we read them
    std::vector<Race*> tempList;

    while( child )
    {
        Race *tmp = new Race();
        tmp->initRace(child);

        list.push_back(tmp);

        child = mxmlFindElement(child, tree, "RACE", NULL, NULL,
                                MXML_NO_DESCEND);
    }


    // now sort the races by ownerID, simple N^2
    for (unsigned i = 0 ;i< list.size(); i++)
    {
        for (unsigned j=0; j<list.size()-1-i; j++)
        {
            if (list[j+1]->ownerID < list[j]->ownerID)
            {
                Race *tempRace = list[j];
                list[j] = list[j+1];
                list[j+1] = tempRace;
            }
        }
    }

    return true;
}

// calculate the growth rate of a race on a given planet
// Return: -15..(100% of racial growth factor)
double Race::getGrowthRate(Planet *whichPlanet) const
{
	// calculate habitat
	double hab = planetValueCalcRace(whichPlanet->currentEnvironment, this);
	if( hab < 1 )
	{
		// red planet
		return hab;
	}

	// initial return value (growth scaled to hab)
	double ret = double(growthRate) * hab / 100.0;

	const unsigned maxPop = getMaxPop(whichPlanet) / 100; // in 100's
	const unsigned curPop = whichPlanet->getPopulation(); // in kt

	// if under one quarter pop
	if( (curPop << 2) <= maxPop )
	{
		return ret;
	}

	//Ned? over pop, should be neg?
	/*
	Stars Faq says:
	0.04% per % over 100% cap
	Thus a 200% cap planet is 100% over and thus has (0.04 * 100 = 4%) a 4% death rate
	This maxes out at 400% capacity at 12%
	*/
	if( curPop >= maxPop )
		return 0.0;
	//else scale down for crowding

	// from Stars FAQ
	// capacity as a percentage
	double capacityPer = double(curPop) / double(maxPop);
	double crowding = 16.0 / 9.0 * sqr(1.0 - capacityPer);

	return crowding * growthRate;
}

// calculate the amount of each mineral extracted on the given planet by this
// race. Does not currently take into account planet goodness -- just # of
// operative mines
CargoManifest Race::getMineralsExtracted(Planet *whichPlanet)
{
	const unsigned curPop = whichPlanet->getPopulation() * populationCargoUnit;

	// calculate effective mines
	const unsigned maxMines = (curPop * mineBuildFactor) / 10000;
	unsigned numMines = std::min(whichPlanet->getMines(), maxMines);

	const CargoManifest concentrations = whichPlanet->getMineralConcentrations();

	// foreach mineral
	CargoManifest mo;
	for(unsigned i = _mineralBegin; i <= _mineralEnd; ++i)
	{
		unsigned conc = concentrations.cargoDetail[i].amount;
		// homeworld is always good for 30%
		if( whichPlanet->isHomeworld() && conc < 30 )
			conc = 30;

		// mined amount = (concentration as percentage) * numMines
		// integer math, so make sure to multiply first (+50 to round)
		mo.cargoDetail[i].amount = (conc * numMines * mineOutput +
		    500) / 1000;
	}

	return mo;
}

// calculate the total number of resources generated on this planet
// does not currently factor in planetary goodness
unsigned Race::getResourceOutput(Planet *whichPlanet)
{
	//Ned, 4B population
	unsigned population = whichPlanet->getPopulation() * populationCargoUnit;
	//Ned, is the 1000 in the XML?
	unsigned maxFactories = population / 10000 * factoryBuildFactor;
	unsigned numFactories = whichPlanet->getFactories();

	/*
		cerr << "\n\n\n";
		cerr << whichPlanet->name<<": "<<population<<" inhabitants.\n";
		cerr << whichPlanet->name<<": "<<maxFactories<<" = MAXFACTORIES.\n";
		cerr << whichPlanet->name<<": "<<numFactories<<" = NUMFACTORIES.\n";
		cerr << "populationResourceFactor = "<<populationResourceFactor<<endl;
		cerr << "populationResourceUnit = "<<populationResourceUnit<<endl;
		cerr << "factoryOutput = "<<factoryOutput<<endl;
		cerr << "factoriesperUnitOutput = "<<factoriesPerUnitOutput<<endl;
	*/

	// limit number of factories to what we can use
	if( numFactories > maxFactories )
		numFactories = maxFactories;

	// factories rounded up
	unsigned outputFromFactories  = unsigned(ceil(double(numFactories) *
	    factoryOutput / factoriesPerUnitOutput));
	// population rounded down
	unsigned outputFromPopulation = population * populationResourceFactor / populationResourceUnit / 100;

	/*
		cerr<<whichPlanet->name<<": "<<outputFromFactories<<" resources from factories.\n";
		cerr<<whichPlanet->name<<": "<<outputFromPopulation<<" resources from population.\n";
		cerr<<whichPlanet->name<<" generating "<<(outputFromFactories + outputFromPopulation)<<" resources.\n";
	*/

	return outputFromFactories + outputFromPopulation;
}

unsigned Race::getMaxPop(Planet *whichPlanet) const
{
	unsigned maxPop = 1000000;

	long hab = planetValueCalcRace(whichPlanet->currentEnvironment, this);
	// if red planet
	if( hab < 5 )
	{
		maxPop = 50000;
	}
	else
	{
		// scale max by hab rating (5..100%)
		maxPop = (unsigned)floor(double(hab) / 100.0 * double(maxPop));
	}

	double scale = 1.0;

	if( lrtVector[onlyBasicRemoteMining_LRT] == 1 )
		scale *= 1.1;

	// adjust for hyper-expansionist
	if( prtVector[hyperExpansionist_PRT] == 1 )
		scale *= 0.5;
	else if( prtVector[jackOfAllTrades_PRT] == 1 )
		scale *= 1.2;

	return unsigned(double(maxPop) * scale) ;
}

// calculate planetary maximims of stuff like factories, mines, etc
void Race::getMaxInfrastructure(Planet *whichPlanet, PlanetaryInfrastructure &infra)
{
    unsigned population = getMaxPop(whichPlanet);
	 unsigned curPop = whichPlanet->getPopulation() * 100;
	 population = std::min(population, curPop);

    infra.factories = unsigned(floor(double(population) / 10000.0 * double(factoryBuildFactor)));
    infra.mines     = unsigned(floor(double(population) / 10000.0 * double(mineBuildFactor)));
	 infra.scanner   = 1;

	 // 1 for the first hundred people, 1 for every _full_ 2500 after that
	 if( population < 100 )
		 infra.defenses = 0;
	 else
		 infra.defenses = 1 + ((population - 100) / 2500);
	 if( infra.defenses > 100 )
		 infra.defenses = 100;
}

// initialize the race object with the contents of the race subtree pointed to by tree
bool Race::initRace(mxml_node_t *tree)
{
	lrtVector.clear();
	prtVector.clear();

	mxml_node_t *child = NULL;
	unsigned valArray[128];
	int valCounter=0;
	int tempInt;

	child = mxmlFindElement(tree,tree,"SINGULARNAME",NULL,NULL,MXML_DESCEND);
	if( !child )
	{
	    std::cerr << "Error, could not parse race name\n";
	    return false;
	}
	child = child->child;

	//singularName = child->value.text.string; // won't work
	assembleNodeArray(child, singularName);

	child = mxmlFindElement(tree,tree,"PLURALNAME",NULL,NULL,MXML_DESCEND);
	if( !child )
	{
	    std::cerr << "Error, could not parse race name\n";
	    return false;
	}
	child = child->child;
	//pluralName = child->value.text.string;
	assembleNodeArray(child,pluralName);

	child = mxmlFindElement(tree,tree,"GROWTHRATE",NULL,NULL,MXML_DESCEND);
	if( child == NULL )
	{
	    std::cerr << "Error, could not parse growth rate for race:"<<singularName<<"\n";
	    return false;
	}
	child = child->child;
	growthRate = atoi(child->value.text.string);

	child = mxmlFindElement(tree,tree,"OWNERID",NULL,NULL,MXML_DESCEND);
	if (child == NULL)
	{
	    std::cerr << "Error, could not parse ownerID for race:"<<singularName<<"\n";
	    return false;
	}
	child = child->child;
	ownerID = atoi(child->value.text.string);

	// habitation optimal
	child = mxmlFindElement(tree,tree,"HAB",NULL,NULL,MXML_DESCEND);
	if (child == NULL)
	{
	    std::cerr << "Error, could not parse HAB [optimal] for race:"<<singularName<<"\n";
	    return false;
	}

	valCounter = assembleNodeArray(child->child,valArray,3);
	if( valCounter != 3 )
	{
	    std::cerr << "Error, found "<<valCounter<<" hab values (expecting 3) for race:"<<singularName<<endl;
	    return false;
	}
	optimalEnvironment.grav = valArray[0];
	optimalEnvironment.temp = valArray[1];
	optimalEnvironment.rad = valArray[2];

	// hab tolerence
	child = mxmlFindElement(tree,tree,"TOLERANCE",NULL,NULL,MXML_DESCEND);
	if (child == NULL)
	{
	    std::cerr << "Error, could not parse TOLERANCE [hab range] for race:"<<singularName<<"\n";
	    return false;
	}

	valCounter = assembleNodeArray(child->child,valArray,3);
	if( valCounter != 3 )
	{
	    std::cerr << "Error, found "<<valCounter<<" tolerance values (expecting 3) for race:"<<singularName<<endl;
	    return false;
	}
	environmentalTolerances.grav = valArray[0];
	environmentalTolerances.temp = valArray[1];
	environmentalTolerances.rad = valArray[2];

	// hab immunity
	child = mxmlFindElement(tree,tree,"IMMUNITY",NULL,NULL,MXML_DESCEND);
	if( child == NULL )
	{
	    std::cerr << "Error, could not parse IMMUNITY [hab] for race:"<<singularName<<"\n";
	    return false;
	}

	valCounter = assembleNodeArray(child->child,valArray,3);
	if( valCounter != 3 )
	{
	    std::cerr << "Error, found "<<valCounter<<" immunity values (expecting 3) for race:"<<singularName<<endl;
	    return false;
	}
	environmentalImmunities.grav = valArray[0];
	environmentalImmunities.temp = valArray[1];
	environmentalImmunities.rad = valArray[2];

	// terraform Ability
	child = mxmlFindElement(tree,tree,"TERRAABILITY",NULL,NULL,MXML_DESCEND);
	if (child == NULL)
	{
	    currentTerraformingAbility.grav = 0;
	    currentTerraformingAbility.temp = 0;
	    currentTerraformingAbility.rad = 0;
	}
	else
	{
	    valCounter = assembleNodeArray(child->child,valArray,3);
	    if (valCounter!=3)
	    {
	        std::cerr << "Error, found "<<valCounter<<" immunity values (expecting 3) for race:"<<singularName<<endl;
	        return false;
	    }
	    currentTerraformingAbility.grav = valArray[0];
	    currentTerraformingAbility.temp = valArray[1];
	    currentTerraformingAbility.rad = valArray[2];
	}

	techCostMultiplier.parseXML(tree, "TECHCOSTMULTIPLIER");
	//Ned, remove and fix
	for(unsigned i = 0; i < numberOf_TECH; i++)
		if( techCostMultiplier.techLevels[i] > 2 )
			techCostMultiplier.techLevels[i] = 2;

	// These factors are for AR type factor breaks
	// in the AR resource calculation
	techMiningFactorBreak.parseXML(tree,"TECHMININGFACTORBREAK");
	techResourceFactorBreak.parseXML(tree,"TECHRESOURCEFACTORBREAK");

	//------------------
	child = mxmlFindElement(tree, tree, "LRTVECTOR", NULL, NULL, MXML_DESCEND);
	if( !child )
	{
	    cerr << "Error, missing acceptable LRT list for race: " << pluralName << endl;
	    return false;
	}
	child = child->child;

	tempInt = assembleNodeArray(child, valArray, numberOf_LRT);

	for(tempInt = 0;tempInt < numberOf_LRT; tempInt++)
	    lrtVector.push_back(valArray[tempInt]);

	if( tempInt != numberOf_LRT )
	{
	   cerr << "Error, incomplete  LRT list for race: " << pluralName << endl;
	   cerr << "Expecting "<<numberOf_LRT<<" entries, found " << tempInt << endl;
		lrtVector.push_back(0);  //temporary fix for files when adding cheap engines
	   //return false;
	}
	//------------------
	child = mxmlFindElement(tree, tree, "PRTVECTOR", NULL, NULL, MXML_DESCEND);
	if( !child )
	{
	    cerr << "Error, missing acceptable PRT list for race: " << pluralName << endl;
	    return false;
	}
	child = child->child;

	tempInt = assembleNodeArray(child, valArray, numberOf_LRT);

	for(tempInt = 0;tempInt < numberOf_PRT; tempInt++)
	{
	   prtVector.push_back(valArray[tempInt]);
	   if(valArray[tempInt] == 1) primaryTrait = tempInt;
	}
	if( tempInt != numberOf_PRT )
	{
	    cerr << "Error, incomplete  PRT list for race: " << pluralName << endl;
	    cerr << "Expecting "<<numberOf_PRT<<" entries, found " << tempInt << endl;
	
	   return false;
	}

	//------------------
	factoryMineralCost.parseXML(tree,"FACTORYMINERALCOST");
	   mineMineralCost.parseXML(tree,"MINEMINERALCOST");

	parseXML_noFail(tree, "FACTORYOUTPUT",            factoryOutput);
	parseXML_noFail(tree, "MINEOUTPUT",               mineOutput);

	parseXML_noFail(tree, "FACTORYRESOURCECOST",      factoryResourceCost);
	parseXML_noFail(tree, "MINERESOURCECOST",         mineResourceCost);

	parseXML_noFail(tree, "FACTORYBUILDFACTOR",       factoryBuildFactor);
	parseXML_noFail(tree, "MINEBUILDFACTOR",          mineBuildFactor);
	parseXML_noFail(tree, "POPULATIONRESOURCEUNIT",   tempInt);
	populationResourceUnit = tempInt / 100;
	parseXML_noFail(tree, "POPULATIONMININGUNIT",     populationMiningUnit);
	parseXML_noFail(tree, "POPULATIONRESOURCEFACTOR", populationResourceFactor);
	parseXML_noFail(tree, "POPULATIONMININGFACTOR",   populationMiningFactor);
	parseXML_noFail(tree, "EXPENSIVESTARTAT3",        expensiveStartAtThree);

	parseXML_noFail(tree, "EXTRAPOINTALLOCATION", tempInt);
	extraPointAllocation = (LeftoverPointsEnum)tempInt;

	return true;
}

void Race::toXMLString(string &theString) const
{
    char cTemp[2048];
    string tempString;
    theString = "";
    theString += "<RACE>\n";
    sprintf(cTemp,"<SINGULARNAME>%s</SINGULARNAME>\n",singularName.c_str());
    theString += cTemp;
    sprintf(cTemp,"<PLURALNAME>%s</PLURALNAME>\n",pluralName.c_str());
    theString += cTemp;
    sprintf(cTemp,"<GROWTHRATE>%i</GROWTHRATE>\n",growthRate);
    theString +=cTemp;
    sprintf(cTemp,"<OWNERID>%i</OWNERID>\n",ownerID);
    theString += cTemp;
    sprintf(cTemp,"<HAB TYPE=\"gtr 1.0\">%i %i %i</HAB>\n",optimalEnvironment.grav,optimalEnvironment.temp,optimalEnvironment.rad);
    theString += cTemp;
    sprintf(cTemp,"<TOLERANCE TYPE=\"gtr 1.0\">%i %i %i</TOLERANCE>\n",environmentalTolerances.grav,environmentalTolerances.temp,environmentalTolerances.rad);
    theString += cTemp;
    sprintf(cTemp,"<IMMUNITY TYPE=\"gtr 1.0\">%i %i %i</IMMUNITY>\n",environmentalImmunities.grav,environmentalImmunities.temp,environmentalImmunities.rad);
    theString += cTemp;
    sprintf(cTemp,"<TERRAABILITY TYPE=\"gtr 1.0\">%i %i %i</TERRAABILITY>\n",currentTerraformingAbility.grav,currentTerraformingAbility.temp,currentTerraformingAbility.rad);
    theString += cTemp;

    techCostMultiplier.toXMLString(tempString,"TECHCOSTMULTIPLIER");
    theString += tempString;
    techMiningFactorBreak.toXMLString(tempString,"TECHMININGFACTORBREAK");
    theString += tempString;
    techResourceFactorBreak.toXMLString(tempString,"TECHRESOURCEFACTORBREAK");
    theString += tempString;

    vectorToString(tempString,prtVector);
    theString += "<PRTVECTOR>" + tempString + "</PRTVECTOR>\n";
    vectorToString(tempString,lrtVector);
    theString += "<LRTVECTOR>" + tempString + "</LRTVECTOR>\n";

    factoryMineralCost.toXMLString(tempString,"FACTORYMINERALCOST","SCALE=\"KiloTons\"");
    theString += tempString;
    mineMineralCost.toXMLString(tempString,"MINEMINERALCOST","SCALE=\"KiloTons\"");
    theString += tempString;

    sprintf (cTemp,"<FACTORYOUTPUT>%i</FACTORYOUTPUT>\n<MINEOUTPUT>%i</MINEOUTPUT>\n<FACTORYRESOURCECOST>%i</FACTORYRESOURCECOST>\n<MINERESOURCECOST>%i</MINERESOURCECOST>\n<FACTORYBUILDFACTOR>%i</FACTORYBUILDFACTOR>\n<MINEBUILDFACTOR>%i</MINEBUILDFACTOR>\n<POPULATIONRESOURCEUNIT>%i</POPULATIONRESOURCEUNIT>\n<POPULATIONMININGUNIT>%i</POPULATIONMININGUNIT>\n<POPULATIONRESOURCEFACTOR>%i</POPULATIONRESOURCEFACTOR>\n<POPULATIONMININGFACTOR>%i</POPULATIONMININGFACTOR>\n",
		 factoryOutput,mineOutput,factoryResourceCost,mineResourceCost,
		 factoryBuildFactor,mineBuildFactor,populationResourceUnit*100,
		 populationMiningUnit,populationResourceFactor,populationMiningFactor);
    theString += cTemp;

	sprintf (cTemp,"<EXPENSIVESTARTAT3>%i</EXPENSIVESTARTAT3>\n",expensiveStartAtThree);
	theString += cTemp;
	sprintf(cTemp,"<EXTRAPOINTALLOCATION>%i</EXTRAPOINTALLOCATION>\n",extraPointAllocation);
	theString += cTemp;
    theString += "</RACE>\n";
}

void Race::raceToString(string &theString)
{
    char tempString[2048];
    theString = "";
    sprintf(tempString,"Race Name: %s (%s)\n",singularName.c_str(),pluralName.c_str());
    theString += tempString;
    sprintf(tempString,"Growth Rate: %i\n",growthRate);
    theString +=tempString;
    sprintf(tempString,"Owner ID: %i\n",ownerID);
    theString += tempString;
    sprintf(tempString,"Hab range (gtr): %i %i %i\n", optimalEnvironment.grav, optimalEnvironment.temp, optimalEnvironment.rad);
    theString += tempString;
    sprintf(tempString,"Tolerance rance (gtr): %i %i %i\n",environmentalTolerances.grav,environmentalTolerances.temp,environmentalTolerances.rad);
    theString += tempString;
    sprintf(tempString,"Immunity flags (gtr) %i %i %i\n",environmentalImmunities.grav,environmentalImmunities.temp,environmentalImmunities.rad);
    theString += tempString;
}

bool Race::canTerraformPlanet(Planet *whichPlanet, GameData &gd) const
{
	if( whichPlanet->currentEnvironment == optimalEnvironment )
		return false;

	Environment delta = whichPlanet->originalEnvironment -
	    whichPlanet->currentEnvironment;

	if( abs(delta.grav) < currentTerraformingAbility.grav )
		return true;

	if( abs(delta.temp) < currentTerraformingAbility.temp )
		return true;

	if( abs(delta.rad) < currentTerraformingAbility.rad )
		return true;

	return false;
}

Environment Race::getFinalTerraformEnvironment(Planet *whichPlanet)
{
    Environment delta = whichPlanet->originalEnvironment - optimalEnvironment;
	 Environment final = whichPlanet->originalEnvironment;

    //grav
    if (abs(delta.grav) >= currentTerraformingAbility.grav) //can't terraform it all
    {
        //too much to terraform
        if (whichPlanet->originalEnvironment.grav > optimalEnvironment.grav) //we are too high
            final.grav -= currentTerraformingAbility.grav;
        else //we are too low
            final.grav += currentTerraformingAbility.grav;
    }
    else
        final.grav = optimalEnvironment.grav;

    //temp
    if (abs(delta.temp) >= currentTerraformingAbility.temp) //can't terraform it all
    {
        //too much to terraform
        if (whichPlanet->originalEnvironment.temp > optimalEnvironment.temp) //we are too high
            final.temp -= currentTerraformingAbility.temp;
        else //we are too low
            final.temp += currentTerraformingAbility.temp;
    }
    else
        final.temp = optimalEnvironment.temp;

    //rad
    if (abs(delta.rad) >= currentTerraformingAbility.rad) //can't terraform it all
    {
        //too much to terraform
        if (whichPlanet->originalEnvironment.rad > optimalEnvironment.rad) //we are too high
            final.rad -= currentTerraformingAbility.rad;
        else //we are too low
            final.rad += currentTerraformingAbility.rad;
    }
    else
        final.rad = optimalEnvironment.rad;

    if (environmentalImmunities.rad == 1)
        final.rad = whichPlanet->currentEnvironment.rad;
    if (environmentalImmunities.grav == 1)
        final.grav = whichPlanet->currentEnvironment.grav;
    if (environmentalImmunities.temp == 1)
        final.temp = whichPlanet->currentEnvironment.temp;
    return final;
}

// determine the amount of terraforming to do
Environment Race::getTerraformAmount(Planet *whichPlanet, bool isMin, GameData &gd)
{
	// calculate distance from current to optimal
	Environment delta = whichPlanet->currentEnvironment - optimalEnvironment;

	// clear immunities
	if( environmentalImmunities.rad == 1 )
		delta.rad = 0;
	if( environmentalImmunities.grav == 1 )
		delta.grav = 0;
	if( environmentalImmunities.temp == 1 )
		delta.temp = 0;

	// if any out of tolerance
	bool radOutOfTolerance  = abs(delta.rad ) > environmentalTolerances.rad;
	bool gravOutOfTolerance = abs(delta.grav) > environmentalTolerances.grav;
	bool tempOutOfTolerance = abs(delta.temp) > environmentalTolerances.temp;

	if( radOutOfTolerance || gravOutOfTolerance || tempOutOfTolerance )
	{
		// zero tolerance delta (we are not worried about them yet)
		if( !radOutOfTolerance )
			delta.rad = 0;
		if( !tempOutOfTolerance )
			delta.temp = 0;
		if( !gravOutOfTolerance )
			delta.grav = 0;
	}

	Environment amount;
	if( abs(delta.grav) > abs(delta.temp) && abs(delta.grav) > abs(delta.rad) )
	{
		amount.grav = gd.onePercentValues.grav;
		if( delta.grav < 0 )
			amount.grav = 0 - gd.onePercentValues.grav;
	}
	else if( abs(delta.temp) > abs(delta.rad) ) // not grav, is it temp?
	{
		amount.temp = gd.onePercentValues.temp;
		if( delta.temp < 0 )
			amount.temp = 0 - gd.onePercentValues.temp;
	}
	else
	{
		amount.rad = gd.onePercentValues.rad;
		if( delta.rad < 0 )
			amount.rad = 0 - gd.onePercentValues.rad;
	}
	return amount;
}

void Race::updateTerraformAbility(GameData &gd)
{
	Environment ability;

    for (unsigned i = 0 ;i< gd.componentList.size();i++)
    {
        StructuralElement *el = gd.componentList[i];
        if (el->isBuildable(this, gd.getPlayerData(ownerID)->currentTechLevel))
        {
            if (el->terraformAbility.grav >ability.grav)
                ability.grav = el->terraformAbility.grav;
            if (el->terraformAbility.temp >ability.temp)
                ability.temp = el->terraformAbility.temp;
            if (el->terraformAbility.rad >ability.rad)
                ability.rad = el->terraformAbility.rad;
        }
    }
    currentTerraformingAbility = ability;
}

Environment Environment::operator+(const Environment &param)
{
    Environment tEnv;
    tEnv.grav = grav + param.grav;
    tEnv.rad = rad + param.rad;
    tEnv.temp = temp + param.temp;
    return tEnv;
}

Environment Environment::operator-(const Environment &param)
{
    Environment tEnv;
    tEnv.grav = grav - param.grav;
    tEnv.rad = rad - param.rad;
    tEnv.temp = temp - param.temp;
    return tEnv;
}

bool Environment::operator==(const Environment &param)
{
	return param.temp == temp && param.rad == rad && param.grav == grav;
}

//----------------------------------------------------------------------------
int statsMin[16]={7,5,5,5,5,2,5,0,0,0,0,0,0,0,0,0};
int statsMax[16]={25,15,25,25,25,15,25,6,2,2,2,2,2,2,9,0};

int prtCost[numberOf_PRT];
int lrtCost[numberOf_LRT];
int scienceCost[numberOf_TECH];
int scienceLadder[numberOf_TECH*2 +1];
int facProdCost[26];
int facResReqCost[26];
int facNumOperCost[26];
int mineProdCost[26];
int mineResReqCost[26];
int mineNumOperCost[26];
int kColonistsForUnitRes[26];

int growthRateCost[21]; //one more for the zero index that is never used
int growthRateHabFactor[21]; //factor for taking hab range into account wrt. GR


#ifdef INCLUDE_ORIGINAL_RACE_CODE
#define GRBIT_LRT_IFE improvedFuelEfficiency_LRT
#define GRBIT_LRT_TT totalTerraforming_LRT
#define GRBIT_LRT_ARM advancedRemoteMining_LRT
#define GRBIT_LRT_ISB improvedFuelEfficiency_LRT
#define GRBIT_LRT_GR generalizedResearch_LRT
#define GRBIT_LRT_UR ultimateRecycling_LRT
#define GRBIT_LRT_MA mineralAlchemy_LRT
#define GRBIT_LRT_NRSE noRamscoopEngines_LRT
#define GRBIT_LRT_CE cheapEngines_LRT
#define GRBIT_LRT_OBRM onlyBasicRemoteMining_LRT
#define GRBIT_LRT_NAS noAdvancedScanners_LRT
#define GRBIT_LRT_LSP lowStartingPopulation_LRT
#define GRBIT_LRT_BET bleedingEdgeTechnology_LRT
#define GRBIT_LRT_RS regeneratingShields_LRT

#define GRBIT_TECH75_LVL3 29
#define GRBIT_FACT_GER_LESS 31

#define STAT_RES_PER_COL 0
#define STAT_TEN_FAC_RES 1
#define STAT_FAC_COST 2
#define STAT_FAC_OPERATE 3
#define STAT_TEN_MINES_PROD 4
#define STAT_MINE_COST 5
#define STAT_MINE_OPERATE 6
#define STAT_SPEND_LEFTOVER 7
#define STAT_ENERGY_COST 8
#define STAT_WEAP_COST 9
#define STAT_PROP_COST 10
#define STAT_CONST_COST 11
#define STAT_ELEC_COST 12
#define STAT_BIO_COST 13
#define STAT_PRT 14
struct playerDataStruct
{
	char centerHab[3];
	char lowerHab[3];
	char upperHab[3];
	char growthRate;
	char stats[16];
	unsigned long grbit;
	char cheaterFlag;
} player;
#define IMMUNE(a) ((a)==-1)
int getbit(unsigned long dw, int bitno)
{
	return (dw&(1<<bitno))!=0;
}
 long planetValueCalc(char* planetHabData,playerDataStruct* playerData)
{
	signed long planetValuePoints=0,redValue=0,ideality=10000;



	planetValuePoints = 0; ideality = 10000;redValue = 0;
	envVal(playerData->centerHab[0]==-1,playerData->lowerHab[0],playerData->upperHab[0],playerData->centerHab[0],planetHabData[0],planetValuePoints,ideality,redValue);
	envVal(playerData->centerHab[1]==-1,playerData->lowerHab[1],playerData->upperHab[1],playerData->centerHab[1],planetHabData[1],planetValuePoints,ideality,redValue);
	envVal(playerData->centerHab[2]==-1,playerData->lowerHab[2],playerData->upperHab[2],playerData->centerHab[2],planetHabData[2],planetValuePoints,ideality,redValue);

	if (redValue!=0)
	{
		return -redValue;
	}

	planetValuePoints = sqrt((double)planetValuePoints/3)+0.9;
	planetValuePoints = planetValuePoints * ideality/10000;

	return planetValuePoints;
}



 signed long habPoints(playerDataStruct* playerData)
{
	unsigned long isTotalTerraforming;
	double advantagePoints,tempLoopAccumulator,gravLoopAccumulator;
	signed long radLoopAccumulator,planetDesir;
	short terraformCapacity,deltaHabVector[3];
	char testPlanetHab[3];
		//order GTR

	advantagePoints = 0.0;
	isTotalTerraforming = getbit(playerData->grbit,GRBIT_LRT_TT);

	deltaHabVector[0]=deltaHabVector[1]=deltaHabVector[2]=0;
/*
	Generate planets within the hab range of the race and accumulate
	hab points.  For non immune habs slice the range into 11 segments,
	and generate planets in each segment.  Do full combinatorial
	across all 3 habs.  Iterate 3 times with different assumptions
	on terraforming capacity when generating planet value.
*/
	for (int terraFormIter = 0;terraFormIter<3;terraFormIter++)
	{
		Environment startRange;
		Environment habWidth;
		Environment centerE;
		Environment playerLow;
		Environment playerHigh;
		Environment numRangeSections;

		bool gravImmune = playerData->centerHab[0]==-1;
		bool tempImmune = playerData->centerHab[1]==-1;
		bool radImmune = playerData->centerHab[2]==-1;

		centerE.grav = playerData->centerHab[0];
		centerE.temp = playerData->centerHab[1];
		centerE.rad = playerData->centerHab[2];

		playerLow.grav = playerData->lowerHab[0];
		playerLow.temp = playerData->lowerHab[1];
		playerLow.rad = playerData->lowerHab[2];
		playerHigh.grav = playerData->upperHab[0];
		playerHigh.temp = playerData->upperHab[1];
		playerHigh.rad = playerData->upperHab[2];

		if (terraFormIter==0)
			terraformCapacity=0;
		else
			if (terraFormIter==1)
			{
				if (isTotalTerraforming)
					terraformCapacity = 8;
				else
					terraformCapacity = 5;
			}
		else
			if (isTotalTerraforming)
				terraformCapacity = 17;
			else
				terraformCapacity = 15;

		if (gravImmune)
		{
			startRange.grav = 50;
			habWidth.grav = 11;
			numRangeSections.grav = 1;
		}
		else
		{
			startRange.grav = playerLow.grav-terraformCapacity;
			if (startRange.grav <0) startRange.grav = 0;
			int tmpHab = playerHigh.grav + terraformCapacity;
			if (tmpHab>100) tmpHab = 100;
			habWidth.grav = tmpHab - startRange.grav;
			numRangeSections.grav = 11;
		}

		if (tempImmune)
		{
			startRange.temp = 50;
			habWidth.temp = 11;
			numRangeSections.temp = 1;
		}
		else
		{
			startRange.temp = playerLow.temp-terraformCapacity;
			if (startRange.temp <0) startRange.temp = 0;
			int tmpHab = playerHigh.temp + terraformCapacity;
			if (tmpHab>100) tmpHab = 100;
			habWidth.temp = tmpHab - startRange.temp;
			numRangeSections.temp = 11;
		}

		if (radImmune)
		{
			startRange.rad = 50;
			habWidth.rad = 11;
			numRangeSections.rad = 1;
		}
		else
		{
			startRange.rad = playerLow.rad-terraformCapacity;
			if (startRange.rad <0) startRange.rad = 0;
			int tmpHab = playerHigh.rad + terraformCapacity;
			if (tmpHab>100) tmpHab = 100;
			habWidth.rad = tmpHab - startRange.rad;
			numRangeSections.rad = 11;
		}

		
		//the following loops set up hab values for a planet and tests
		//planet value
		//i j and k loops set up planet hab 0 1 and 2 indexes respectively
		//widths are subdivided by iter count, and a full cross section of
		//planets is generated.  Each h loop gives a different terra value
		//Final is scaled linearly with width.
        gravLoopAccumulator = 0.0;
		for (int gravIter = 0;gravIter<numRangeSections.grav;gravIter++)
        {
			tempLoopAccumulator = 0.0;
			for (int tempIter=0;tempIter<numRangeSections.temp;tempIter++)
			{
				radLoopAccumulator = 0;
				for (int radIter=0;radIter<numRangeSections.rad;radIter++)
				{
					int lowValue = 0;
					int proposedHabValue = 0;

					
					findProposedHab(gravImmune,gravIter,numRangeSections.grav,startRange.grav,centerE.grav,habWidth.grav,terraformCapacity,proposedHabValue,deltaHabVector[0]);
					testPlanetHab[0] = (char)proposedHabValue;
					findProposedHab(tempImmune,tempIter,numRangeSections.temp,startRange.temp,centerE.temp,habWidth.temp,terraformCapacity,proposedHabValue,deltaHabVector[1]);
					testPlanetHab[1] = (char)proposedHabValue;
					findProposedHab(radImmune,radIter,numRangeSections.rad,startRange.rad,centerE.rad,habWidth.rad,terraformCapacity,proposedHabValue,deltaHabVector[2]);
					testPlanetHab[2] = (char)proposedHabValue;

					planetDesir = planetValueCalc(testPlanetHab,playerData);

					int totalDeltaHab = deltaHabVector[0]+deltaHabVector[1]+deltaHabVector[2];
					if (totalDeltaHab>terraformCapacity)
					{
						planetDesir-=totalDeltaHab-terraformCapacity;
						if (planetDesir<0) planetDesir=0;
					}
					planetDesir *= planetDesir;
					switch (terraFormIter)
					{
						case 0: planetDesir*=7; break;
						case 1: planetDesir*=5; break;
						default: planetDesir*=6;
					}
					radLoopAccumulator+=planetDesir;
				}//for rad loop

				if (!radImmune) radLoopAccumulator = (radLoopAccumulator*habWidth.rad)/100;
				else radLoopAccumulator *= 11;

				tempLoopAccumulator += radLoopAccumulator;
			}//for temp loop
			if (!tempImmune) tempLoopAccumulator = (tempLoopAccumulator*habWidth.temp)/100;
			else tempLoopAccumulator *= 11;

			gravLoopAccumulator += tempLoopAccumulator;
        }//for grav loop
		if (!gravImmune) gravLoopAccumulator = (gravLoopAccumulator*habWidth.grav)/100;
		else gravLoopAccumulator *= 11;

		advantagePoints += gravLoopAccumulator;
	} //for h loop

	return advantagePoints/10.0+0.5;
}


int calculateAdvantagePoints(playerDataStruct* playerData)
{
	int points = 1650;
	int hab;
	int PRT,grRateFactor,factoriesRaceCanOperate,resourcesProducedByTenFactories,tmpPoints;

#ifdef jdebug
	cout << "Step 1, points = " << points << endl;
#endif
	BoundsCheck(playerData);
	PRT = playerData->stats[STAT_PRT];
	hab = habPoints(playerData)/2000;

#ifdef jdebug
	cout << "Step 2, hab points = " << hab << endl;
#endif

	int raceGrowthRate = playerData->growthRate;
	points -= growthRateCost[raceGrowthRate];
	grRateFactor = growthRateHabFactor[raceGrowthRate];
	points -= (hab*grRateFactor)/24;

#ifdef jdebug
	cout << "Step 3, points = " << points << endl;
#endif


	int numImmunities = 0;

	for (int j=0; j<3; j++)
	{
		if (playerData->centerHab[j]<0)
		{
			numImmunities++;
		}
		else
		{
			points += abs(playerData->centerHab[j] - 50)*4;
		}
	}
	if (numImmunities>1) points -= 150;

#ifdef jdebug
	cout << "Step 4, points = " << points << endl;
#endif

	factoriesRaceCanOperate = playerData->stats[STAT_FAC_OPERATE];
	resourcesProducedByTenFactories = playerData->stats[STAT_TEN_FAC_RES];

	if (factoriesRaceCanOperate>10 || resourcesProducedByTenFactories>10)
	{
		factoriesRaceCanOperate -= 9;
		if (factoriesRaceCanOperate<1)
			factoriesRaceCanOperate=1;

		resourcesProducedByTenFactories  -= 9;
		if (resourcesProducedByTenFactories<1)
			resourcesProducedByTenFactories=1;

		int multiplier = 2;
		if (PRT==hyperExpansionist_PRT)
			multiplier = 3;

		resourcesProducedByTenFactories *= multiplier;

		/*additional penalty for two- and three-immune*/
		if (numImmunities>=2)
			points -= ((resourcesProducedByTenFactories*factoriesRaceCanOperate)*raceGrowthRate)/2;
		else
			points -= ((resourcesProducedByTenFactories*factoriesRaceCanOperate)*raceGrowthRate)/9;
	}

	int numColonistsGenerateUnitResource = playerData->stats[STAT_RES_PER_COL]; //in thousands
	
	points -= kColonistsForUnitRes[numColonistsGenerateUnitResource];

#ifdef jdebug
	cout << "Step 5, points = " << points << endl;
#endif


	if (PRT != alternateReality_PRT)
	{
		tmpPoints=0;

		tmpPoints += facProdCost[playerData->stats[STAT_TEN_FAC_RES]];
		tmpPoints += facResReqCost[playerData->stats[STAT_FAC_COST]];
		tmpPoints += facNumOperCost[playerData->stats[STAT_FAC_OPERATE]];

		tmpPoints += mineProdCost[playerData->stats[STAT_TEN_MINES_PROD]];
		tmpPoints += mineResReqCost[playerData->stats[STAT_MINE_COST]];
		tmpPoints += mineNumOperCost[playerData->stats[STAT_MINE_OPERATE]];
		if (getbit(playerData->grbit,GRBIT_FACT_GER_LESS) != 0) tmpPoints += 175;
		points -= tmpPoints;

	}
	else points += 210; /* AR */
	
#ifdef jdebug
	cout << "Step 6, points = " << points << endl;
#endif

	/*LRTs*/
	points -= prtCost[PRT];
	int numHandicap = 0;
	int numBenficial = 0;
	for(int j=0;j<=13;j++)
	{
		if (getbit(playerData->grbit,j)!=0)
		{
			if (lrtCost[j]<=0) numHandicap++;
			else numBenficial++;
			points -= lrtCost[j];
		}
	}
	if ((numBenficial+numHandicap)>4)
		points -= (numBenficial+numHandicap)*(numBenficial+numHandicap-4)*10;
	if ((numHandicap-numBenficial)>3)
		points -= (numHandicap-numBenficial-3)*60;
	if ((numBenficial-numHandicap)>3)
		points -= (numBenficial-numHandicap-3)*40;

	if (getbit(playerData->grbit,GRBIT_LRT_NAS) != 0)
	{
		if (PRT == packetPhysicist_PRT) points -= 280;
		else if (PRT == superStealth_PRT) points -= 200;
		else if (PRT == jackOfAllTrades_PRT) points -= 40;
	}

#ifdef jdebug
	cout << "Step 7, points = " << points << endl;
#endif

	/*science*/
	tmpPoints=0;

	
	for (int j=STAT_ENERGY_COST;j<=STAT_BIO_COST;j++)
		tmpPoints += playerData->stats[j];
	points += scienceLadder[tmpPoints];

	//punish races with efficient populations and expensive techs
	//why?  I have no clue but there you have it.
	if (tmpPoints<2 && playerData->stats[STAT_RES_PER_COL]<10)
			points -= 190;
	
	
	if (getbit(playerData->grbit,GRBIT_TECH75_LVL3) != 0) points-=180;
	if (PRT==alternateReality_PRT && playerData->stats[STAT_ENERGY_COST]==2/*50% less*/) points -= 100;

#ifdef jdebug
	cout << "Step 8, points = " << points << endl;

	cout << "Returning final value of " << points/3 << endl;
#endif

	return points/3;
}


void BoundsCheck(playerDataStruct* playerData)
{
	short i,tmp;

	for (i=0;i<3;i++)
	{
		if (IMMUNE(playerData->lowerHab[i]))
		{
			if (!IMMUNE(playerData->upperHab[i]) || !IMMUNE(playerData->centerHab[i]))
			{
				playerData->upperHab[i] = playerData->centerHab[i] = -1;
				playerData->cheaterFlag |= 0x10;
			}
		}
		else
		{
			if (playerData->lowerHab[i]<0)
			{
				playerData->lowerHab[i]=0;
				playerData->cheaterFlag |= 0x10;
			}
			if (playerData->lowerHab[i]>100)
			{
				playerData->lowerHab[i]=100;
				playerData->cheaterFlag |= 0x10;
			}
			if (playerData->upperHab[i]>100)
			{
				playerData->upperHab[i]=100;
				playerData->cheaterFlag |= 0x10;
			}
			if (playerData->lowerHab[i]>playerData->upperHab[i])
			{
				playerData->lowerHab[i] = playerData->upperHab[i];
				playerData->cheaterFlag |= 0x10;
			}
			tmp = playerData->centerHab[i];
			if (tmp != (playerData->lowerHab[i] + (playerData->upperHab[i] - playerData->lowerHab[i])/2))
			{
				tmp = playerData->lowerHab[i] + (playerData->upperHab[i] - playerData->lowerHab[i])/2;
				playerData->cheaterFlag |= 0x10;
			}
		}
	}

	if (playerData->growthRate > 20)
	{
		playerData->growthRate = 20;
		playerData->cheaterFlag |= 0x10;
	}
	if (playerData->growthRate < 1)
	{
		playerData->growthRate = 1;
		playerData->cheaterFlag |= 0x10;
	}

	for (i=0; i<16; i++)
	{
		if (playerData->stats[i] < statsMin[i])
		{
			playerData->stats[i] = statsMin[i];
			playerData->cheaterFlag |= 0x10;
		}
		if (playerData->stats[i] > statsMax[i])
		{
			playerData->stats[i] = statsMax[i];
			playerData->cheaterFlag |= 0x10;
		}
	}
}


#endif //INCLUDE_ORIGINAL_RACE_CODE

void initRaceWizard(void)
{
//int lrtCost[14]={-235,-25,-159,-201,40,-240,-155,160,240,255,325,180,70,30};
	lrtCost[improvedFuelEfficiency_LRT] = 235;
	lrtCost[totalTerraforming_LRT] = 25;
	lrtCost[advancedRemoteMining_LRT] = 159;
	lrtCost[improvedStarbases_LRT] = 201;
	lrtCost[generalizedResearch_LRT] = -40;
	lrtCost[ultimateRecycling_LRT] = 240;
	lrtCost[mineralAlchemy_LRT] = 155;
	lrtCost[noRamscoopEngines_LRT] = -160;
	lrtCost[cheapEngines_LRT] = -240;
	lrtCost[onlyBasicRemoteMining_LRT] = -255;
	lrtCost[noAdvancedScanners_LRT] = -325;
	lrtCost[lowStartingPopulation_LRT] = -180;
	lrtCost[bleedingEdgeTechnology_LRT] = -70;
	lrtCost[regeneratingShields_LRT] = -30;
	//scienceCost is an adder for cost depending on how many "cheap" techs
	//you have selected.  These numbers represent final stars calues,
	//but appear somewhat random.  Future versions might want to switch
	//to some polynonial function based on the number of cheaps vs exp, etc.
	scienceCost[0] = 150;
	scienceCost[1] = 330;
	scienceCost[2] = 540;
	scienceCost[3] = 780;
	scienceCost[4] = 1050;
	scienceCost[5] = 1380;

	//science ladder is an array with the correct costs/benefits based on summing the tech cost
	//vectors where expensive is 0, normal is 1 and cheap is 2
	scienceLadder[0] = 1380;
	scienceLadder[1] = 1050;
	scienceLadder[2] = 780;
	scienceLadder[3] = 540;
	scienceLadder[4] = 330;
	scienceLadder[5] = 150;
	scienceLadder[6] = 0;
	scienceLadder[7] = -130;
	scienceLadder[8] = -520;
	scienceLadder[9] = -1170;
	scienceLadder[10] = -2080;
	scienceLadder[11] = -2730;
	scienceLadder[12] = -3250;

	prtCost[hyperExpansionist_PRT] = 40;
	prtCost[superStealth_PRT] = 95;
	prtCost[warMonger_PRT] = 45;
	prtCost[claimAdjuster_PRT] = 10;
	prtCost[innerStrength_PRT] = -100;
	prtCost[spaceDemolition_PRT] = -150;
	prtCost[packetPhysicist_PRT] = 120;
	prtCost[interstellarTraveller_PRT] = 180;
	prtCost[alternateReality_PRT] = 90;
	prtCost[jackOfAllTrades_PRT] = -66;

	growthRateCost[0] = 0;
	growthRateCost[1] = -21000;
	growthRateCost[2] = -16800;
	growthRateCost[3] = -12600;
	growthRateCost[4] = -8400;
	growthRateCost[5] = -4200;
	growthRateCost[6] = -3600;
	growthRateCost[7] = -2250;
	growthRateCost[8] = -600;
	growthRateCost[9] = -225;
	growthRateCost[10] = 0;
	growthRateCost[11] = 0;
	growthRateCost[12] = 0;
	growthRateCost[13] = 0;
	growthRateCost[14] = 0;
	growthRateCost[15] = 0;
	growthRateCost[16] = 0;
	growthRateCost[17] = 0;
	growthRateCost[18] = 0;
	growthRateCost[19] = 0;
	growthRateCost[20] = 0;

	growthRateHabFactor[0] = 0;
	growthRateHabFactor[1] = 1;
	growthRateHabFactor[2] = 2;
	growthRateHabFactor[3] = 3;
	growthRateHabFactor[4] = 4;
	growthRateHabFactor[5] = 5;
	growthRateHabFactor[6] = 7;
	growthRateHabFactor[7] = 9;
	growthRateHabFactor[8] = 11;
	growthRateHabFactor[9] = 13;
	growthRateHabFactor[10] = 15;
	growthRateHabFactor[11] = 17;
	growthRateHabFactor[12] = 19;
	growthRateHabFactor[13] = 21;
	growthRateHabFactor[14] = 24;
	growthRateHabFactor[15] = 27;
	growthRateHabFactor[16] = 30;
	growthRateHabFactor[17] = 33;
	growthRateHabFactor[18] = 36;
	growthRateHabFactor[19] = 39;
	growthRateHabFactor[20] = 45;

	//points associated with number of factories
	//a race can operate (5-25)
	facNumOperCost[0] = 0;
	facNumOperCost[1] = 0;
	facNumOperCost[2] = 0;
	facNumOperCost[3] = 0;
	facNumOperCost[4] = 0;
	facNumOperCost[5] = -200;
	facNumOperCost[6] = -160;
	facNumOperCost[7] = -120;
	facNumOperCost[8] = -80;
	facNumOperCost[9] = -40;
	facNumOperCost[10] = 0;
	facNumOperCost[11] = 35;
	facNumOperCost[12] = 70;
	facNumOperCost[13] = 105;
	facNumOperCost[14] = 140;
	facNumOperCost[15] = 175;
	facNumOperCost[16] = 210;
	facNumOperCost[17] = 275;
	facNumOperCost[18] = 340;
	facNumOperCost[19] = 405;
	facNumOperCost[20] = 470;
	facNumOperCost[21] = 535;
	facNumOperCost[22] = 645;
	facNumOperCost[23] = 725;
	facNumOperCost[24] = 805;
	facNumOperCost[25] = 885;

	//factory efficiency (5-15)
	facProdCost[0] = 0;
	facProdCost[1] = 0;
	facProdCost[2] = 0;
	facProdCost[3] = 0;
	facProdCost[4] = 0;
	facProdCost[5] = -500;
	facProdCost[6] = -400;
	facProdCost[7] = -300;
	facProdCost[8] = -200;
	facProdCost[9] = -100;
	facProdCost[10] = 0;
	facProdCost[11] = 121;
	facProdCost[12] = 242;
	facProdCost[13] = 423;
	facProdCost[14] = 604;
	facProdCost[15] = 785;
	facProdCost[16] = 0;
	facProdCost[17] = 0;
	facProdCost[18] = 0;
	facProdCost[19] = 0;
	facProdCost[20] = 0;
	facProdCost[21] = 0;
	facProdCost[22] = 0;
	facProdCost[23] = 0;
	facProdCost[24] = 0;
	facProdCost[25] = 0;

	//factory cost in resources (5-25)
	facResReqCost[0] = 0;
	facResReqCost[1] = 0;
	facResReqCost[2] = 0;
	facResReqCost[3] = 0;
	facResReqCost[4] = 0;
	facResReqCost[5] = 1500;
	facResReqCost[6] = 960;
	facResReqCost[7] = 540;
	facResReqCost[8] = 240;
	facResReqCost[9] = 60;
	facResReqCost[10] = 0;
	facResReqCost[11] = -55;
	facResReqCost[12] = -110;
	facResReqCost[13] = -165;
	facResReqCost[14] = -220;
	facResReqCost[15] = -275;
	facResReqCost[16] = -330;
	facResReqCost[17] = -385;
	facResReqCost[18] = -440;
	facResReqCost[19] = -495;
	facResReqCost[20] = -550;
	facResReqCost[21] = -605;
	facResReqCost[22] = -660;
	facResReqCost[23] = -715;
	facResReqCost[24] = -770;
	facResReqCost[25] = -825;

	//point cost associated with the number of mines that can be operated
	mineNumOperCost[0] = 0; //not in range 0-4.   5 mine minimum
	mineNumOperCost[1] = 0;
	mineNumOperCost[2] = 0;
	mineNumOperCost[3] = 0;
	mineNumOperCost[4] = 0;
	mineNumOperCost[5] = -200;
	mineNumOperCost[6] = -160;
	mineNumOperCost[7] = -120;
	mineNumOperCost[8] = -80;
	mineNumOperCost[9] = -40;
	mineNumOperCost[10] = 0;
	mineNumOperCost[11] = 35;
	mineNumOperCost[12] = 70;
	mineNumOperCost[13] = 105;
	mineNumOperCost[14] = 140;
	mineNumOperCost[15] = 175;
	mineNumOperCost[16] = 210;
	mineNumOperCost[17] = 245;
	mineNumOperCost[18] = 280;
	mineNumOperCost[19] = 315;
	mineNumOperCost[20] = 350;
	mineNumOperCost[21] = 385;
	mineNumOperCost[22] = 420;
	mineNumOperCost[23] = 455;
	mineNumOperCost[24] = 490;
	mineNumOperCost[25] = 525;

	//point cost associated with how much each mine unit produces
	mineProdCost[0] = 0; //not in range 0-4
	mineProdCost[1] = 0;
	mineProdCost[2] = 0;
	mineProdCost[3] = 0;
	mineProdCost[4] = 0;
	mineProdCost[5] = -500;
	mineProdCost[6] = -400;
	mineProdCost[7] = -300;
	mineProdCost[8] = -200;
	mineProdCost[9] = -100;
	mineProdCost[10] = 0;
	mineProdCost[11] = 169;
	mineProdCost[12] = 338;
	mineProdCost[13] = 507;
	mineProdCost[14] = 676;
	mineProdCost[15] = 845;
	mineProdCost[16] = 1014;
	mineProdCost[17] = 1183;
	mineProdCost[18] = 1352;
	mineProdCost[19] = 1521;
	mineProdCost[20] = 1690;
	mineProdCost[21] = 1859;
	mineProdCost[22] = 2028;
	mineProdCost[23] = 2197;
	mineProdCost[24] = 2366;
	mineProdCost[25] = 2535;

	//point costs associated with how expensive it is to build a mine in resources
	mineResReqCost[0] = 0; //not in range
	mineResReqCost[1] = 0; //not in range
	mineResReqCost[2] = 360;
	mineResReqCost[3] = -80;
	mineResReqCost[4] = -145;
	mineResReqCost[5] = -210;
	mineResReqCost[6] = -275;
	mineResReqCost[7] = -340;
	mineResReqCost[8] = -405;
	mineResReqCost[9] = -470;
	mineResReqCost[10] = -535;
	mineResReqCost[11] = -600;
	mineResReqCost[12] = -665;
	mineResReqCost[13] = -730;
	mineResReqCost[14] = -795;
	mineResReqCost[15] = -860;
	mineResReqCost[16] = -925;
	mineResReqCost[17] = -990;
	mineResReqCost[18] = -1055;
	mineResReqCost[19] = -1120;
	mineResReqCost[20] = -1185;
	mineResReqCost[21] = -1250;
	mineResReqCost[22] = -1315;
	mineResReqCost[23] = -1380;
	mineResReqCost[24] = -1445;
	mineResReqCost[25] = -1510;
	//thousands of colonists required to produce the unit number of resources (10)
	//range is 7-25
	kColonistsForUnitRes[0] = 0;
	kColonistsForUnitRes[1] = 0;
	kColonistsForUnitRes[2] = 0;
	kColonistsForUnitRes[3] = 0;
	kColonistsForUnitRes[4] = 0;
	kColonistsForUnitRes[5] = 0;
	kColonistsForUnitRes[6] = 0;
	kColonistsForUnitRes[7] = 2400;
	kColonistsForUnitRes[8] = 1260;
	kColonistsForUnitRes[9] = 600;
	kColonistsForUnitRes[10] = 0;
	kColonistsForUnitRes[11] = -120;
	kColonistsForUnitRes[12] = -240;
	kColonistsForUnitRes[13] = -360;
	kColonistsForUnitRes[14] = -480;
	kColonistsForUnitRes[15] = -600;
	kColonistsForUnitRes[16] = -720;
	kColonistsForUnitRes[17] = -840;
	kColonistsForUnitRes[18] = -960;
	kColonistsForUnitRes[19] = -1080;
	kColonistsForUnitRes[20] = -1200;
	kColonistsForUnitRes[21] = -1320;
	kColonistsForUnitRes[22] = -1440;
	kColonistsForUnitRes[23] = -1560;
	kColonistsForUnitRes[24] = -1680;
	kColonistsForUnitRes[25] = -1800;

}


void envVal(bool isImmune, int habLower, int habUpper, int habCenter,int planetHab,
			long &pvPoints,long &ideality, long &redValue)
{
	int tmp = 0;
	int habRadius = habUpper-habCenter;
	if (isImmune)
		pvPoints += 10000;
	else
	{
		if (habLower <= planetHab && habUpper >= planetHab)
		{
			//green
			habRadius = habUpper-habCenter;
			//calculate percent distance of width from center
			int pValue = abs(planetHab-habCenter) *100 / habRadius ;
			tmp = abs(habCenter-planetHab);
			//affect larger radii less ideality for being outside range smaller amounts
			int idealityDelta = ((tmp)*2)-habRadius;

			//convert % distance to % optimal
			pValue = 100 - pValue;
			pvPoints += pValue*pValue;
			if (idealityDelta>0)
			{
				ideality *= habRadius*2 - idealityDelta;
				ideality /= habRadius*2;
			}
		}
		else
		{
			//RED
			tmp = abs(planetHab-habCenter) - habRadius;

			if (tmp>15) tmp=15;

			redValue+=tmp;
		}
	}
}

long planetValueCalcRace(const Environment &env, const Race *whichRace)
{
	signed long planetValuePoints=0,redValue=0,ideality=10000;
	Environment centerE;
	Environment playerLow;
	Environment playerHigh;

	bool gravImmune = whichRace->environmentalImmunities.grav == 1;//playerData->centerHab[0]==-1;
	bool tempImmune = whichRace->environmentalImmunities.temp == 1;//playerData->centerHab[1]==-1;
	bool radImmune = whichRace->environmentalImmunities.rad == 1;//playerData->centerHab[2]==-1;

	centerE.grav = whichRace->optimalEnvironment.grav;// playerData->centerHab[0];
	centerE.temp = whichRace->optimalEnvironment.temp;// playerData->centerHab[1];
	centerE.rad = whichRace->optimalEnvironment.rad;// playerData->centerHab[2];

	playerLow.grav = whichRace->optimalEnvironment.grav - whichRace->environmentalTolerances.grav;// playerData->lowerHab[0];
	playerLow.temp = whichRace->optimalEnvironment.temp - whichRace->environmentalTolerances.temp;
	playerLow.rad = whichRace->optimalEnvironment.rad - whichRace->environmentalTolerances.rad;
	playerHigh.grav = whichRace->optimalEnvironment.grav + whichRace->environmentalTolerances.grav;
	playerHigh.temp = whichRace->optimalEnvironment.temp + whichRace->environmentalTolerances.temp;
	playerHigh.rad = whichRace->optimalEnvironment.rad + whichRace->environmentalTolerances.rad;
	if (playerLow.grav<0 )playerLow.grav = 0;
	if (playerLow.temp<0) playerLow.temp = 0;
	if (playerLow.rad <0) playerLow.rad = 0;
	if (playerHigh.grav > 100) playerHigh.grav = 100;
	if (playerHigh.temp > 100) playerHigh.temp = 100;
	if (playerHigh.rad >  100) playerHigh.rad = 100;

	planetValuePoints = 0;
	ideality = 10000;
	redValue = 0;
	envVal(gravImmune, playerLow.grav, playerHigh.grav, centerE.grav, env.grav, planetValuePoints, ideality, redValue);
	envVal(tempImmune, playerLow.temp, playerHigh.temp, centerE.temp, env.temp, planetValuePoints, ideality, redValue);
	envVal( radImmune, playerLow.rad,  playerHigh.rad,  centerE.rad,  env.rad,  planetValuePoints, ideality, redValue);

	if( redValue != 0 )
	{
		return -redValue;
	}

	planetValuePoints = (long)(sqrt((double)planetValuePoints/3)+0.9);
	planetValuePoints = planetValuePoints * ideality/10000;

	return planetValuePoints;
}

void findProposedHab(bool isImmune, int iterValue,int numSections, int startRange, int centerValue,
					 int width, int terraCap, int &propV, short &deltaV)
{
	int lowValue = 0;
	if (iterValue==0 || numSections<=1)
    	lowValue = startRange;
	else
		lowValue = (width*iterValue) / (numSections-1) + startRange;
	propV = lowValue;
	if (terraCap!=0 && !isImmune)
	{
		deltaV = centerValue-lowValue;

		if (abs(deltaV)<=terraCap)
			deltaV=0; //we are within the terra capacity, so no delta from optimal
		else if (deltaV<0)
			deltaV+=terraCap; //we are above center, and outside terra cap
		else deltaV-=terraCap;//we are below center, and outside terra cap
		//deltaHab now contains the absolute difference between optimal
		//and what the value can be terraformed to based on this iterations
		//terraforming assumptions
		propV = centerValue - deltaV;
	}

}

long habPointsRace(Race *whichRace)
{
	unsigned long isTotalTerraforming;
	double advantagePoints,tempLoopAccumulator,gravLoopAccumulator;
	signed long radLoopAccumulator,planetDesir;
	short terraformCapacity,deltaHabVector[3];
	char testPlanetHab[3];
		//order GTR

	advantagePoints = 0.0;
	isTotalTerraforming = whichRace->lrtVector[totalTerraforming_LRT];//getbit(playerData->grbit,GRBIT_LRT_TT);

	deltaHabVector[0]=deltaHabVector[1]=deltaHabVector[2]=0;
/*
	Generate planets within the hab range of the race and accumulate
	hab points.  For non immune habs slice the range into 11 segments,
	and generate planets in each segment.  Do full combinatorial
	across all 3 habs.  Iterate 3 times with different assumptions
	on terraforming capacity when generating planet value.
*/
	for (int terraFormIter = 0;terraFormIter<3;terraFormIter++)
	{
		Environment startRange;
		Environment habWidth;
		Environment centerE;
		Environment playerLow;
		Environment playerHigh;
		Environment numRangeSections;

		bool gravImmune = whichRace->environmentalImmunities.grav == 1;//playerData->centerHab[0]==-1;
		bool tempImmune = whichRace->environmentalImmunities.temp == 1;//playerData->centerHab[1]==-1;
		bool radImmune = whichRace->environmentalImmunities.rad == 1;//playerData->centerHab[2]==-1;

		centerE.grav = whichRace->optimalEnvironment.grav; // playerData->centerHab[0];
		centerE.temp = whichRace->optimalEnvironment.temp; // playerData->centerHab[1];
		centerE.rad  = whichRace->optimalEnvironment.rad;  // playerData->centerHab[2];

		playerLow.grav = whichRace->optimalEnvironment.grav - whichRace->environmentalTolerances.grav;// playerData->lowerHab[0];
		playerLow.temp = whichRace->optimalEnvironment.temp - whichRace->environmentalTolerances.temp;
		playerLow.rad  = whichRace->optimalEnvironment.rad - whichRace->environmentalTolerances.rad;

		playerHigh.grav = whichRace->optimalEnvironment.grav + whichRace->environmentalTolerances.grav;
		playerHigh.temp = whichRace->optimalEnvironment.temp + whichRace->environmentalTolerances.temp;
		playerHigh.rad  = whichRace->optimalEnvironment.rad + whichRace->environmentalTolerances.rad;

		if (playerLow.grav<0 )playerLow.grav = 0;
		if (playerLow.temp<0) playerLow.temp = 0;
		if (playerLow.rad <0) playerLow.rad = 0;
		if (playerHigh.grav > 100) playerHigh.grav = 100;
		if (playerHigh.temp > 100) playerHigh.temp = 100;
		if (playerHigh.rad >  100) playerHigh.rad = 100;

		if (terraFormIter==0)
			terraformCapacity=0;
		else
			if (terraFormIter==1)
			{
				if (isTotalTerraforming)
					terraformCapacity = 8;
				else
					terraformCapacity = 5;
			}
		else
			if (isTotalTerraforming)
				terraformCapacity = 17;
			else
				terraformCapacity = 15;

		if (gravImmune)
		{
			startRange.grav = 50;
			habWidth.grav = 11;
			numRangeSections.grav = 1;
		}
		else
		{
			startRange.grav = playerLow.grav-terraformCapacity;
			if (startRange.grav <0) startRange.grav = 0;
			int tmpHab = playerHigh.grav + terraformCapacity;
			if (tmpHab>100) tmpHab = 100;
			habWidth.grav = tmpHab - startRange.grav;
			numRangeSections.grav = 11;
		}

		if (tempImmune)
		{
			startRange.temp = 50;
			habWidth.temp = 11;
			numRangeSections.temp = 1;
		}
		else
		{
			startRange.temp = playerLow.temp-terraformCapacity;
			if (startRange.temp <0) startRange.temp = 0;
			int tmpHab = playerHigh.temp + terraformCapacity;
			if (tmpHab>100) tmpHab = 100;
			habWidth.temp = tmpHab - startRange.temp;
			numRangeSections.temp = 11;
		}

		if (radImmune)
		{
			startRange.rad = 50;
			habWidth.rad = 11;
			numRangeSections.rad = 1;
		}
		else
		{
			startRange.rad = playerLow.rad-terraformCapacity;
			if (startRange.rad <0) startRange.rad = 0;
			int tmpHab = playerHigh.rad + terraformCapacity;
			if (tmpHab>100) tmpHab = 100;
			habWidth.rad = tmpHab - startRange.rad;
			numRangeSections.rad = 11;
		}

		
		//the following loops set up hab values for a planet and tests
		//planet value
		//i j and k loops set up planet hab 0 1 and 2 indexes respectively
		//widths are subdivided by iter count, and a full cross section of
		//planets is generated.  Each h loop gives a different terra value
		//Final is scaled linearly with width.
        gravLoopAccumulator = 0.0;

		Planet *tempPlanet = new Planet();

		for (int gravIter = 0;gravIter<numRangeSections.grav;gravIter++)
        {
			tempLoopAccumulator = 0.0;
			for (int tempIter=0;tempIter<numRangeSections.temp;tempIter++)
			{
				radLoopAccumulator = 0;
				for(int radIter = 0; radIter < numRangeSections.rad; radIter++)
				{
					int proposedHabValue = 0;

					findProposedHab(gravImmune,gravIter,numRangeSections.grav,startRange.grav,centerE.grav,habWidth.grav,terraformCapacity,proposedHabValue,deltaHabVector[0]);
					testPlanetHab[0] = (char)proposedHabValue;
					tempPlanet->currentEnvironment.grav = proposedHabValue;
					findProposedHab(tempImmune,tempIter,numRangeSections.temp,startRange.temp,centerE.temp,habWidth.temp,terraformCapacity,proposedHabValue,deltaHabVector[1]);
					testPlanetHab[1] = (char)proposedHabValue;
					tempPlanet->currentEnvironment.temp = proposedHabValue;
					findProposedHab(radImmune,radIter,numRangeSections.rad,startRange.rad,centerE.rad,habWidth.rad,terraformCapacity,proposedHabValue,deltaHabVector[2]);
					testPlanetHab[2] = (char)proposedHabValue;
					tempPlanet->currentEnvironment.rad = proposedHabValue;

					planetDesir = planetValueCalcRace(tempPlanet->currentEnvironment,whichRace);

					int totalDeltaHab = deltaHabVector[0]+deltaHabVector[1]+deltaHabVector[2];
					if (totalDeltaHab>terraformCapacity)
					{
						planetDesir-=totalDeltaHab-terraformCapacity;
						if (planetDesir<0) planetDesir=0;
					}
					planetDesir *= planetDesir;
					switch (terraFormIter)
					{
						case 0: planetDesir*=7; break;
						case 1: planetDesir*=5; break;
						default: planetDesir*=6;
					}
					radLoopAccumulator+=planetDesir;
				}//for rad loop

				if (!radImmune) radLoopAccumulator = (radLoopAccumulator*habWidth.rad)/100;
				else radLoopAccumulator *= 11;

				tempLoopAccumulator += radLoopAccumulator;
			}//for temp loop
			if (!tempImmune) tempLoopAccumulator = (tempLoopAccumulator*habWidth.temp)/100;
			else tempLoopAccumulator *= 11;

			gravLoopAccumulator += tempLoopAccumulator;
        }//for grav loop
		delete tempPlanet;
		if (!gravImmune) gravLoopAccumulator = (gravLoopAccumulator*habWidth.grav)/100;
		else gravLoopAccumulator *= 11;

		advantagePoints += gravLoopAccumulator;
	} //for h loop

	return (long)(advantagePoints/10.0+0.5);
}

int calculateAdvantagePointsFromRace(Race* whichRace)
{
	int points = 1650;
	int hab;
	int PRT,grRateFactor,factoriesRaceCanOperate,resourcesProducedByTenFactories,tmpPoints;

#ifdef jdebug
	cout << "Step 1, points = " << points << endl;
#endif
	PRT = whichRace->primaryTrait;
	hab = habPointsRace(whichRace)/2000;

#ifdef jdebug
	cout << "Step 2, hab points = " << hab << endl;
#endif

	int raceGrowthRate = whichRace->growthRate;
	points -= growthRateCost[raceGrowthRate];
	grRateFactor = growthRateHabFactor[raceGrowthRate];
	points -= (hab*grRateFactor)/24;

#ifdef jdebug
	cout << "Step 3, points = " << points << endl;
#endif


	int numImmunities = 0;

	if (whichRace->environmentalImmunities.grav == 1)
		numImmunities++;
	else
		points += abs(whichRace->optimalEnvironment.grav - 50)*4;

	if (whichRace->environmentalImmunities.temp == 1)
		numImmunities++;
	else
		points += abs(whichRace->optimalEnvironment.temp - 50)*4;

	if (whichRace->environmentalImmunities.rad == 1)
		numImmunities++;
	else
		points += abs(whichRace->optimalEnvironment.rad - 50)*4;
	if (numImmunities>1) points -= 150;

#ifdef jdebug
	cout << "Step 4, points = " << points << endl;
#endif

	factoriesRaceCanOperate = whichRace->factoryBuildFactor;// playerData->stats[STAT_FAC_OPERATE];
	resourcesProducedByTenFactories = whichRace->factoryOutput;// playerData->stats[STAT_TEN_FAC_RES];

	if (factoriesRaceCanOperate>10 || resourcesProducedByTenFactories>10)
	{
		factoriesRaceCanOperate -= 9;
		if (factoriesRaceCanOperate<1)
			factoriesRaceCanOperate=1;

		resourcesProducedByTenFactories  -= 9;
		if (resourcesProducedByTenFactories<1)
			resourcesProducedByTenFactories=1;

		int multiplier = 2;
		if (PRT==hyperExpansionist_PRT)
			multiplier = 3;

		resourcesProducedByTenFactories *= multiplier;

		/*additional penalty for two- and three-immune*/
		if (numImmunities>=2)
			points -= ((resourcesProducedByTenFactories*factoriesRaceCanOperate)*raceGrowthRate)/2;
		else
			points -= ((resourcesProducedByTenFactories*factoriesRaceCanOperate)*raceGrowthRate)/9;
	}

	if (PRT != alternateReality_PRT)
	{
		int numColonistsGenerateUnitResource = whichRace->populationResourceUnit;// playerData->stats[STAT_RES_PER_COL]; //in hundreds
	
		points -= kColonistsForUnitRes[numColonistsGenerateUnitResource];
	}
	else
	{
		int energyProductionFactor = whichRace->populationResourceUnit; //AR overlaps into this
		points -= kColonistsForUnitRes[energyProductionFactor];
	}

#ifdef jdebug
	cout << "Step 5, points = " << points << endl;
#endif


	if (PRT != alternateReality_PRT)
	{
		tmpPoints=0;

		tmpPoints += facProdCost[whichRace->factoryOutput];//playerData->stats[STAT_TEN_FAC_RES]];
		tmpPoints += facResReqCost[whichRace->factoryResourceCost];//playerData->stats[STAT_FAC_COST]];
		tmpPoints += facNumOperCost[whichRace->factoryBuildFactor];//playerData->stats[STAT_FAC_OPERATE]];

		tmpPoints += mineProdCost[whichRace->mineOutput];//playerData->stats[STAT_TEN_MINES_PROD]];
		tmpPoints += mineResReqCost[whichRace->mineResourceCost];//playerData->stats[STAT_MINE_COST]];
		tmpPoints += mineNumOperCost[whichRace->mineBuildFactor];//playerData->stats[STAT_MINE_OPERATE]];

		if (whichRace->factoryMineralCost.cargoDetail[_germanium].amount == 3) tmpPoints += 175;
//		if (getbit(playerData->grbit,GRBIT_FACT_GER_LESS) != 0) tmpPoints += 175;

		points -= tmpPoints;

	}
	else points += 210; /* AR */
	
#ifdef jdebug
	cout << "Step 6, points = " << points << endl;
#endif

	/*LRTs*/
	points -= prtCost[PRT];
	int numHandicap = 0;
	int numBenficial = 0;
        int j; // MS bug
	for(j=0;j<numberOf_LRT;j++)
	{
		if (whichRace->lrtVector[j] == 1) //(getbit(playerData->grbit,j)!=0)
		{
			if (lrtCost[j]<=0) numHandicap++;
			else numBenficial++;
			points -= lrtCost[j];
		}
	}
	if ((numBenficial+numHandicap)>4)
		points -= (numBenficial+numHandicap)*(numBenficial+numHandicap-4)*10;
	if ((numHandicap-numBenficial)>3)
		points -= (numHandicap-numBenficial-3)*60;
	if ((numBenficial-numHandicap)>3)
		points -= (numBenficial-numHandicap-3)*40;

	if (whichRace->lrtVector[noAdvancedScanners_LRT]==1)//	if (getbit(playerData->grbit,GRBIT_LRT_NAS) != 0)
	{
		if (PRT == packetPhysicist_PRT) points -= 280;
		else if (PRT == superStealth_PRT) points -= 200;
		else if (PRT == jackOfAllTrades_PRT) points -= 40;
	}

#ifdef jdebug
	cout << "Step 7, points = " << points << endl;
#endif

	/*science*/
	tmpPoints=0;

	
//	for (int j=STAT_ENERGY_COST;j<=STAT_BIO_COST;j++)
//		tmpPoints += playerData->stats[j];
	for (j = 0 ; j < numberOf_TECH ; j++)
	{
		if (whichRace->techCostMultiplier.techLevels[j] == 1)
			tmpPoints += 1;//normal cost
		else if (whichRace->techCostMultiplier.techLevels[j] == 0)
			tmpPoints += 2; //expensive cost
		else if (whichRace->techCostMultiplier.techLevels[j] == 2)
			tmpPoints += 0; //yes, redundant I know, but it may change?
		else
		{
			cout << "Error -- invalid tech cost multiplier for race "
				  <<whichRace->singularName<<" value = "
				  <<whichRace->techCostMultiplier.techLevels[j]<<endl;
		}
	}
	points += scienceLadder[tmpPoints];
	
	//punish races with efficient populations and expensive techs
	//why?  I have no clue but there you have it.
	if (tmpPoints<2 && whichRace->populationResourceUnit<10)//playerData->stats[STAT_RES_PER_COL]<10)
			points -= 190;
	
	
//	if (getbit(playerData->grbit,GRBIT_TECH75_LVL3) != 0) points-=180;
	if (whichRace->expensiveStartAtThree == 1) points -= 180;
//	if (PRT==alternateReality_PRT && playerData->stats[STAT_ENERGY_COST]==2/*50% less*/) points -= 100;
	if (PRT==alternateReality_PRT && whichRace->techCostMultiplier.techLevels[energy_TECH]==0) points -= 100;

#ifdef jdebug
	cout << "Step 8, points = " << points << endl;

	cout << "Returning final value of " << points/3 << endl;
#endif

	return points/3;
}

//#endif

char gtmp[256];
char ttmp[256];
char rtmp[256];

const char* gravString(int grav)
{
	double f = 0.12;
	f += (8.0-0.12)/100.0 * (double)grav;
	sprintf(gtmp,"%2.2fg",f);
	return gtmp;
}
const char* tempString(int temp)
{
	sprintf(ttmp,"%dC",(temp-50)*4);
	return ttmp;
}
const char* radString(int rad)
{
	sprintf(rtmp,"%dmR",rad);
	return rtmp;
}

void Race::clear()
{
	while (prtVector.size() < numberOf_PRT) prtVector.push_back(0);
	while (lrtVector.size() < numberOf_LRT) lrtVector.push_back(0);
        int i; // MS bug
	for (i = 0; i < numberOf_PRT;i++) prtVector[i] = 0;
	for (i=0; i<numberOf_LRT; i++) lrtVector[i] = 0;
	growthRate = 0;
	expensiveStartAtThree = 0;

	currentTerraformingAbility.clear();
	environmentalImmunities.clear();
	environmentalTolerances.clear();
	factoryBuildFactor = 0;
	factoryMineralCost.clear();
	factoryOutput = 0;
	factoryResourceCost = 0;
	mineBuildFactor = 0;
	mineMineralCost.clear();
	mineOutput = 0;
	mineResourceCost = 0;
	optimalEnvironment.clear();
	ownerID = 0;
	pluralName = "Empty Race";
	populationMiningFactor = 0;
	populationMiningUnit = 0;
	populationResourceFactor = 0;
	populationResourceUnit = 0;
	primaryTrait = 0;
	singularName = "Empty Race";
	techCostMultiplier.clear();
	techMiningFactorBreak.clear();
	techResourceFactorBreak.clear();
	extraPointAllocation = enum_surfaceMinerals;
}
Race *makeNewHumanoidsRace()
{
	Race *humans = new Race();

	humans->clear();//clearRace(humans);
	humans->prtVector[jackOfAllTrades_PRT] = 1;
	humans->primaryTrait = jackOfAllTrades_PRT;

	//no lrts for the humans

	humans->growthRate = 15;
	humans->optimalEnvironment.grav = 50;
	humans->optimalEnvironment.temp = 50;
	humans->optimalEnvironment.rad  = 50;

	humans->environmentalTolerances.grav = 35;
	humans->environmentalTolerances.temp = 35;
	humans->environmentalTolerances.rad  = 35;

	humans->factoryBuildFactor = 10;
	humans->factoryMineralCost.cargoDetail[_germanium].amount = 4;
	humans->factoryOutput = 10;
	humans->factoryResourceCost = 10;

	humans->mineBuildFactor = 10;
	humans->mineOutput = 10;
	humans->mineResourceCost = 5;
	humans->pluralName = "Humanoids";
	humans->populationMiningFactor = 0;
	humans->populationResourceFactor = 1;
	humans->populationResourceUnit = 10;
	humans->primaryTrait = jackOfAllTrades_PRT;
	humans->singularName = "Humanoid";
	humans->techCostMultiplier.techLevels[weapons_TECH]      = 1;
	humans->techCostMultiplier.techLevels[construction_TECH] = 1;
	humans->techCostMultiplier.techLevels[energy_TECH]       = 1;
	humans->techCostMultiplier.techLevels[propulsion_TECH]   = 1;
	humans->techCostMultiplier.techLevels[electronics_TECH]  = 1;
	humans->techCostMultiplier.techLevels[biotech_TECH]      = 1;

	return humans;
}
Race *makeNewRabitoidsRace()
{
	Race *theRace = new Race();

	theRace->clear();//clearRace(humans);
	theRace->prtVector[interstellarTraveller_PRT] = 1;
	theRace->primaryTrait = interstellarTraveller_PRT;

	theRace->lrtVector[improvedFuelEfficiency_LRT] = 1;
	theRace->lrtVector[totalTerraforming_LRT] = 1;
	theRace->lrtVector[cheapEngines_LRT] = 1;
	theRace->lrtVector[noAdvancedScanners_LRT] = 1;

	theRace->growthRate = 20;
	theRace->optimalEnvironment.grav = 33;
	theRace->optimalEnvironment.temp = 58;
	theRace->optimalEnvironment.rad  = 33;

	theRace->environmentalTolerances.grav = 23;
	theRace->environmentalTolerances.temp = 23;
	theRace->environmentalTolerances.rad  = 20;

	//Factory Settings
	theRace->factoryOutput = 10;
	theRace->factoryResourceCost = 9;
	theRace->factoryBuildFactor = 17;
	theRace->factoryMineralCost.cargoDetail[_germanium].amount = 3;

	//mine settings
	theRace->mineOutput = 10;
	theRace->mineResourceCost = 9;
	theRace->mineBuildFactor = 10;

	theRace->populationMiningFactor = 0;
	theRace->populationResourceFactor = 1;
	theRace->populationResourceUnit = 10;

	theRace->pluralName = "Rabitoids";
	theRace->singularName = "Rabitoid";
	theRace->techCostMultiplier.techLevels[weapons_TECH]      = 2;
	theRace->techCostMultiplier.techLevels[construction_TECH] = 1;
	theRace->techCostMultiplier.techLevels[energy_TECH]       = 2;
	theRace->techCostMultiplier.techLevels[propulsion_TECH]   = 0;
	theRace->techCostMultiplier.techLevels[electronics_TECH]  = 1;
	theRace->techCostMultiplier.techLevels[biotech_TECH]      = 0;
	theRace->expensiveStartAtThree = 0;
	return theRace;
}
Race *makeNewInsectoidsRace()
{
	Race *theRace = new Race();

	theRace->clear();
	theRace->prtVector[warMonger_PRT] = 1;
	theRace->primaryTrait = warMonger_PRT;

	theRace->lrtVector[improvedStarbases_LRT] = 1;
	theRace->lrtVector[regeneratingShields_LRT] = 1;
	theRace->lrtVector[cheapEngines_LRT] = 1;

	theRace->growthRate = 10;

	theRace->optimalEnvironment.grav = 50;
	theRace->optimalEnvironment.temp = 50;
	theRace->optimalEnvironment.rad  = 85;

	theRace->environmentalImmunities.grav = 1;

	theRace->environmentalTolerances.grav = 50;
	theRace->environmentalTolerances.temp = 50;
	theRace->environmentalTolerances.rad  = 15;

	//Factory Settings
	theRace->factoryOutput = 10;
	theRace->factoryResourceCost = 10;
	theRace->factoryBuildFactor = 10;
	theRace->factoryMineralCost.cargoDetail[_germanium].amount = 4;

	//mine settings
	theRace->mineOutput = 9;
	theRace->mineResourceCost = 10;
	theRace->mineBuildFactor = 6;

	theRace->populationMiningFactor = 0;
	theRace->populationResourceFactor = 1;
	theRace->populationResourceUnit = 10;

	theRace->pluralName = "Insectoids";
	theRace->singularName = "Insectoid";
	theRace->techCostMultiplier.techLevels[weapons_TECH]      = 0;
	theRace->techCostMultiplier.techLevels[construction_TECH] = 0;
	theRace->techCostMultiplier.techLevels[energy_TECH]       = 0;
	theRace->techCostMultiplier.techLevels[propulsion_TECH]   = 0;
	theRace->techCostMultiplier.techLevels[electronics_TECH]  = 1;
	theRace->techCostMultiplier.techLevels[biotech_TECH]      = 2;
	theRace->expensiveStartAtThree = 0;
	return theRace;
}
Race *makeNewNucleotidsRace()
{
	Race *theRace = new Race();

	theRace->clear();
	theRace->prtVector[superStealth_PRT] = 1;
	theRace->primaryTrait = superStealth_PRT;

	theRace->lrtVector[improvedStarbases_LRT] = 1;
	theRace->lrtVector[advancedRemoteMining_LRT] = 1;

	theRace->growthRate = 10;

	theRace->optimalEnvironment.grav = 50;
	theRace->optimalEnvironment.temp = 50;
	theRace->optimalEnvironment.rad  = 50;

	theRace->environmentalImmunities.grav = 1;

	theRace->environmentalTolerances.grav = 50;
	theRace->environmentalTolerances.temp = 38;
	theRace->environmentalTolerances.rad  = 50;

	//Factory Settings
	theRace->factoryOutput = 10;
	theRace->factoryResourceCost = 10;
	theRace->factoryBuildFactor = 10;
	theRace->factoryMineralCost.cargoDetail[_germanium].amount = 4;

	//mine settings
	theRace->mineOutput = 10;
	theRace->mineResourceCost = 15;
	theRace->mineBuildFactor = 5;

	theRace->populationMiningFactor = 0;
	theRace->populationResourceFactor = 1;
	theRace->populationResourceUnit = 9;

	theRace->pluralName = "Nucleotids";
	theRace->singularName = "Nucleotid";
	theRace->techCostMultiplier.techLevels[weapons_TECH]      = 2;
	theRace->techCostMultiplier.techLevels[construction_TECH] = 2;
	theRace->techCostMultiplier.techLevels[energy_TECH]       = 2;
	theRace->techCostMultiplier.techLevels[propulsion_TECH]   = 2;
	theRace->techCostMultiplier.techLevels[electronics_TECH]  = 2;
	theRace->techCostMultiplier.techLevels[biotech_TECH]      = 2;
	theRace->expensiveStartAtThree = 1;
	return theRace;
}
Race *makeNewSiliconoidsRace()
{
	Race *theRace = new Race();

	theRace->clear();
	theRace->prtVector[hyperExpansionist_PRT] = 1;
	theRace->primaryTrait = hyperExpansionist_PRT;

	theRace->lrtVector[improvedFuelEfficiency_LRT] = 1;
	theRace->lrtVector[ultimateRecycling_LRT] = 1;
	theRace->lrtVector[onlyBasicRemoteMining_LRT] = 1;
	theRace->lrtVector[bleedingEdgeTechnology_LRT] = 1;

	theRace->growthRate = 6;

	theRace->optimalEnvironment.grav = 50;
	theRace->optimalEnvironment.temp = 50;
	theRace->optimalEnvironment.rad  = 50;

	theRace->environmentalImmunities.grav = 1;
	theRace->environmentalImmunities.temp = 1;
	theRace->environmentalImmunities.rad  = 1;

	theRace->environmentalTolerances.grav = 50;
	theRace->environmentalTolerances.temp = 50;
	theRace->environmentalTolerances.rad  = 50;

	//Factory Settings
	theRace->factoryOutput = 12;
	theRace->factoryResourceCost = 12;
	theRace->factoryBuildFactor = 15;
	theRace->factoryMineralCost.cargoDetail[_germanium].amount = 4;

	//mine settings
	theRace->mineOutput = 10;
	theRace->mineResourceCost =9;
	theRace->mineBuildFactor = 10;

	theRace->populationMiningFactor = 0;
	theRace->populationResourceFactor = 1;
	theRace->populationResourceUnit = 8;

	theRace->pluralName = "Siliconoids";
	theRace->singularName = "Siliconoid";
	theRace->techCostMultiplier.techLevels[weapons_TECH]      = 1;
	theRace->techCostMultiplier.techLevels[construction_TECH] = 0;
	theRace->techCostMultiplier.techLevels[energy_TECH]       = 1;
	theRace->techCostMultiplier.techLevels[propulsion_TECH]   = 0;
	theRace->techCostMultiplier.techLevels[electronics_TECH]  = 1;
	theRace->techCostMultiplier.techLevels[biotech_TECH]      = 2;
	theRace->expensiveStartAtThree = 0;
	return theRace;
}
Race *makeNewAntetherialsRace()
{
	Race *theRace = new Race();

	theRace->clear();
	theRace->prtVector[spaceDemolition_PRT] = 1;
	theRace->primaryTrait = spaceDemolition_PRT;

	theRace->lrtVector[mineralAlchemy_LRT] = 1;
	theRace->lrtVector[advancedRemoteMining_LRT] = 1;
	theRace->lrtVector[noRamscoopEngines_LRT] = 1;
	theRace->lrtVector[cheapEngines_LRT] = 1;
	theRace->lrtVector[noAdvancedScanners_LRT] = 1;

	theRace->growthRate = 7;

	theRace->optimalEnvironment.grav = 15;
	theRace->optimalEnvironment.temp = 50;
	theRace->optimalEnvironment.rad  = 85;

	theRace->environmentalTolerances.grav = 15;
	theRace->environmentalTolerances.temp = 50;
	theRace->environmentalTolerances.rad  = 15;

	//Factory Settings
	theRace->factoryOutput = 11;
	theRace->factoryResourceCost = 10;
	theRace->factoryBuildFactor = 18;
	theRace->factoryMineralCost.cargoDetail[_germanium].amount = 4;

	//mine settings
	theRace->mineOutput = 10;
	theRace->mineResourceCost = 10;
	theRace->mineBuildFactor = 10;

	theRace->populationMiningFactor = 0;
	theRace->populationResourceFactor = 1;
	theRace->populationResourceUnit = 7;

	theRace->pluralName = "Antetheral";
	theRace->singularName = "Antetherals";
	theRace->techCostMultiplier.techLevels[weapons_TECH]      = 2;
	theRace->techCostMultiplier.techLevels[construction_TECH] = 0;
	theRace->techCostMultiplier.techLevels[energy_TECH]       = 0;
	theRace->techCostMultiplier.techLevels[propulsion_TECH]   = 0;
	theRace->techCostMultiplier.techLevels[electronics_TECH]  = 0;
	theRace->techCostMultiplier.techLevels[biotech_TECH]      = 0;
	theRace->expensiveStartAtThree = 0;
	return theRace;
}

Race* raceFactory(unsigned i)
{
   switch( i )
   {
   case 0: return makeNewHumanoidsRace();
   case 1: return makeNewSiliconoidsRace();
   case 2: return makeNewRabitoidsRace();
   case 3: return makeNewAntetherialsRace();
   case 4: return makeNewInsectoidsRace();
   case 5: return NULL; // random
   case 6: return makeNewNucleotidsRace();
   }
   return NULL;
}

int go()
{
	initRaceWizard();
	// checking for hardcoded humanoids race
#ifdef INCLUDE_ORIGINAL_RACE_CODE
	player.centerHab[0]=player.centerHab[1]=player.centerHab[2]=50;
	player.lowerHab[0]=player.lowerHab[1]=player.lowerHab[2]=15;
	player.upperHab[0]=player.upperHab[1]=player.upperHab[2]=85;
	player.growthRate=15;
	player.stats[0]=10;
	player.stats[1]=10;
	player.stats[2]=10;
	player.stats[3]=10;
	player.stats[4]=10;
	player.stats[5]=5;
	player.stats[6]=10;
	player.stats[7]=0;
	//0 for expensive tech 1 for normal tech 2 for cheap tech
	player.stats[8]=1;
	player.stats[9]=1;
	player.stats[10]=1;
	player.stats[11]=1;
	player.stats[12]=1;
	player.stats[13]=1;
	player.stats[14]=jackOfAllTrades_PRT;
	player.stats[15]=0;
	player.grbit=0;

	cout << "adv points: " << calculateAdvantagePoints(&player)
		<< "\t\tmust be: " << 25 << endl;



	

	player.lowerHab[0]=player.lowerHab[1]=player.lowerHab[2]=40;
	player.upperHab[0]=player.upperHab[1]=player.upperHab[2]=60;
	cout << "adv points: " << calculateAdvantagePoints(&player)
		<< "\t\tmust be: 534"<< endl;


	// checking for hardcoded rabbitoids race
	player.centerHab[0]=33;	player.centerHab[1]=58;	player.centerHab[2]=33;
	player.lowerHab[0]=10;	player.lowerHab[1]=35;	player.lowerHab[2]=13;
	player.upperHab[0]=56;	player.upperHab[1]=81;	player.upperHab[2]=53;
	player.growthRate=20;
	player.stats[0]=10;
	player.stats[1]=10;
	player.stats[2]=9;
	player.stats[3]=17;
	player.stats[4]=10;
	player.stats[5]=9;
	player.stats[6]=10;
	player.stats[7]=4;
	player.stats[8]=0;
	player.stats[9]=0;
	player.stats[10]=2;
	player.stats[11]=1;
	player.stats[12]=1;
	player.stats[13]=2;
	player.stats[14]=interstellarTraveller_PRT;
	player.stats[15]=0;
	player.grbit=0x80000503;

	cout << "adv points: " << calculateAdvantagePoints(&player)
		<< "\t\tmust be: " << 32 << endl;


	// checking for hardcoded insectoids race
	player.centerHab[0]=-1;	player.centerHab[1]=50;	player.centerHab[2]=85;
	player.lowerHab[0]=-1;	player.lowerHab[1]=0;	player.lowerHab[2]=70;
	player.upperHab[0]=-1;	player.upperHab[1]=100;	player.upperHab[2]=100;
	player.growthRate=10;
	player.stats[0]=10;
	player.stats[1]=10;
	player.stats[2]=10;
	player.stats[3]=10;
	player.stats[4]=9;
	player.stats[5]=10;
	player.stats[6]=6;
	player.stats[7]=1;
	player.stats[8]=2;
	player.stats[9]=2;
	player.stats[10]=2;
	player.stats[11]=2;
	player.stats[12]=1;
	player.stats[13]=0;
	player.stats[14]=warMonger_PRT;
	player.stats[15]=0;
	player.grbit=0x2108;

	cout << "adv points: " << calculateAdvantagePoints(&player)
		<< "\t\tmust be: " << 43 << endl;


	// checking for hardcoded nucleotids race
	player.centerHab[0]=-1;	player.centerHab[1]=50;	player.centerHab[2]=50;
	player.lowerHab[0]=-1;	player.lowerHab[1]=12;	player.lowerHab[2]=0;
	player.upperHab[0]=-1;	player.upperHab[1]=88;	player.upperHab[2]=100;
	player.growthRate=10;
	player.stats[0]=9;
	player.stats[1]=10;
	player.stats[2]=10;
	player.stats[3]=10;
	player.stats[4]=10;
	player.stats[5]=15;
	player.stats[6]=5;
	player.stats[7]=0;
	player.stats[8]=0;
	player.stats[9]=0;
	player.stats[10]=0;
	player.stats[11]=0;
	player.stats[12]=0;
	player.stats[13]=0;
	player.stats[14]=superStealth_PRT;
	player.stats[15]=0;
	player.grbit=0x2000000C;

	cout << "adv points: " << calculateAdvantagePoints(&player)
		<< "\t\tmust be: " << 11 << endl;


	// checking for hardcoded silicanoids race
	player.centerHab[0]=-1;	player.centerHab[1]=-1;	player.centerHab[2]=-1;
	player.lowerHab[0]=-1;	player.lowerHab[1]=-1;	player.lowerHab[2]=-1;
	player.upperHab[0]=-1;	player.upperHab[1]=-1;	player.upperHab[2]=-1;
	player.growthRate=6;
	player.stats[0]=8;
	player.stats[1]=12;
	player.stats[2]=12;
	player.stats[3]=15;
	player.stats[4]=10;
	player.stats[5]=9;
	player.stats[6]=10;
	player.stats[7]=3;
	player.stats[8]=1;
	player.stats[9]=1;
	player.stats[10]=2;
	player.stats[11]=2;
	player.stats[12]=1;
	player.stats[13]=0;
	player.stats[14]=hyperExpansionist_PRT;
	player.stats[15]=0;
	player.grbit=0x1221;

	cout << "adv points: " << calculateAdvantagePoints(&player)
		<< "\t\tmust be: " << 9 << endl;


	// checking for hardcoded antetherals race
	player.centerHab[0]=15;	player.centerHab[1]=50;	player.centerHab[2]=85;
	player.lowerHab[0]=0;	player.lowerHab[1]=0;	player.lowerHab[2]=70;
	player.upperHab[0]=30;	player.upperHab[1]=100;	player.upperHab[2]=100;
	player.growthRate=7;
	player.stats[0]=7;
	player.stats[1]=11;
	player.stats[2]=10;
	player.stats[3]=18;
	player.stats[4]=10;
	player.stats[5]=10;
	player.stats[6]=10;
	player.stats[7]=0;
	player.stats[8]=2;
	player.stats[9]=0;
	player.stats[10]=2;
	player.stats[11]=2;
	player.stats[12]=2;
	player.stats[13]=2;
	player.stats[14]=spaceDemolition_PRT;
	player.stats[15]=0;
	player.grbit=0x5C4;

	cout << "adv points: " << calculateAdvantagePoints(&player)
		<< "\t\tmust be: " << 7 << endl;

#endif //INCLUDE_ORIGINAL_RACE_CODE
	Race *testRace = makeNewHumanoidsRace();
	cout << "***"<<testRace->pluralName<<" \tCALC points: "<<calculateAdvantagePointsFromRace(testRace)<< "\t\tmust be: 25"<<endl;
	testRace->environmentalTolerances.grav = 10;
	testRace->environmentalTolerances.temp = 10;
	testRace->environmentalTolerances.rad  = 10;
	testRace->pluralName = "Dummies";
	cout << "***"<<testRace->pluralName<<" \tCALC points: "<<calculateAdvantagePointsFromRace(testRace)<< "\t\tmust be: 534"<<endl;
	delete testRace;
	testRace = makeNewRabitoidsRace();
	cout << "***"<<testRace->pluralName<<" \tCALC points: "<<calculateAdvantagePointsFromRace(testRace)<< "\t\tmust be: 32"<<endl;
	delete testRace;
	testRace = makeNewInsectoidsRace();
	cout << "***"<<testRace->pluralName<<" \tCALC points: "<<calculateAdvantagePointsFromRace(testRace)<< "\t\tmust be: 43"<<endl;
	delete testRace;
	testRace = makeNewNucleotidsRace();
	cout << "***"<<testRace->pluralName<<" \tCALC points: "<<calculateAdvantagePointsFromRace(testRace)<< "\t\tmust be: 11"<<endl;
	delete testRace;
	testRace = makeNewSiliconoidsRace();
	cout << "***"<<testRace->pluralName<<" \tCALC points: "<<calculateAdvantagePointsFromRace(testRace)<< "\t\tmust be: 9"<<endl;
	delete testRace;
	testRace = makeNewAntetherialsRace();
	cout << "***"<<testRace->pluralName<<" \tCALC points: "<<calculateAdvantagePointsFromRace(testRace)<< "\t\tmust be: 7"<<endl;
	delete testRace;


	return 0;
}

