#ifdef MSVC
#pragma warning(disable: 4786)
#endif
/*****************************************************************************
                          minefield -  description
                             -------------------
    begin                : 23 Nov 05
    copyright            : (C) 2005
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
#include <math.h>

static inline double square(double f)
{
	return f * f;
}

// return intercept points for the given sphere for a line defined by p1 and p2
// note that since the sim is currently 2D, everything is projected down into the XY plane
// return an array of 7 doubles, the first being the number of intercept points followed
// by the 2 points in x,y,z tuples
double* vectorSphereIntercepts (Location p1, Location p2, Location sphere, double sphere_r)
{
	double point_x1 = (double) p1.getX();
	double point_y1 = (double) p1.getY();
	double point_z1 = 0.0;
	double point_x2 = (double) p2.getX();
	double point_y2 = (double) p2.getY();
	double point_z2 = 0.0;
	double sphere_x3 = (double) sphere.getX();
	double sphere_y3 = (double) sphere.getY();
	double sphere_z3 = 0.0;

	double a, b, c, mu, i ;
	double * output;
	output = (double*) malloc(7*sizeof(double));

	a = square(point_x2 - point_x1) + square(point_y2 - point_y1) + square(point_z2 - point_z1);

	b = 2 * ( (point_x2 - point_x1) * (point_x1 - sphere_x3) +
	          (point_y2 - point_y1) * (point_y1 - sphere_y3) +
	          (point_z2 - point_z1) * (point_z1 - sphere_z3) );

	c =  square(sphere_x3) + square(sphere_y3) +
	     square(sphere_z3) + square(point_x1)  +
	     square(point_y1)  + square(point_z1)  -
	     2 * ( sphere_x3*point_x1 + sphere_y3*point_y1 + sphere_z3*point_z1 ) - square(sphere_r) ;

	i = (b * b) - (4 * a * c);

	output[0] = 0.0;

	if( i < 0.0 )
	{
		// no intersection
		output[0] = 0.0;
		return(output);
	}

	if( i == 0.0 )
	{
		// one intersection
		output[0] = 1.0;

		mu = -b/(2*a) ;
		output[1] = point_x1 + mu*(point_x2-point_x1);
		output[2] = point_y1 + mu*(point_y2-point_y1);
		output[3] = point_z1 + mu*(point_z2-point_z1);
		return output;
	}

    if( i > 0.0 )
    {
        // two intersections
        output[0] = 2.0;

        // first intersection
        mu = (-b + sqrt( square(b) - 4*a*c )) / (2*a);
        output[1] = point_x1 + mu*(point_x2-point_x1);
        output[2] = point_y1 + mu*(point_y2-point_y1);
        output[3] = point_z1 + mu*(point_z2-point_z1);
        // second intersection
        mu = (-b - sqrt(square(b) - 4*a*c )) / (2*a);
        output[4] = point_x1 + mu*(point_x2-point_x1);
        output[5] = point_y1 + mu*(point_y2-point_y1);
        output[6] = point_z1 + mu*(point_z2-point_z1);

        return(output);
    }

    cerr << "Fatal error in math routine: vectorSphereIntercepts while calculating minefield intercepts.\n Sorry\n";
    starsExit("minefield.cpp",-1);
    return output;
}

unsigned distanceTraveledBeforeStruck(unsigned distanceRequired, double hitProb) //distance in LY, hitProb in %
{
    if( distanceRequired == 0 )
    {
       cerr<<"Error -- called distanceTraveledBeforeStruck() with 0 distance\n";
       starsExit("minefield.cpp",-1);
    }

    for(unsigned i = 0; i < distanceRequired; i++)
    {
        double rNum = (double)(((double)rand())/((double)(RAND_MAX)+1.0))*100.0;
        //cerr<<rNum<<"("<<hitProb<<")\n";
        if (rNum<=hitProb)
            return i;
    }

    return distanceRequired;
}

Minefield::Minefield(void)
{
	nonPenScanCloakValue = 85;
	penScanCloakValue = 0;
	radius = 0;
	for(unsigned i = 0; i <_numberOfMineTypes; i++)
		mineComposition[i] = 0;
}

bool Minefield::actionHandler(GameData &gd, const Action &what)
{
	if( what.ID != decay_mines_action )
	{
		std::cerr << "Error, minefield " << name << " #" << objectId
		    << " was sent an unhandleable action " << what.description
		    << std::endl;
		starsExit("minefield.cpp", -1);
	}
	//else
	decay(gd);

	return true;
}

void Minefield::addMines(Location l, int addComposition[])
{
	if( !canAdd(l, addComposition) )
	{
		cerr<<"Error -- attempted to add dissimilar mines to field id "<<objectId<<endl;
		starsExit("minefield.cpp",-1);
	}

	unsigned numMines = 0;
	for(unsigned i = 0; i < _numberOfMineTypes; ++i)
	{
		mineComposition[i] += addComposition[i];
		numMines += mineComposition[i];
	}
	double r = sqrt(double(numMines));
	radius = (unsigned)ceil(r);
}

bool Minefield::canAdd(Location l, int addComposition[]) const
{
	if( !withinRadius(l) )
		return false;

	int numTypeInProposedField = 0;
	for(unsigned i = 0; i < _numberOfMineTypes; ++i)
	{
		if( mineComposition[i] != 0 || addComposition[i] != 0 )
			++numTypeInProposedField;
	}

	return numTypeInProposedField <= 1;
}

#define HEAVY_DAMAGE 500
#define HEAVY_KICKER 100
#define HEAVY_MIN 2000
#define HEAVY_MIN_KICKER 500
#define HEAVY_MIN_ENGINE_QUAN 4
#define HEAVY_MIN_SCOOP_QUAN_KICKER 1

#define STANDARD_DAMAGE 100
#define STANDARD_KICKER 25
#define STANDARD_MIN 500
#define STANDARD_MIN_KICKER 100
#define STANDARD_MIN_ENGINE_QUAN 5
#define STANDARD_MIN_SCOOP_QUAN_KICKER 1

void Minefield::damageFleet(GameData &gd, Fleet *theFleet)
{
	unsigned totalShips = 0;
	unsigned totalEngines = 0;
	unsigned numScoop = 0;
	unsigned numDestroyed = 0;
	unsigned damageAbsorbedByShields = 0;
	unsigned totalDamage = 0;
	bool accumulateDamage = false;
	Ship *cShip;
	CargoManifest debrisGenerated;

	unsigned i; // MS bug
	for (i = 0;i<theFleet->shipRoster.size();i++)
	{
		cShip = theFleet->shipRoster[i];
		totalShips += cShip->quantity;
		totalEngines += cShip->numEngines;
		if (cShip->warpEngine.isScoop)
			numScoop += cShip->numEngines;
	}

	if (mineComposition[_heavyMine] !=0 )
	{
		unsigned possibleDamage = totalEngines * HEAVY_DAMAGE;
		possibleDamage += numScoop * HEAVY_KICKER;
		unsigned minDamage = HEAVY_MIN;
		if (numScoop != 0)
			minDamage += HEAVY_MIN_KICKER;
		unsigned damagePerEngine = 0;
		unsigned scoopBonusDamage = 0;
		if (possibleDamage < minDamage) //didn't do minimum fleet damage
		{
			totalDamage = minDamage;
			unsigned numDamageUnits = totalEngines * HEAVY_MIN_ENGINE_QUAN;
			numDamageUnits += numScoop * HEAVY_MIN_SCOOP_QUAN_KICKER;
			damagePerEngine = minDamage / numDamageUnits;
			scoopBonusDamage = damagePerEngine;
		}
		else
		{
			accumulateDamage = true;
			damagePerEngine = HEAVY_DAMAGE;
			scoopBonusDamage = HEAVY_KICKER;
		}
		for (unsigned i = 0;i<theFleet->shipRoster.size();i++)
		{
			cShip = theFleet->shipRoster[i];
			unsigned perShipDamage = damagePerEngine * cShip->numEngines;
			unsigned shieldDamagePerShip = cShip->shields;
			if (cShip->warpEngine.isScoop)
				perShipDamage += damagePerEngine;
			if (shieldDamagePerShip > perShipDamage/2)
				shieldDamagePerShip = perShipDamage/2;

			if (accumulateDamage)
				totalDamage += perShipDamage * cShip->quantity;
			perShipDamage -= shieldDamagePerShip;
			damageAbsorbedByShields += shieldDamagePerShip;

			unsigned fractionDamaged = perShipDamage * FRACTIONAL_DAMAGE_DENOMINATOR / cShip->armor;
			cShip->percentDamaged += fractionDamaged;
			if (fractionDamaged)
				cShip->numberDamaged = cShip->quantity;
			if (cShip->percentDamaged >= FRACTIONAL_DAMAGE_DENOMINATOR)
			{
				numDestroyed += cShip->quantity;
				debrisGenerated = debrisGenerated + cShip->baseMineralCost * cShip->quantity;
				cShip->quantity = 0;
			}
		}
		Race *ownerRace = gd.getRaceById(ownerID);
		std::string msg = "Fleet " + theFleet->getName() + " struck the "+ownerRace->singularName+" minefield # ";
		char buf[1024];
		sprintf(buf,"%lu --",objectId);
		msg += buf;
		msg += name;
		msg += ".  Total damage done to fleet = ";
		sprintf (buf,"%i DP. ",totalDamage);
		msg += buf;
		msg += "Shields absorbed ";
		sprintf (buf,"%i DP. ",damageAbsorbedByShields);
		msg += buf;
		sprintf (buf,"%i Ships were destroyed.",numDestroyed);
		msg += buf;
		theFleet->clean(gd);
		if (theFleet->shipRoster.size() == 0)
			msg += " The Fleet was completely obliterated!";
		gd.playerMessage(theFleet->ownerID,0,msg);
	}

	if (mineComposition[_standardMine] !=0 )
	{
		unsigned possibleDamage = totalEngines * STANDARD_DAMAGE;
		possibleDamage += numScoop * STANDARD_KICKER;
		unsigned minDamage = STANDARD_MIN;
		if (numScoop != 0)
			minDamage += STANDARD_MIN_KICKER;
		unsigned damagePerEngine = 0;
		unsigned scoopBonusDamage = 0;
		if (possibleDamage < minDamage) //didn't do minimum fleet damage
		{
			totalDamage = minDamage;
			unsigned numDamageUnits = totalEngines * STANDARD_MIN_ENGINE_QUAN;
			numDamageUnits += numScoop * STANDARD_MIN_SCOOP_QUAN_KICKER;
			damagePerEngine = minDamage / numDamageUnits;
			scoopBonusDamage = damagePerEngine;
		}
		else
		{
			damagePerEngine = STANDARD_DAMAGE;
			scoopBonusDamage = STANDARD_KICKER;
		}

		for (unsigned i = 0;i<theFleet->shipRoster.size();i++)
		{
			cShip = theFleet->shipRoster[i];
			unsigned perShipDamage = damagePerEngine * cShip->numEngines;
			unsigned shieldDamagePerShip = cShip->shields;
			if (cShip->warpEngine.isScoop)
				perShipDamage += damagePerEngine;
			if (shieldDamagePerShip > perShipDamage/2)
				shieldDamagePerShip = perShipDamage/2;

			perShipDamage -= shieldDamagePerShip;
			damageAbsorbedByShields += shieldDamagePerShip;

			unsigned fractionDamaged = perShipDamage * FRACTIONAL_DAMAGE_DENOMINATOR / cShip->armor;
			cShip->percentDamaged += fractionDamaged;
			if (cShip->percentDamaged >= FRACTIONAL_DAMAGE_DENOMINATOR)
			{
				numDestroyed += cShip->quantity;
				debrisGenerated = debrisGenerated + cShip->baseMineralCost * cShip->quantity;
				cShip->quantity = 0;
			}
		}

		Race *ownerRace = gd.getRaceById(ownerID);
		std::string msg = "Fleet " + theFleet->getName() + " struck the "+ownerRace->singularName+" minefield # ";
		char buf[1024];
		sprintf(buf,"%lu --",objectId);
		msg += buf;
		msg += name;
		msg += ".  Total damage done to fleet = ";
		sprintf (buf,"%i DP. ",totalDamage);
		msg += buf;
		msg += "Shields absorbed ";
		sprintf (buf,"%i DP. ",damageAbsorbedByShields);
		msg += buf;
		sprintf (buf,"%i Ships were destroyed.",numDestroyed);
		gd.playerMessage(theFleet->ownerID,0,msg);
	}

	unsigned lostInCollision = 0;
	unsigned numRemaining = 0;
	for (i = 0;i<_numberOfMineTypes;i++)
	{
		if (mineComposition[i] < 10)
		{
			lostInCollision += mineComposition[i];
			mineComposition[i] = 0;
		}
		else if (mineComposition[i]<200)
		{
			mineComposition[i] -= 10;
			lostInCollision += 10;
		}
		else if (mineComposition[i]<=1000)
		{
			unsigned temp = mineComposition[i]/20; //lose 5%
			lostInCollision += temp;
			mineComposition[i] -= temp;
		}
		else if (mineComposition[i] <= 5000)
		{
			lostInCollision += 50;
			mineComposition[i] -= 50;
		}
		else
		{
			unsigned temp = mineComposition[i]/100; //lose 1%
			lostInCollision += temp;
			mineComposition[i] -= temp;
		}
		numRemaining += mineComposition[i];
	}

	double num = (double) numRemaining;
	double r = sqrt(num);
	radius = (unsigned)ceil(r);

	Race *ownerRace = gd.getRaceById(theFleet->ownerID);
	std::string msg = ownerRace->singularName +" Fleet # ";
	char buf[1024];
	sprintf(buf,"%lu --",theFleet->objectId);
	msg += buf;
	sprintf(buf,"%lu.  ",objectId);
	msg += " struck minefield #";
	msg += buf;
	sprintf (buf,"%i Ships were destroyed.",numDestroyed);
	msg += buf;
	sprintf (buf,"  %i mines were destroyed.  ",lostInCollision);
	msg += buf;
	sprintf (buf,"Minefield radius collapses to %i L.Y.",radius);
	msg += buf;
	gd.playerMessage(ownerID,0,msg);

	if (theFleet->shipRoster.size() == 0)
	{
		debrisGenerated = debrisGenerated + theFleet->cargoList;
	}

	if (debrisGenerated.getMass() != 0)
	{
		//we generated a debris field
		debrisGenerated = debrisGenerated/3; //randomly divide by 3 for salvage
		Debris *junk = new Debris(theFleet->ownerID, theFleet->coordinates, debrisGenerated);
		gd.debrisList.push_back(junk);
	}
}

void Minefield::decay(GameData &gd)
{
    int decayRate = 2;
    char cTemp[256];
    PlayerData *ownerData = gd.getPlayerData(ownerID);

    //find enclosed planets
    Planet *cp = NULL;
    unsigned i; // MS bug
    for (i = 0;i<gd.planetList.size();i++)
    {
        cp = gd.planetList[i];
        if (withinRadius(cp->coordinates))
            decayRate+=4; //rate +4 for each planet
    }

    if (decayRate>50)
        decayRate = 50;

    if (ownerData->race->prtVector[spaceDemolition_PRT] == 1)
        decayRate /= 2;

    unsigned totalDecay = 0;
    unsigned numMines = 0;
    for (i = 0; i<_numberOfMineTypes; i++)
        if (mineComposition[i] != 0)
        {
            unsigned decayAmount = mineComposition[i] * decayRate / 100;
            mineComposition[i] -= decayAmount;
            totalDecay += decayAmount;
            numMines += mineComposition[i];
        }

    double num = (double) numMines;
    double r = sqrt(num);
    radius = (unsigned)ceil(r);

    string mess = "Minefield: " + name;
    sprintf (cTemp, " (%lu) ",objectId);
    mess += cTemp;
    mess += "decayed by";
    sprintf (cTemp, " %i mines (%i%%).", totalDecay, decayRate);
    mess += cTemp;
    mess += "  Minefield radius redeuced to ";
    sprintf (cTemp, " %i L.Y.",radius);
    mess += cTemp;
    gd.playerMessage(ownerID,0,mess);
}

double Minefield::hitProbability(unsigned speed, const Race *fleetOwner) const
{
	double baseProb = 0.0;
	unsigned safeSpeed = maxSafeSpeed(fleetOwner);
	if( speed <= safeSpeed )
		return 0.0;

	if( mineComposition[_standardMine] != 0 )
		baseProb = 0.3;
	if( mineComposition[_heavyMine] != 0 )
		baseProb = 1.0;
	if( mineComposition[_speedTrapMine] != 0 )
		baseProb = 3.5;

	return baseProb * (speed - safeSpeed);
}

unsigned Minefield::maxSafeSpeed(const Race *whichRace) const
{
	unsigned bonusSpeed = 0; // bonus safe speed for Racial Traits
	if( whichRace->prtVector[spaceDemolition_PRT] == 1 )
		bonusSpeed = 2;
	else if( whichRace->prtVector[superStealth_PRT] == 1 )
		bonusSpeed = 1;

	if( mineComposition[_standardMine] != 0 )
		return 4 + bonusSpeed;

	if( mineComposition[_heavyMine] != 0 )
		return 6 + bonusSpeed;

	if( mineComposition[_speedTrapMine] != 0 )
		return 5 + bonusSpeed;

	return 10;
}

bool Minefield::needToWorry(Location orig, Location dest, int px1, int py1,
    int pr) const
{
	const unsigned x1 = orig.getX();
	const unsigned y1 = orig.getY();
	const unsigned x2 = dest.getX();
	const unsigned y2 = dest.getY();

	int dx = x2 - x1;
	int dy = y2 - y1;
	int r  = (int)sqrt((float)(dy * dy + dx * dx)) / 2;

	const int midX = (x1 + x2) / 2;
	const int midY = (y1 + y2) / 2;

	dx = midX - px1;
	dy = midY - py1;

	int dist = (int)sqrt((float)(dy * dy + dx * dx));

	return dist < (r + pr);
}

bool Minefield::parseXML(mxml_node_t *tree)
{
    PlaceableObject::parseXML(tree);

    bool temp = true;
    temp = temp && parseXML_noFail(tree,"NONPENSCANCLOAK",nonPenScanCloakValue);
    temp = temp && parseXML_noFail(tree,"PENSCANCLOAK",penScanCloakValue);
    temp = temp && parseXML_noFail(tree,"RADIUS",radius);

    mxml_node_t * child = mxmlFindElement(tree, tree, "COMPOSITION", NULL, NULL, MXML_DESCEND);
    if( !child )
    {
        cerr << "Error, missing Composition for minefield: " << objectId << endl;
        return false;
    }
    child = child->child;

    int tempInt = assembleNodeArray(child, mineComposition, numberOf_LRT);
    if( tempInt != _numberOfMineTypes )
    {
        cerr << "Error, incomplete composition for minefield : " << objectId << endl;
        cerr << "Expecting "<<_numberOfMineTypes<<" entries, found " << tempInt << endl;
        return false;
    }
    return temp;
}

void Minefield::toXMLStringGuts(string &theString) const
{
    theString = "";

    theString += "<MINEFIELD>\n";
    string tempString = "";
    PlaceableObject::toXMLStringGuts(tempString);

    theString += tempString;
    char cTemp[2048];
    sprintf (cTemp,"<COMPOSITION>%i %i %i</COMPOSITION>\n",mineComposition[_heavyMine], mineComposition[_speedTrapMine], mineComposition[_standardMine]);
    theString += cTemp;

    sprintf (cTemp,"<NONPENSCANCLOAK>%i</NONPENSCANCLOAK>\n",nonPenScanCloakValue);
    theString += cTemp;

    sprintf (cTemp,"<PENSCANCLOAK>%i</PENSCANCLOAK>\n",penScanCloakValue);
    theString += cTemp;

    sprintf (cTemp,"<RADIUS>%i</RADIUS>\n",radius);
    theString += cTemp;

    theString += "</MINEFIELD>\n";
}

// return the distance along this vector a fleet successfully traveled, considering this minefield
// return the full distance vector if no mines were hit, or the distance traveled until a mine was struck
unsigned Minefield::whereStruckMinefield(Location pi, Location pf,
    unsigned speed, Race *fleetOwner) const
{
	unsigned distance = pi.distanceFrom(pf);
	double hitProb = hitProbability(speed, fleetOwner);
	if( hitProb == 0 )
		return distance;

	double *intercepts = vectorSphereIntercepts(pi,pf,coordinates,radius);
	if (intercepts[0] == 0)
	{
		free(intercepts); //malloced in vectorSphereIntercepts
		return distance;
	}
	unsigned distThroughField = 0;
	Location firstIntercept;
	Location secondIntercept;
	/*   firstIntercept.init((unsigned)ceil(intercepts[1]),(unsigned)ceil(intercepts[2]));
	     secondIntercept.init((unsigned)ceil(intercepts[4]),(unsigned)ceil(intercepts[5]));*/

	//vectorSphere orders from left to right
	//so we may have to swap if we are traveling
	//right to left
	firstIntercept.set(intercepts[1],intercepts[2]);
	secondIntercept.set(intercepts[4],intercepts[5]);
	if (pi.distanceFrom(firstIntercept) > pi.distanceFrom(secondIntercept))
	{
		secondIntercept.set(intercepts[1],intercepts[2]);
		firstIntercept.set(intercepts[4],intercepts[5]);
	}

	//check to see if our actual vector would pass through the entire field
	unsigned passThroughDistance = firstIntercept.distanceFrom(secondIntercept);

	if (passThroughDistance > pi.distanceFrom(secondIntercept))
	{
		firstIntercept = pi; //we are starting withing the field
		passThroughDistance = firstIntercept.distanceFrom(secondIntercept);
	}
	if (intercepts[0] == 1) //we just scrape the edge of the field
	{
		distThroughField = distanceTraveledBeforeStruck(1,hitProb);
		free(intercepts); //malloced in vectorSphereIntercepts
		if (distThroughField == 1)
			return distance; //we only had to cross one LY of the field, so if we didn't hit, we go the whole way
		//we hit the field in the 1 LY
		return (pi.distanceFrom(firstIntercept));  //total distance traveled is the distance to the first intercept point
	}

	free(intercepts); //malloced in vectorSphereIntercepts
	//our vector (extended to inf.) cuts through the circle (sphere projected into z=0 actually)

	unsigned distanceToField = pi.distanceFrom(firstIntercept);
	unsigned totalAttemptedDistance = pi.distanceFrom(pf);
	unsigned distanceAttemptedThroughField = passThroughDistance; //assume we are trying to go completely through
	if ( (distanceToField + passThroughDistance) >totalAttemptedDistance) // true if we are not trying to pass through the field entirely
		distanceAttemptedThroughField = totalAttemptedDistance - distanceToField;
	unsigned distanceSuccessful = distanceTraveledBeforeStruck(distanceAttemptedThroughField,hitProb);
	if (distanceSuccessful == distanceAttemptedThroughField)
		return distance; //did not hit a mine -- go the distance
	return (distanceToField + distanceSuccessful); //hit a mine - go to field, and then successfulDistance additional
}

bool Minefield::withinRadius(Location l) const
{
	unsigned distance = l.distanceFrom(coordinates);
	return distance <= radius;
}

