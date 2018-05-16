#ifndef PLACEABLE_H
#define PLACEABLE_H

#include "gameobj.h"
#include "cargo.h"

#include <vector>
#include <cmath>

class GameData;

// a point in space
class Location
{
protected: // data
	double xLocation;
	double yLocation;

public: // methods
	bool operator< (const Location &rhs) const;
	bool operator==(const Location &rhs) const;
	bool operator!=(const Location &rhs) const;

	void set(unsigned x, unsigned y)
	{
	   xLocation = ceil((double)x);
	   yLocation = ceil((double)y);
	}

	void set(double x, double y)
	{
	   xLocation = x;
	   yLocation = y;
	}


	void finalizeLocation(void)
	{
	   //xLocation = ceil(xLocation);
	   //yLocation = ceil(yLocation);
	}

	void moveToward(const Location &l, unsigned ly);
	double makeAngleTo(const Location &l) const;

	unsigned getX(void) const
	{
	   return (unsigned)floor(xLocation+0.5);
	}
	double getXDouble(void) const
	{
	  return xLocation;
	}
	double getYdouble(void) const
	{
	  return yLocation;
	}

	unsigned getY(void) const
	{
	   return (unsigned)floor(yLocation+0.5);
	}

	uint64_t getHash(void) const
	{
	   uint64_t hash = getX();
	   hash <<= 32;
	   hash += getY();
	   return hash;
	}

	double   actualDistanceFrom(const Location &where) const;
	unsigned distanceFrom(const Location &where) const;
};

// should be an interface
class PlaceableObject : public GameObject
{
public: // data
	Location      coordinates;
	CargoManifest cargoList;

	//scan radii here for programmer convenience.
	//can be used by gui elements and scanning resolution
	//rather than calculating at real time.
	//these values calculated and set at following times:
	//after read, prior to turn processing
	//after research phase (planets only)
	//upon fleet creation (in updateFleetVariables)
	unsigned     penScanRadius;
	unsigned     nonPenScanRadius;

	// moved here from fleet since technically planets can have orders (unload
	// to fleet) as well -- planet parsing needs to be restructured to use the
	// hierarchical parse/string conversion
	std::vector<class Order*> currentOrders;

public: // methods
	PlaceableObject(void)
	{
	   myType = gameObjectType_placeableObject;
	   penScanRadius = 0;
	   nonPenScanRadius = 0;
	}

	virtual ~PlaceableObject(void) {}

	// accessors
	unsigned getX(void) const { return coordinates.getX(); }
	unsigned getY(void) const { return coordinates.getY(); }

	const CargoManifest& getCargo(void) const { return cargoList; }
	void setCargo(const CargoManifest &c) { cargoList = c; }

	// no default implementation
	virtual bool actionHandler(GameData &gd, const class Action &what)
	{
	   return false;
	}

	// parse data for itself out of this xml branch
	bool parseXML(mxml_node_s *tree);
	void toXMLStringGuts(std::string &theString) const;

	virtual void toXMLString(std::string &theString) const
	{ toXMLStringGuts(theString); }

protected:
	bool unloadAction(GameData &gd, bool guiBased);
	bool   loadAction(GameData &gd, bool guiBased);
};

#endif

