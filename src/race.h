#ifndef RACE__H
#define RACE__H
//
// C++ Interface: race
//
// Copyright: See COPYING file that comes with this distribution
//
#include "classDef.h"
#include "planet.h"

enum LeftoverPointsEnum
{
	enum_surfaceMinerals = 0,
	enum_mineralConcentrations,
	enum_factories,
	enum_mines,
	enum_defenses
};

enum lesserRaceTraits
{
	improvedFuelEfficiency_LRT=0,
	totalTerraforming_LRT,
	advancedRemoteMining_LRT,
	improvedStarbases_LRT,
	generalizedResearch_LRT,
	ultimateRecycling_LRT,
	mineralAlchemy_LRT,
	noRamscoopEngines_LRT,
	cheapEngines_LRT,
	onlyBasicRemoteMining_LRT,
	noAdvancedScanners_LRT,
	lowStartingPopulation_LRT,
	bleedingEdgeTechnology_LRT,
	regeneratingShields_LRT,
	numberOf_LRT
};

enum primaryRaceTraits
{
    jackOfAllTrades_PRT=0,
    alternateReality_PRT,
    interstellarTraveller_PRT,
    packetPhysicist_PRT,
    spaceDemolition_PRT,
    innerStrength_PRT,
    claimAdjuster_PRT,
    warMonger_PRT,
    superStealth_PRT,
    hyperExpansionist_PRT,
    numberOf_PRT
};

class Race
{
public: // data
	std::string singularName;
	std::string   pluralName;

	Environment optimalEnvironment;
	Environment environmentalTolerances;
	Environment environmentalImmunities;
	Environment currentTerraformingAbility;

	unsigned    ownerID;

	StarsTechnology techCostMultiplier;

	// These factors are for AR type factor breaks
	// in the AR resource calculation
	StarsTechnology techMiningFactorBreak;
	StarsTechnology techResourceFactorBreak;

	std::vector<unsigned>     lrtVector;
	std::vector<unsigned>     prtVector;
	int						  primaryTrait;
	unsigned factoryOutput;
	unsigned mineOutput;

	CargoManifest factoryMineralCost;
	CargoManifest mineMineralCost;
	unsigned factoryResourceCost;
	unsigned mineResourceCost;

	// number buildable per unit population
	unsigned factoryBuildFactor;
	unsigned mineBuildFactor;

	unsigned populationResourceUnit;
	unsigned populationMiningUnit;
	unsigned populationResourceFactor; //amount of resources per unit population
	unsigned populationMiningFactor; //amount mined per unit population

	int growthRate;
	int expensiveStartAtThree;
	LeftoverPointsEnum extraPointAllocation;

public: // methods
	bool   initRace(mxml_node_t *tree);
	void   clear(void);

	void   toXMLString(std::string &theString) const;
	void   raceToString   (std::string &theString);

	unsigned getMaxPop(Planet *whichPlanet) const;

	// return -15.0..100.0
	double getGrowthRate(Planet *whichPlanet) const;

	 // minerals extracted by this race on this planet
    CargoManifest getMineralsExtracted(Planet *whichPlanet);

	 // Resource output for this race on this planet
    unsigned getResourceOutput(Planet *whichPlanet);

    void getMaxInfrastructure(Planet *whichPlanet, PlanetaryInfrastructure &infra);

	 // true if this race can terraform this planet to ideal in any one field
	 //Ned, not sure why you'd want that calculation...
    bool   canTerraformPlanet (Planet *whichPlanet, GameData &gd) const;

    Environment getTerraformAmount(Planet *whichPlanet,
                                   bool isMinTerraform,
                                   GameData &gd);//returns the env. change for 1 unit of terra

	 // return env. of planet if fully terraformed
    Environment getFinalTerraformEnvironment(Planet *whichPlanet);

    void updateTerraformAbility(GameData &gd);
};

void initRaceWizard(void);
bool parseRaces(mxml_node_t *tree, std::vector<Race*> &list);
void racesToXMLString(const std::vector<Race*> &list, std::string &theString);
long planetValueCalcRace(const Environment &env, const Race *whichRace);
Race* raceFactory(unsigned i);
int calculateAdvantagePointsFromRace(Race* whichRace);

// population is stored in units of 100 per kilotonne
static const unsigned populationCargoUnit = 100;

#endif

