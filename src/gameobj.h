#ifndef GAMEOBJ_H
#define GAMEOBJ_H

#include <string>
#ifndef MSVC
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
#endif

struct mxml_node_s;

enum gameObjectType
{
   gameObjectType_base = 0,
   gameObjectType_placeableObject,
   gameObjectType_ship,
   gameObjectType_fleet,
   gameObjectType_packet,
   gameObjectType_minefield,
   gameObjectType_debris,
   gameObjectType_planet
};

// base object for all NewStars objects
class GameObject
{
protected: // data
	std::string    name;

public: // data
	uint64_t       objectId;
	unsigned       ownerID;
	gameObjectType myType;

public: // methods
	GameObject(void)
	{
		name = "base game object";
		objectId = 0;
		ownerID = 0;
		myType = gameObjectType_base;
	}

	const std::string& getName(void) const { return name; }
	void               setName(const std::string &n) { name = n; }

   // returns true if theID is a valid client created
   // objectID for this player.
   static bool isValidNewRange(uint64_t theID, unsigned playerNumber);

   // parse data for itself out of this xml branch
   bool parseXML(mxml_node_s *tree);
   // convert to XML only the "guts" of the object, without the class header
   void toXMLStringGuts(std::string &theString) const;

   static uint64_t nextObjectId;
};

#endif
