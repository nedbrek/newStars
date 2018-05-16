#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <vector>
#include <string>

class PhysicsObject
{
public:
   PhysicsObject(void) {}
};

class Universe
{
public: // data
   static unsigned maxTechLevel;
   static unsigned techKickerPerLevel;

   unsigned      width;
   unsigned      height;
   unsigned      gameYear;
   PhysicsObject gamePhysics;

   bool          masterCopy;

   std::string   componentFileName;
   std::string   hullFileName;
   std::string   playerFileBaseName;
   unsigned      numPlayers;

   std::vector<unsigned> techCost;
	bool                  slowTech_;

protected: // methods
	unsigned randY(unsigned dy);

public: // methods
   struct GenParms
   {
      enum Uniqueness { UNONE, UX, UY };

      unsigned   width, height; // universe
      unsigned   deadX, deadY;  // border
      unsigned   rad;           // min radius between stars
      unsigned   density;       // 1..10000 star per 10000 ly^2
      Uniqueness u;             // star uniqueness
   };

	void startNew(const char *rootPath, const GenParms &gp,
	    std::vector<class Planet*> *stars);

   unsigned getTechCost(class PlayerData *whichPlayer,
       class StarsTechnology toTech);

   bool parseUniverseData(struct mxml_node_s *tree);

   void xmlPrelude (std::string &theString) const;
   void xmlPostlude(std::string &theString) const;
};

#endif

