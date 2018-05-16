// all the code for the battle engine
#ifdef MSVC
#pragma warning(disable: 4786)
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "newStars.h"
#include "planet.h"
#include "race.h"
#include "utils.h"
#include "battle.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <math.h>
#include <assert.h>
#include <sstream>

using namespace std;

// -----------------------------------------------------------------------
// BATTLEBOARD ENGINE
// max allowed tokens on board
static const unsigned MAX_TOKENS = 256;

// outer loop count
static const unsigned MAX_ROUNDS =  16;

// inner loop count
static const unsigned MAX_PHASES =   3;

// size of board
static const unsigned MAX_X      =   9;
static const unsigned MAX_Y      =   9;

// number of moves required to escape
static const unsigned RUNAWAY_COUNT = 7;

// maximum allowed benefit for energy caps
static const double MAX_CAPACITOR = 2.5;

// debug output
#define DEBUG_BB 0

// ---------------------------------------------------------------------------
// the unit of the battle engine, a stack of ships of the same design,
// belonging to the same player
//
// they move, fire, absorb damage, and escape together
// contains local battleboard info and a Ship Pointer
class BattleToken
{
public:
	unsigned     tokenId_;
   unsigned     xgrid;          // grid position X
   unsigned     ygrid;          // grid position Y
   unsigned     ownerID;        // Player owner
   Ship        *ship;           // Poiner to Ship object represented
   BattleToken *target;         // Pointer to current target
   unsigned     quantity;       // number of live ships
   double       bmass;          // battle mass; i.e. ship mass +/- 10%
   bool         armed;          // armed/unarmed cached
   unsigned     shields;        // current shields
   unsigned     armor;          // current armor
   double       capacitor;      // total capacitor effect, 1.0 -> 2.5
   double       reflector;      // total reflector effect, 1.0 -> 0.0
   bool         regen;          // shield regeneration
   bool         flee;           // ship attempting to runaway
   unsigned     fleeCount;      // number of squares moved while fleeing
   bool         ignore;         // ignore this battleToken: dead or fled
   uint64_t     battleOrderID;  // cache the fleet's battleOrderID
   double       compAcc;        // computer accuarcy improvement
   double       jammAcc;        // jammer accuracy degradation

public:
   BattleToken(Ship* theShip, Fleet* theFleet, GameData &gameData);
	void setTokenId(unsigned tid) { tokenId_ = tid; }

	// set board position
   void placeBattleToken(unsigned x, unsigned y);

	// can this token can be targetted for a particular battleOrder
   bool validTarget(TargetType target) const;

   // distance between 2 battle tokens
	unsigned dist(BattleToken* bt) const;

   unsigned applyBeamDamage(unsigned damage, bool sapper);
};

// ---------------------------------------------------------------------------
// use in a list to determine/maintain firing order
class WeaponToken
{
public:
   BattleToken* btoken_;
   unsigned     structureID;
   unsigned     initiative;

public:
   WeaponToken(BattleToken* bt, unsigned structID, unsigned init);

   bool findTargetInRange(const std::vector<BattleToken*> &tList,
	                       StructuralElement *element, GameData &gameData);
   double calcAttract(BattleToken* bt, StructuralElement *element, GameData &gameData);
};

// ---------------------------------------------------------------------------
// report a token to a player
class BattleReport::Token
{
protected:
	uint64_t designId_;
	uint64_t ownerId_;
	unsigned tokenId_;
	unsigned count_;
	unsigned cargoMass_;
	unsigned damagePctScale_;
	unsigned posX_;
	unsigned posY_;

public:
	Token(BattleToken *bt)
	{
		designId_       = bt->ship->designhullID;
		ownerId_        = bt->ownerID;
		tokenId_        = bt->tokenId_;
		count_          = bt->ship->quantity;
		cargoMass_      = 0;
		damagePctScale_ = bt->ship->percentDamaged;
		posX_           = bt->xgrid;
		posY_           = bt->ygrid;
	}

	void toXMLString(string &theString) const;
};

// ---------------------------------------------------------------------------
class BattleReport::Event
{
public:
	virtual ~Event(void) = 0;

	virtual void toXMLString(string &theString) const = 0;
};

BattleReport::Event::~Event(void) {}

// ---------------------------------------------------------------------------
class BattleMove : public BattleReport::Event
{
protected:
	unsigned tokenId_;
	unsigned posX_;
	unsigned posY_;

public:
	BattleMove(const BattleToken *bt)
	{
		tokenId_ = bt->tokenId_;
		posX_    = bt->xgrid;
		posY_    = bt->ygrid;
	}

	void toXMLString(string &theString) const
	{
		std::ostringstream os;
		os << "<MOVE>\n";
		os << "<TOKENID>" << tokenId_ << "</TOKENID>\n";
		os << "<POS_X>" << posX_ << "</POS_X>\n";
		os << "<POS_Y>" << posY_ << "</POS_Y>\n";
		os << "</MOVE>\n";

		theString = os.str();
	}
};

// ---------------------------------------------------------------------------
class BattleFire : public BattleReport::Event
{
protected:
	unsigned attToken_;
	unsigned tgtToken_;
	unsigned newArmor_;
	unsigned newShield_;
	unsigned newCnt_;

public:
	BattleFire(unsigned damage, WeaponToken *wt)
	{
		assert( wt->btoken_->target );
		attToken_  = wt->btoken_->tokenId_;
		tgtToken_  = wt->btoken_->target->tokenId_;
		newArmor_  = wt->btoken_->target->armor;
		newShield_ = wt->btoken_->target->shields;
		newCnt_    = wt->btoken_->target->quantity;
	}

	void toXMLString(string &theString) const
	{
		std::ostringstream os;
		os << "<FIRE TYPE=\"beam\">\n";
		os << "<TOKENID>"               << attToken_  << "</TOKENID>\n";
		os << "<TARGET>"                << tgtToken_  << "</TARGET>\n";
		os << "<ARMOR UPDATE=\"new\">"  << newArmor_  << "</ARMOR>\n";
		os << "<SHIELD UPDATE=\"new\">" << newShield_ << "</SHIELD>\n";
		os << "<COUNT UPDATE=\"new\">"  << newCnt_    << "</COUNT>\n";
		os << "</FIRE>\n";

		theString = os.str();
	}
};

// ---------------------------------------------------------------------------
BattleReport::BattleReport(void)
{
}

BattleReport::~BattleReport(void)
{
	for_each(tokens_.begin(), tokens_.end(), deleteT<Token>());
	for_each(events_.begin(), events_.end(), deleteT<Event>());
}

bool BattleReport::parseXML(mxml_node_t *tree)
{
	return true;
}

void BattleReport::toXMLString(string &theString) const
{
	if( tokens_.empty() )
	{
		theString = "<BATTLE/>\n";
		return;
	}

	std::ostringstream os;
	os << "<BATTLE>\n";

	std::string tmp;
	os << "<TOKENS>\n";
	for(std::vector<Token*>::const_iterator ti = tokens_.begin(); ti != tokens_.end(); ++ti)
	{
		(**ti).toXMLString(tmp);
		os << tmp;
	}
	os << "</TOKENS>\n";

	if( events_.size() )
	{
		os << "<EVENTS>\n";
		for(std::vector<Event*>::const_iterator ei = events_.begin(); ei != events_.end(); ++ei)
		{
			(**ei).toXMLString(tmp);
			os << tmp;
		}
		os << "</EVENTS>\n";
	}
	else
	{
		os << "<EVENTS/>\n";
	}

	os << "</BATTLE>\n";

	theString = os.str();
}

void BattleReport::addTokens(const std::vector<BattleToken*> &tList)
{
	for(std::vector<BattleToken*>::const_iterator i = tList.begin();
		i != tList.end(); ++i)
	{
		tokens_.push_back(new Token(*i));
	}
}

void BattleReport::addMove(const BattleToken *bt)
{
	events_.push_back(new BattleMove(bt));
}

void BattleReport::addFire(unsigned damage, WeaponToken *wt)
{
	events_.push_back(new BattleFire(damage, wt));
}

// ---------------------------------------------------------------------------
void BattleReport::Token::toXMLString(string &theString) const
{
	std::ostringstream os;
	os << "<TOKEN>\n";
	os << "<TOKENID>" << tokenId_ << "</TOKENID>\n";
	os << "<DESIGNID>" << designId_ << "</DESIGNID>\n";
	os << "<OWNERID>"  << ownerId_  << "</OWNERID>\n";
	os << "<COUNT>"    << count_    << "</COUNT>\n";
	os << "<CARGOMASS>" << cargoMass_ << "</CARGOMASS>\n";
	os << "<DAMAGE SCALE=\"" << FRACTIONAL_DAMAGE_DENOMINATOR << '"' << '>'
	   << damagePctScale_ << "</DAMAGE>\n";
	os << "<POS_X>" << posX_ << "</POS_X>\n";
	os << "<POS_Y>" << posY_ << "</POS_Y>\n";
	os << "</TOKEN>\n";

	theString = os.str();
}

// ---------------------------------------------------------------------------
BattleToken::BattleToken(Ship *theShip, Fleet *theFleet, GameData &gameData)
{
	// invariant
   xgrid     = 0;
   ygrid     = 0;
   target    = NULL;
   bmass     = 0.0;
   armed     = false;
   capacitor = 1.0;
   reflector = 1.0;
   regen     = false;
   flee      = false;
   fleeCount = 0;
   ignore    = false;
   compAcc   = 1.0;
   jammAcc   = 1.0;

	// pull in params
   ownerID       = theFleet->ownerID;
   ship          = theShip;
   quantity      = ship->quantity;
   battleOrderID = theFleet->battleOrderID;

	// starting armor and shields
   shields = ship->shields * quantity;

	// might need to adjust armor by damage coming in
   armor = ship->armor * quantity; // assume no damage
   if( ship->percentDamaged )
   {
      const double healthy = double(FRACTIONAL_DAMAGE_DENOMINATOR - ship->percentDamaged) /
                             double(FRACTIONAL_DAMAGE_DENOMINATOR);

		// initial armor is number damaged times health percentage (1 - damage)
      armor = (unsigned) ((ship->armor * ship->numberDamaged) * healthy);
		// add on undamaged ships
      armor += (ship->armor * (ship->quantity - ship->numberDamaged));

      // printf("ship incoming damaged: armor %d (%d X %d) : healthy %f : effective armor %d\n",
      //    quantity*ship->armor,quantity,ship->armor,healthy,armor);
   }

	// examine the ship design
   // calc capacitors/deflectors here, isArmed, etc.
   for(unsigned i = 0; i < ship->slotList.size(); ++i)
   {
      if( ship->slotList[i]->numberPopulated == 0 ) continue;

		StructuralElement *element = gameData.componentList[ship->slotList[i]->structureID];

		if( element->beam.isActive || element->missle.isActive )
			armed = true;

		if( element->energyCapacitance )
		{
			const double capfactor = (100.0 + element->energyCapacitance)/100.0;
			for(unsigned j = 0; j < ship->slotList[i]->numberPopulated; ++j)
				capacitor = capacitor * capfactor;
		}

		if( element->beamReflection )
		{
			const double reffactor = (100.0 - element->beamReflection) /100.0;
			for(unsigned j = 0; j < ship->slotList[i]->numberPopulated; ++j)
				reflector = reflector * reffactor;
			// printf("beamRef %d : ref-factor %f : total ref %f\n",element->beamReflection,reffactor,reflector);
		}

		if( element->accuracyImprovement )
		{
			for(unsigned j = 0; j < ship->slotList[i]->numberPopulated; ++j)
				compAcc = compAcc * (1.0-(((double) (element->accuracyImprovement)) / 100.0));
		}

		if( element->jam )
		{
			for(unsigned j = 0; j < ship->slotList[i]->numberPopulated; ++j)
				jammAcc = jammAcc * (1.0-(((double) (element->jam)) / 100.0));
		}
   }

   if( capacitor > MAX_CAPACITOR )
      capacitor = MAX_CAPACITOR;

   compAcc = 1.0 - compAcc;

   //Ned? Jammer code maybe should be in ship class somewhere?
   jammAcc = 1.0 - jammAcc;
   if( jammAcc > 0.95 )
      jammAcc = 0.95;
   if( ship->isSpaceStation )
      jammAcc = jammAcc * 0.75;

   // printf("Computer Accuarcy is %6.4f : Jammer Degradation is %6.4f \n",compAcc,jammAcc);

   // check to see if this player has shield regen
   PlayerData *pData = gameData.getPlayerData(ownerID);
   regen = pData->race->lrtVector[regeneratingShields_LRT] != 0;

   // basic check for fleeing ...
   //Ned BUG: ignoring battle order based fleeing for now
   if( !armed )
      flee = true;
}

// set xgrid, ygrid
void BattleToken::placeBattleToken(unsigned x, unsigned y)
{
   xgrid = x;
   ygrid = y;
}

// Return: distance between 2 battle tokens
// note: distance in Stars! battleboard allows diagonal
unsigned BattleToken::dist(BattleToken *bt) const
{
   unsigned xdist;
   if( xgrid > bt->xgrid ) xdist = xgrid - bt->xgrid;
   else                    xdist = bt->xgrid - xgrid;

	unsigned ydist;
   if( ygrid > bt->ygrid ) ydist = ygrid - bt->ygrid;
   else                    ydist = bt->ygrid - ygrid;

   if( xdist > ydist )
      return xdist;

	return ydist;
}

// can this token can be targetted for a particular battleOrder
// Return: true if valid target
bool BattleToken::validTarget(TargetType target) const
{
	switch( target )
	{
	case             _anyTarget: return true;
	case        _starbaseTarget: return ship->isSpaceStation;
	case      _armedShipsTarget: return  armed;
	case    _unarmedShipsTarget: return !armed;
	case   _fuelTransportTarget: return ship->hullClass == fuelTransport_class;
	case   _freighterOnlyTarget: return ship->hullClass ==     freighter_class;
	case _bomberFreighterTarget:
      return ship->hullClass == freighter_class ||
             ship->hullClass == bomber_class;

	case _noTarget: return false;
	case _numTargetTypes:
		assert(false);
	}

	assert(false);
   return false;
}

// apply damage to kill
// Return: unused damage
unsigned BattleToken::applyBeamDamage(unsigned damage, bool sapper)
{
	// scale damage down for reflectors
   if( reflector < 1.0 )
   {
      if( DEBUG_BB ) printf("  - reflector reduces damage %d X %6.4f ", damage, reflector);

      damage = unsigned(damage * reflector);

      if( DEBUG_BB ) printf(" = %d \n",damage);
   }

	// beam damage is first applied to shields
   if( shields > 0 )
   {
      if( damage <= shields )
      {
			// shields hold
         if( DEBUG_BB ) printf("  - shields at %d take %d damage \n", shields, damage);

         shields -= damage;
         return 0;
      }

		// shields gone
		damage -= shields;
		shields = 0;
		if( DEBUG_BB ) printf("  - the shields canna take it! \n");
   }

	// sappers do not damage armor
   if( damage > 0 && !sapper )
   {
      if( damage <= armor )
      {
			// all damage absorbed by the stack, check casualties

			// kill ships equal to the amount of damage divided by the armor of
			// any single ship
         const unsigned kills = unsigned(double(damage) /
                                         (double(armor) /
                                          double(quantity)));

         if( DEBUG_BB ) printf("  - damage %d : armor %d : targ-count %d : kills %d \n",
                               damage, armor, quantity, kills);

			// shields should be zero at this point...
         // NOTE: the below shield recomputation may be unsafe
         shields = (shields * kills) / quantity;

         armor    -= damage;
         quantity -= kills;
         return 0;
      }

		// boom! total token kill
		if( DEBUG_BB ) printf("  - target blows up good! : killed all %d\n", quantity);

		damage  -= armor;
		armor    = 0;
		quantity = 0;
		ignore   = true; // ignore this token for the rest of the battle
   }

	// scale damage back from reflector
   if( reflector < 1.0 && damage > 0 )
      damage = unsigned(double(damage) / reflector);

   return damage;
}

// ---------------------------------------------------------------------------
WeaponToken::WeaponToken(BattleToken* bt, unsigned structID,unsigned init)
{
   btoken_     = bt;
   structureID = structID;
   initiative  = init;
}

// -----------------------------------------------------------------------
// walk thru list of tokens and choose targets in range of this weapon
// Return: true if anyone found a valid target
//
// NOTE: related to retargetBattleTokens.
//       probably should re-org code later to unify these two
//
//Ned BUG: missing real attraction code
//Ned BUG: missing target filters (using battle orders, enemies, etc)
bool WeaponToken::findTargetInRange(const std::vector<BattleToken*> &tList,
                                    StructuralElement *element,
                                    GameData &gameData)
{
	btoken_->target = NULL;

	BattleOrder* bOrder = gameData.getBattleOrderByID(btoken_->battleOrderID);

	// space stations have extra range
	const unsigned rngBonus = btoken_->ship->isSpaceStation ? 1 : 0;

	// live to end
	bool primary_match   = false;
	bool secondary_match = false;

	double attract = 0.0; // highest attractiveness so far
	for(unsigned i = 0; i < tList.size(); ++i)
	{
		// don't fire on your own
		if( btoken_->ownerID == tList[i]->ownerID ) continue;

		// don't target blown up tokens
		if( tList[i]->ignore ) continue;

		bool primary   = tList[i]->validTarget(bOrder->  primaryTarget);
		bool secondary = tList[i]->validTarget(bOrder->secondaryTarget);

		//Ned, this calls stub attraction code
		double test_attract = calcAttract(tList[i], element, gameData);

		//Ned? below code should be in above attraction code?
		if( element->beam.isActive && element->beam.isShieldSapper &&
		    tList[i]->shields == 0 )
		{
			test_attract = 0.0;
			primary      = false;
			secondary    = false;
		}

		// if not a candidate
		if( !primary && (!secondary || primary_match) ) continue;

		// find distance to target
		unsigned dist = btoken_->dist(tList[i]);
		// apply range bonus
		if( dist > rngBonus )
			dist -= rngBonus;
		else
			dist = 0;

		bool inRange = false;

		// if a beam weapon and in range
		if( element->beam.isActive &&
		    element->beam.range >= dist )
		{
			inRange = true;
      }
		// if missle weapon and in range
		if( element->missle.isActive &&
		    element->missle.range >= dist )
		{
			inRange = true;
		}

		// see if this target is more attractive than current
		if( inRange && test_attract > attract )
		{
			btoken_->target = tList[i];
			attract = test_attract;
			if (primary)
				primary_match = true;
			else
				secondary_match = true;
		}
	}

	return primary_match || secondary_match;
}

// calculate battle attractiveness between 1 weapon and 1 stack
// Return: attraction value
double WeaponToken::calcAttract(BattleToken* bt, StructuralElement *element, GameData &gameData )
{
	//Ned TBD: fill in real code
	return 1.0;
}

// ---------------------------------------------------------------------------
// find largest number of tokens that any one player could have in this battle
//
// if sum of potential tokens is less than MAX_TOKENS, returns MAX_TOKENS
static unsigned maxBattleTokens(const std::vector<Fleet*>   &fList,
                                const std::vector<unsigned> &playerList)
{
	// parse down for allowable stacks
	unsigned fair_tokens = MAX_TOKENS / playerList.size();

	// First: get the basic counts on the first pass
	// Also: figure out basic unused portion
	unsigned returncount = 0;
	unsigned reqcount = 0;
	bool recalc = false;

	unsigned total_tokens = 0; // total of all tokens from all players

	// build a map from player id to number of tokens for that player
	std::map<unsigned, unsigned> tokenMap;
	//foreach player
	// 'returncount' gets excess tokens from players below fair
	// 'reqcount'    is number of players with too many tokens
	for(unsigned i = 0; i < playerList.size(); ++i)
	{
		// get number of tokens for this player
		unsigned tokencount = 0;

		//foreach fleet in battle
		for(unsigned j = 0; j < fList.size(); ++j)
		{
			// if player id match, add tokens
			if( fList[j]->ownerID == playerList[i] )
				tokencount += fList[j]->shipRoster.size();
		}

		// save the token count for this player id
		tokenMap[ playerList[i] ] = tokencount;

		total_tokens += tokencount;
		if( tokencount <= fair_tokens )
			returncount += (fair_tokens - tokencount);
		else
		{
			++reqcount;
			recalc = true;
		}
		// printf("player %2d : token count = %d\n",playerList[i], tokencount);
	}

	// if there are very few tokens, or all players want the fair number
	if( total_tokens <= MAX_TOKENS || returncount == 0 ) return MAX_TOKENS;

	// now, iterate if needed
	unsigned iterCt = 0;
	while( recalc )
	{
		// for safety, check max-iteration
		++iterCt;
		assert( iterCt < 256 );

		// try again with more tokens per player
		fair_tokens += returncount / reqcount;
		returncount = 0;
		reqcount    = 0;

		for(unsigned i = 0; i < playerList.size(); ++i)
		{
			if (tokenMap[playerList[i]] <= fair_tokens)
				returncount += tokenMap[playerList[i]];
			else
			{
				returncount += fair_tokens;
				++reqcount;
			}
		}
		returncount = returncount - MAX_TOKENS;
		if (returncount < reqcount) recalc = false;
	}

	return fair_tokens;
}

// parse the list of fleets into a list of battle tokens
// no player gets more tokens than fair_tokens
//
//Ned BUG : current code doesn't gurantee that SB is granted a slot!
//
static std::vector<BattleToken*> buildBattleTokenList(
                             const std::vector<Fleet*>   &fList,
                             const std::vector<unsigned> &playerList,
                             unsigned                     fair_tokens,
                             GameData                    &gameData)
{
	unsigned tokenId = 1;
	std::vector<BattleToken*> tList;

	for(unsigned i = 0; i < playerList.size(); ++i)
	{
		unsigned player_tokens = 0;
		for(unsigned j = 0; j < fList.size(); ++j)
		{
			if( fList[j]->ownerID == playerList[i] )
			{
				// cargo dump check
				// note: assumes if any ships of the fleet on battle board, all cargo dumped
				BattleOrder* bOrder = gameData.getBattleOrderByID(fList[j]->battleOrderID);
				if( bOrder->dumpCargo )
					fList[j]->cargoList.clear();

				for(unsigned k = 0; k < fList[j]->shipRoster.size(); ++k)
				{
					BattleToken *bT = new BattleToken(fList[j]->shipRoster[k], fList[j], gameData);
					bT->setTokenId(tokenId);
					++tokenId;
					tList.push_back(bT);

					++player_tokens;
					if( player_tokens >= fair_tokens ) break;
				}
			}
			if( player_tokens >= fair_tokens ) break;
		}
	}

	if( tList.size() > MAX_TOKENS )
	{
		printf("Battle Board bug with token max: %lu\n", tList.size());
		exit(1);
	}

	// printf("tList size= %d\n", tList.size());
	return tList;
}

// place all tokens correctly on battleboard
//
//Ned BUG : current code doesn't work for >2 players
void placeBattleTokens( std::vector<BattleToken*> tList,
                        std::vector<unsigned> playerList)
{
	unsigned xgrid = 0;
	unsigned ygrid = 0;

	for(unsigned i = 0; i < playerList.size(); ++i)
	{
		// may need to build a function for this if too complex ...
		// (especially if position dependent on list size ...)
		switch( i )
		{
		case  0: xgrid = 1; ygrid = 5; break;
		case  1: xgrid = 8; ygrid = 4; break;
		default: xgrid = 0; ygrid = 0;
		}

		for(unsigned j = 0; j < tList.size(); ++j)
		{
			if( tList[j]->ownerID == playerList[i] )
			{
				tList[j]->placeBattleToken(xgrid, ygrid);
			}
		}
	}
}

// walk thru list and assign battle mass (random +/- 10% of ship Mass)
// (simulates ships performing better/worse in combat than expected)
void assignBattleMass( std::vector<BattleToken*> tList)
{
	for(unsigned i = 0; i < tList.size(); ++i)
	{
		// get a number between 0.9000 and 1.1000
		const double factor = 1 + double(-1000 + (rand() % 2001)) / 10000;

		// scale the mass
		tList[i]->bmass = factor * tList[i]->ship->mass;
		// printf("Token %d : mass %d : factor %5.4f : battle-mass %f\n",
		//                i,tList[i]->ship->mass,factor,tList[i]->bmass);
	}
}

// Return: list sorted by bmass, heaviest first
std::vector<BattleToken*> sortBattleTokensByMass(std::vector<BattleToken*> tList)
{
	// bubble sort!
	// (but at least we only do it once per battle)
	for(unsigned i = 0; i < tList.size()-1; ++i)
	{
		unsigned index = i;
		for(unsigned j = i+1; j < tList.size(); ++j)
		{
			if( tList[j]->bmass > tList[index]->bmass )
				index = j;
		}

		// swap
		BattleToken* replace = tList[i];
		tList[i]             = tList[index];
		tList[index]         = replace;
	}

	return tList;
}

// Return: firing order list
//
// - each unique type of weapon on each battleToken gets a position on list
// - need to order list in terms of initiative
// - need to correctly randomize order for ties
static std::vector<WeaponToken*> buildFireList(std::vector<BattleToken*> tList,
                                               GameData &gameData)
{
	int i; // MS bug

	// first, build the basic list
	std::vector<WeaponToken*> fireList; // return value
	//foreach token
	for(i = 0; i < (int)tList.size(); ++i)
	{
		//foreach slot
		for(unsigned j = 0; j < tList[i]->ship->slotList.size(); ++j)
		{
			// if slot is empty
			if( tList[i]->ship->slotList[j]->numberPopulated <= 0 ) continue;

			StructuralElement *element = gameData.componentList[ tList[i]->ship->slotList[j]->structureID ];
			// if not a weapon (beam or missle)
			if( !element->beam.isActive && !element->missle.isActive ) continue;

			// crosscheck for unique weapons type
			bool match = false;
			for(unsigned k = 0; k < j; ++k)
			{
				if( tList[i]->ship->slotList[k]->numberPopulated > 0 )
					if( tList[i]->ship->slotList[j]->structureID ==
					    tList[i]->ship->slotList[k]->structureID )
						match = true;
			}

			// if unique, add it
			if( !match )
			{
				unsigned init = tList[i]->ship->initiative + element->baseInitiative;
				WeaponToken *wT = new WeaponToken(tList[i], tList[i]->ship->slotList[j]->structureID, init);
				fireList.push_back(wT);

				// printf("weapons list : adding ship %d : weapon %d : init %d (%d + %d) \n",
				//    i, tList[i]->ship->slotList[j]->structureID,init,
				//    tList[i]->ship->initiative,element->baseInitiative);

				//  if (element->beam.isActive)
				//  {
				//     printf("ship %d : wweapon %d : range %d : damageFactors size %d ",
				//        i, tList[i]->ship->slotList[j]->structureID,
				//        element->beam.range, element->beam.damageFactors.size());
				//     for (unsigned z = 0; z < element->beam.damageFactors.size(); z++)
				//       printf(": [%d] %d ",z,element->beam.damageFactors[z]);
				//     printf("\n");
				//  }
			}
		}
	}

	// now, sort list for initiative order

	// bubble sort!
	// (but at least we only do it once per battle)
	for(i = 0; i < int(fireList.size())-1; ++i)
	{
		unsigned index = i;
		for(unsigned j = i+1; j < fireList.size(); ++j)
		{
			unsigned equal = 1;
			// the greater init case is easy
			if( fireList[j]->initiative > fireList[index]->initiative )
			{
				index = j;
				equal = 1;
			}
			// handle equal init case
			else if (fireList[j]->initiative == fireList[index]->initiative)
			{
				++equal;
				// find a number between 0 and equal-1
				unsigned roll = rand() % equal;
				// basically, 1 in equal chance to pick new value
				// ie 2nd: 1/2; 3rd: 1/3; 4th: 1/4
				if( roll == 0 )
					index = j;
			}
		}

		// swap (may be swap with self)
		WeaponToken* replace = fireList[i];
		fireList[i]          = fireList[index];
		fireList[index]      = replace;
	}

	return fireList;
}

// walk thru list and choose targets
// Return: found a valid target
//
//Ned BUG: need real attractiveness code (including sapper handling)
static bool retargetBattleTokens(const std::vector<BattleToken*> &tList,
                                 GameData &gameData)
{
	bool targetsfound = false;

	for(unsigned i = 0; i < tList.size(); ++i)
	{
		// ignore dead tokens
		if( tList[i]->ignore ) continue;

		tList[i]->target = NULL; // start over on targets

		BattleOrder* bOrder = gameData.getBattleOrderByID(tList[i]->battleOrderID);
		bool primary = false;
		bool secondary = false;
		bool primary_match = false;
		double attract = 0.0;

		for(unsigned j = 0; j < tList.size(); ++j)
		{
			// don't target your own
			if( tList[i]->ownerID == tList[j]->ownerID ) continue;

			// don't target dead tokens
			if( tList[j]->ignore ) continue;

			primary   = tList[j]->validTarget(bOrder->primaryTarget);
			secondary = tList[j]->validTarget(bOrder->secondaryTarget);
			//Ned TBD - real attractiveness
			double test_attract = 1.0;

			if( primary || (secondary && !primary_match) )
			{
				if (test_attract > attract)
				{
					tList[i]->target = tList[j];
					attract = test_attract;
					targetsfound = true;
					if (primary)
						primary_match = true;
				}
			}
		}
	}

	return targetsfound;
}

// Return: true if this ship can move this phase
//
// Note: move assumed to be in set (0.5, 0.75, 1.00, ... 2.5)
//
// Ned BUG: warmonger gets move bonus??(here or somewhere else?)
static bool canMove(double move, unsigned round, unsigned phase )
{
   // local table uses round from 0 to 7
   while( round > 7 )
      round -= 8;

   bool canmove = false;

   if( phase == 0 )
   {
      if (move == 2.5)
         if ((round == 0) || (round == 2) ||
             (round == 4) || (round == 6) )
            canmove = true;
      if (move == 2.25)
         if ((round == 0) || (round == 4))
            canmove = true;
   }
   else if( phase == 1 )
   {
      if (move >= 2.0)
         canmove = true;

      if (move == 1.75)
         if ((round==0) || (round==1) ||
             (round==3) || (round==4) ||
             (round==5) || (round==7) )
            canmove = true;
      if (move == 1.5)
         if ((round == 0) || (round == 2) ||
             (round == 4) || (round == 6) )
            canmove = true;
      if (move == 1.25)
         if ((round == 0) || (round == 4))
            canmove = true;

   }
   else if( phase == 2 )
   {
      if (move >= 1.0)
         canmove = true;

      if (move == 0.75)
         if ((round==0) || (round==1) ||
             (round==3) || (round==4) ||
             (round==5) || (round==7) )
            canmove = true;
      if (move == 0.5)
         if ((round == 0) || (round == 2) ||
             (round == 4) || (round == 6) )
            canmove = true;

   }

   return canmove;
}

// move each ship toward target
// Ned bugs
// BUG: movement just plain not working like Stars!
//
// BUG: doesn't handle fleeing ship issues correctly
// BUG: doesn't handle battle tactic issues correctly
// BUG: doesn't handle missile range issues correctly
// BUG: movement to current square should not be allowed - must move
// BUG: not handling energy dampners currently
// BUG: not handling war monger bonus moves currently
//
static void moveBattleTokens(const std::vector<BattleToken*> &tList,
   unsigned round, unsigned phase, BattleReport *brp)
{
	for(unsigned i = 0; i < tList.size(); ++i)
	{
		// printf("token %d : move %f \n", i,tList[i]->ship->combatMove);

		// can we move during the current round/phase
		const bool canmove = canMove(tList[i]->ship->combatMove, round, phase);

		// skip if dead, no target, or can't move
		if( tList[i]->ignore || !tList[i]->target || !canmove ) continue;

		if( !tList[i]->flee )
		{
			// agressor pattern

			// can move 1 in both X and in Y (ie diagonal)
			if( tList[i]->xgrid > tList[i]->target->xgrid )
				--(tList[i]->xgrid);
			else if( tList[i]->xgrid < tList[i]->target->xgrid )
				++(tList[i]->xgrid);

			if( tList[i]->ygrid > tList[i]->target->ygrid )
				--(tList[i]->ygrid);
			else if( tList[i]->ygrid < tList[i]->target->ygrid )
				++(tList[i]->ygrid);
		}
		else // flee code
		{
			// currently, only simple flee from target

			// flee in X
			if( (tList[i]->xgrid > tList[i]->target->xgrid) &&
				(tList[i]->xgrid < MAX_X))
				tList[i]->xgrid++;
			else if ((tList[i]->xgrid < tList[i]->target->xgrid) &&
				(tList[i]->xgrid > 0    ))
				tList[i]->xgrid--;
			else if (tList[i]->xgrid == tList[i]->target->xgrid)
			{
				if (tList[i]->xgrid == 0)
					tList[i]->xgrid++;
				else if (tList[i]->xgrid == MAX_X)
					tList[i]->xgrid--;
				else
				{
					// flip a coin ...
					unsigned roll = rand() % 2;
					if (roll == 0)
						tList[i]->xgrid++;
					else
						tList[i]->xgrid--;
				}
			}

			// flee in Y
			if ((tList[i]->ygrid > tList[i]->target->ygrid) &&
				(tList[i]->ygrid < MAX_Y))
				tList[i]->ygrid++;
			else if ((tList[i]->ygrid < tList[i]->target->ygrid) &&
				(tList[i]->ygrid > 0    ))
				tList[i]->ygrid--;
			else if (tList[i]->ygrid == tList[i]->target->ygrid)
			{
				if (tList[i]->ygrid == 0)
					tList[i]->ygrid++;
				else if (tList[i]->ygrid == MAX_Y)
					tList[i]->ygrid--;
				else
				{
					// flip a coin ...
					unsigned roll = rand() % 2;
					if (roll == 0)
						tList[i]->ygrid++;
					else
						tList[i]->ygrid--;
				}
			}

			// bump the flee counter for chance to escape
			tList[i]->fleeCount++;
		}

		// update the battle report
		brp->addMove(tList[i]);

		// basic error checking
		if( (tList[i]->xgrid > MAX_X) ||
			 (tList[i]->ygrid > MAX_Y) )
		{
			printf("There seems to be a bug in moveBattleTokens\n");
			exit(1);
		}

		if (DEBUG_BB)
			printf("token %d (P-ID %d : O-ID %lu): moved to %d, %d \n",
			i,tList[i]->ownerID,tList[i]->ship->objectId,tList[i]->xgrid,tList[i]->ygrid);
	}
}

// dead ships leave behind salvage
//Ned bugs
// BUG: need to investigate salvage packet clumping/ownership in Stars
// BUG: salvage over planet should drop to planet
// BUG: salvage increase for dead freighters carrying cargo
//      problem: cargo carried by fleet ... dang. Complicated.
static void createBattleSalvage(const std::vector<BattleToken*> &tList,
   const std::vector<unsigned> &playerList, const Location &coordinates,
   GameData &gameData)
{
	CargoManifest debris; // reduce construction overhead

	// foreach player in the battle
	for(size_t i = 0; i < playerList.size(); ++i)
	{
		// init debris for each player
		debris.clear();

		// foreach token in battle
		for(size_t j = 0; j < tList.size(); ++j)
		{
			// if current player owns this token
			if( playerList[i] == tList[j]->ownerID )
			{
				// calculate how many ships were lost from this token
				const unsigned numDead = tList[j]->ship->quantity -
				    tList[j]->quantity;

				// if any lost, calculate minerals involved
				if( numDead )
					debris = debris + tList[j]->ship->baseMineralCost * numDead;
			}
		}

		if( debris.getMass() != 0 )
		{
			// debris is 1/3 of total minerals involved
			debris = debris / 3;
			// printf("generating debris: player-id %d : mass=%d \n", playerList[i], debris.getMass() );

			// create a salvage packet with the player id, location, and minerals
			Debris *salvage = new Debris(playerList[i], coordinates, debris);

			// add it to the game list of objects
			gameData.debrisList.push_back(salvage);
		}
	}
}

// propagate battleToken armor damage back to Ship object
static void assignDamageToShip(const std::vector<BattleToken*> &tList)
{
   for(unsigned i = 0; i < tList.size(); ++i)
   {
		// first quantity
      tList[i]->ship->quantity = tList[i]->quantity;

		// if all destroyed, done
      if( !tList[i]->quantity ) continue;

		// amount of armor left, per ship
		const double ratio = double(tList[i]->armor) /
		                     double(tList[i]->quantity * tList[i]->ship->armor);

		// convert to unsigned quantity
		unsigned numerator = unsigned(ratio * FRACTIONAL_DAMAGE_DENOMINATOR);

		// convert from "% alive" to "% dead"
		numerator = FRACTIONAL_DAMAGE_DENOMINATOR - numerator;

		// printf("stack %d : raw: armor-remaining %d : quantity %d : armor-start %d : ratio %16.12f : numerator %d\n",
		//          i,tList[i]->armor,tList[i]->quantity,tList[i]->ship->armor, ratio, numerator);

		// save out the calc
		tList[i]->ship->percentDamaged = numerator;

		// set num damaged
		if( numerator )
			tList[i]->ship->numberDamaged = tList[i]->quantity;
   }
}

// fire all weapons
//
// TBD: checks on how missiles really work
// o assuming shield damage continously applied (torp by torp)
//   -- effects when unshielded ship calculations begin; example for cap missiles
// o assuming armor damage not continously applied to kills (torp by torp)
//   -- effects stack shields when shields taken down by kills (when shields > armor)
// o assuming that continous armor tracking works by stack, not by ship
//   -- i.e. assumes if damage under ship kill, that it is safe to sum it
//   -- this has the effect that 1 torp can sometimes spread damage to 2 ships
//      - scenario: 2 ship of 100 armor = 200 total.
//                : torp damage = 70.
//                : 3 hits can kills 2 ships (3*70 = 210) (my assumption) OR
//                  only 1 ship killed + 1 ship damaged to 70%  (clip 3rd hit on kill)
// o assuming kills take down their share of shields
//   -- effects stack shields
// o assuming that 1 torp hit can spill over shield->armor
//   -- effects how fast armor damage adds up
static void fireWeaponTokens(const std::vector<WeaponToken*> &fireList,
   const std::vector<BattleToken*> &tList, GameData &gameData, BattleReport *brp)
{
	//foreach weapon token (unique list, sorted by init)
	for(unsigned i = 0; i < fireList.size(); ++i)
	{
		// dead ships don't fire
		if( fireList[i]->btoken_->ignore ) continue;

		StructuralElement *element = gameData.componentList[ fireList[i]->structureID ];

		//foreach slot
		for(unsigned j = 0; j < fireList[i]->btoken_->ship->slotList.size(); ++j)
		{
			// if this weapon is not firing now
			if( fireList[i]->btoken_->ship->slotList[j]->structureID != fireList[i]->structureID )
				continue;

			unsigned popcount = fireList[i]->btoken_->ship->slotList[j]->numberPopulated;

			// printf("searching for target: fireList %d : slot %d \n",i,j);
			fireList[i]->findTargetInRange(tList, element, gameData);

			// if no target, done
			if( !fireList[i]->btoken_->target ) continue;

			// handle a beam weapon
			if( element->beam.isActive )
			{
				// Note: need to functionalize this

				// damage is range 0 * slot count * ship count
				unsigned damage = element->beam.damageFactors[0] * popcount *
                              fireList[i]->btoken_->quantity;
				// scaled for energy cap
				if( fireList[i]->btoken_->capacitor > 1.0 )
					damage = unsigned(double(damage) * fireList[i]->btoken_->capacitor);

				BattleToken *target;
				double rfactor;
				unsigned dist;
				if( element->beam.isGatlingCapable )
				{
					// handle gatling cannons
					BattleOrder* bOrder = gameData.getBattleOrderByID(fireList[i]->btoken_->battleOrderID);
					unsigned bonus = 0;
					if( fireList[i]->btoken_->ship->isSpaceStation )
						bonus = 1;

					if( DEBUG_BB )
						printf("o Gattling (token %d slot %d oid %lu) fires in all directions!\n",i,j, fireList[i]->btoken_->ship->objectId);

					unsigned rDamage;
					for(unsigned k = 0; k < tList.size(); ++k)
					{
						// don't fire on our own
						if( fireList[i]->btoken_->ownerID == tList[k]->ownerID ) continue;

						// don't fire on dead tokens
						if( tList[k]->ignore ) continue;

						// don't fire a sapper at ships with no shields
						if( element->beam.isShieldSapper && tList[k]->shields == 0 ) continue;

						bool primary   = tList[k]->validTarget(bOrder->primaryTarget);
						bool secondary = tList[k]->validTarget(bOrder->secondaryTarget);
						if( !primary && !secondary ) continue;

						fireList[i]->btoken_->target = tList[k];
						dist = fireList[i]->btoken_->dist(fireList[i]->btoken_->target);
						// check for out of range
						if( (element->beam.range+bonus) < dist ) continue;

						// apply range dissipation - scaled up to 10% at max range
						if( dist > 0 )
						{
							rfactor = ((double) dist)/element->beam.range;
							if (fireList[i]->btoken_->ship->isSpaceStation)
								rfactor = ((double) dist)/(1+element->beam.range);
							rfactor = 1.0 - rfactor/10.0;
							rDamage = (unsigned) ( (double) damage * rfactor );
						}
						else
						{
							rfactor = 1.0;
							rDamage = damage;
						}

						if( DEBUG_BB )
							printf("o weapon token %d slot %d (oid=%lu) fires at range %d for %d X %d X %d X %6.4f X %6.4f = %d damage\n",
							       i, j, fireList[i]->btoken_->ship->objectId, dist, element->beam.damageFactors[0], popcount, fireList[i]->btoken_->quantity,
							       fireList[i]->btoken_->capacitor,rfactor,rDamage);

						fireList[i]->btoken_->target->applyBeamDamage(rDamage,element->beam.isShieldSapper);
						brp->addFire(rDamage, fireList[i]);
					}
					fireList[i]->btoken_->target = NULL;
				} // end gatling fire (skips normal fire)

				// beam weapons stream from token to token
				while( fireList[i]->btoken_->target )
				{
					dist = fireList[i]->btoken_->dist(fireList[i]->btoken_->target);

					// apply range dissipation - scaled up to 10% at max range
					if( dist > 0 )
					{
						rfactor = ((double) dist)/element->beam.range;
						if (fireList[i]->btoken_->ship->isSpaceStation)
							rfactor = ((double) dist)/(1+element->beam.range);
						rfactor = 1.0 - rfactor/10.0;
						damage = (unsigned) ( (double) damage * rfactor );
					}
					else
						rfactor = 1.0;

					if( DEBUG_BB )
						printf("o weapon token %d slot %d (oid %lu) fires at range %d for %d X %d X %d X %6.4f X %6.4f = %d damage\n",
                               i,j,fireList[i]->btoken_->ship->objectId,
                               dist, element->beam.damageFactors[0],popcount,
                               fireList[i]->btoken_->quantity,
                               fireList[i]->btoken_->capacitor,rfactor,damage);

					// apply damage
					target = fireList[i]->btoken_->target;
					damage = target->applyBeamDamage(damage,element->beam.isShieldSapper);
					brp->addFire(damage, fireList[i]);

					// clean up
					fireList[i]->btoken_->target = NULL;
					target = NULL;

					// check if we can look for a new target ...
					// at this point, if damage>0, should attempt to retarget
					if( damage > 0 )
					{
						// printf("  - leftover damage = %d : should retarget \n",damage);

						// first, pull out range dissipation factor
						if (rfactor < 1.0)
							damage = (unsigned) ( (double) damage / rfactor );

						// attempt to target
						fireList[i]->findTargetInRange( tList, element, gameData );
					}
				}
			}

			// handle missle slot
			if( element->missle.isActive )
			{
				if( DEBUG_BB )
					printf("o weapon token %d slot %d (%lu) : count %d \n",
                                    i,j,fireList[i]->btoken_->ship->objectId,
                                    popcount*fireList[i]->btoken_->quantity );

				// o for loop for damage (popcount*quantity)
				// o need to compute missile hit probability
				// o damage applied shield immediately, accumulate hull
				//   - under assumption about when Cap missiles start 2X
				//   - also, do ship kills immediately undercut remaining shields?
				// o basic: half armor / half shields (spill to armor if no shields)
				// o miss can damage shields at 1/8, no spillover
				// o Capital Missle? if no shields, 2X damage to armor
				// o at most, 1 kill per missile
				// o when to retarget? must be checking if current stack dead ...

				double tohit = ( (double) element->missle.accuracy) / 100.0;
				if( fireList[i]->btoken_->target->jammAcc > fireList[i]->btoken_->compAcc )
				{
					tohit = tohit *
					        (1.0- (fireList[i]->btoken_->target->jammAcc -
                                 fireList[i]->btoken_->compAcc));
				}
				else
				{
					tohit = tohit + ((1.0-tohit) *
                                  (fireList[i]->btoken_->compAcc -
                                   fireList[i]->btoken_->target->jammAcc));
				}

				unsigned hitNum = unsigned(100000 * tohit);
				if( DEBUG_BB )
				{
					printf("   - tohit = %6.4f : hitNum = %d \n", tohit, hitNum);
					printf("   - target (oid=%lu) : quantity %d : shields %d : armor %d\n",
                      fireList[i]->btoken_->target->ship->objectId,
                      fireList[i]->btoken_->target->quantity,
                      fireList[i]->btoken_->target->shields,
                      fireList[i]->btoken_->target->armor);
				}

				unsigned damage;
				unsigned running_damage = 0;

				unsigned targ_armor_per = fireList[i]->btoken_->target->armor/
                                      fireList[i]->btoken_->target->quantity;

				unsigned s_damage;

				for(unsigned k = 0; k < (popcount * fireList[i]->btoken_->quantity); ++k)
				{
					// firing ship may have stopped targetting
					if( !fireList[i]->btoken_->target ) break;

					// lets see if we have a hit
					unsigned roll = rand() % 100000;
					if( roll < hitNum )
					{
						damage = element->missle.damage;

						// Apply shield damage and recompute for Hull (including Cap)
						if( fireList[i]->btoken_->target->shields )
						{
							// missles do half to shields
							s_damage = damage / 2;
							if( s_damage < fireList[i]->btoken_->target->shields )
							{
								// shields will hold
								damage -= s_damage; // half damage to armor
								fireList[i]->btoken_->target->shields -= s_damage;
							}
							else
							{
								// shields fail, remaining damage to armor
								damage -= fireList[i]->btoken_->target->shields;
								fireList[i]->btoken_->target->shields = 0;
								if (DEBUG_BB)
									printf("   - torp %4d finishes off the shields\n",k);
							}
						}
						else // no shields
						{
							if( element->missle.isCapitalShipMissle )
								damage = 2 * damage;
						}

						// if killing damage, cap it
						if( damage > targ_armor_per )
							damage = targ_armor_per;
						running_damage += damage;

						// DEBUG
						// if (DEBUG_BB)
						//    printf("   - torp %4d : damage %d : running %d \n",
						//       k,damage,running_damage );

						// if stack dead, handle it
						if( running_damage > fireList[i]->btoken_->target->armor )
						{
							if( DEBUG_BB )
								printf("   - torps blow stack of %d ships to smithereens \n",
                                     fireList[i]->btoken_->target->quantity );

							// set target to destroyed state
							fireList[i]->btoken_->target->shields  = 0;
							fireList[i]->btoken_->target->armor    = 0;
							fireList[i]->btoken_->target->quantity = 0;
							fireList[i]->btoken_->target->ignore   = true;

							// done with target
							fireList[i]->btoken_->target = NULL;

							// next!
							fireList[i]->findTargetInRange(tList, element, gameData);

							// this should probably be functionalized ...
							if( fireList[i]->btoken_->target )
							{
								running_damage = 0;
								tohit = ( (double) element->missle.accuracy) / 100.0;
								if (fireList[i]->btoken_->target->jammAcc > fireList[i]->btoken_->compAcc)
								{
									tohit = tohit *
                                      (1.0- (fireList[i]->btoken_->target->jammAcc -
                                             fireList[i]->btoken_->compAcc));
								}
								else
								{
									tohit = tohit +
                                             ((1.0-tohit) *
                                              (fireList[i]->btoken_->compAcc -
                                               fireList[i]->btoken_->target->jammAcc));

								}
								hitNum = (unsigned) (100000 * tohit);

								if (DEBUG_BB)
								{
									printf("   - tohit = %6.4f : hitNum = %d \n",tohit,hitNum);
									printf("   - target (oid=%lu) : quantity %d : shields %d : armor %d\n",
                                        fireList[i]->btoken_->target->ship->objectId,
                                        fireList[i]->btoken_->target->quantity,
                                        fireList[i]->btoken_->target->shields,
                                        fireList[i]->btoken_->target->armor);
								}

								targ_armor_per = fireList[i]->btoken_->target->armor/
									fireList[i]->btoken_->target->quantity;
							}
						}
					}
					else // miss, damage shield only
					{
						damage = element->missle.damage / 8;
						if (fireList[i]->btoken_->target->shields)
						{
							if (damage < fireList[i]->btoken_->target->shields)
								fireList[i]->btoken_->target->shields -= damage;
							else
								fireList[i]->btoken_->target->shields = 0;

							// DEBUG
							// printf("   - torp %4d is a miss : damage = %d : shields drop to %d\n",
							//          k,damage, fireList[i]->btoken_->target->shields);

						}
					}
				}

				// apply running armor damage to ships
				// o kills
				// O armor effects
				// o shield effects?

				if( running_damage )
				{
					unsigned kills = running_damage / targ_armor_per;

					// error check
					if( kills >= fireList[i]->btoken_->target->quantity )
					{
						printf("missile damage bug in fireWeaponTokens in battle.cpp\n");
					}

					// TBD: shield handling if any kills
					if( kills && fireList[i]->btoken_->target->shields )
					{
						// note: below may not be computationally safe
						fireList[i]->btoken_->target->shields =
                               (fireList[i]->btoken_->target->shields *
                                 ( fireList[i]->btoken_->target->quantity - kills)) /
                               ( fireList[i]->btoken_->target->quantity );
					}

					fireList[i]->btoken_->target->armor    -= running_damage;
					fireList[i]->btoken_->target->quantity -= kills;

					if( DEBUG_BB )
						printf("   - torps killed %d (applied %d) : remaining %d (s %d a %d)\n",
                                     kills,running_damage,
                                     fireList[i]->btoken_->target->quantity,
                                     fireList[i]->btoken_->target->shields,
                                     fireList[i]->btoken_->target->armor);

				}
			} // if missle
		}
	}
}

// players with regen shields capabilities get shield repair
// Ned BUG: investigate how regen shields actually work
static void regenerateShields(const std::vector<BattleToken*> &tList)
{
   for(unsigned i = 0; i < tList.size(); ++i)
   {
      // printf("token %d : regen %d \n",i,tList[i]->regen);

		// if not regenerating or dead, continue
      if( !tList[i]->regen || tList[i]->ignore ) continue;

		//if (tList[i]->shields) //Ned? only regen if some shields left?

		unsigned shieldMax = tList[i]->quantity * tList[i]->ship->shieldMax;
		if( tList[i]->shields >= shieldMax ) continue;

		if( DEBUG_BB ) printf("shield regen: token %d : prev %d ",i,tList[i]->shields);

		// add 10%, up to max
		tList[i]->shields += shieldMax / 10;
		if( tList[i]->shields > shieldMax )
			tList[i]->shields = shieldMax;

		if( DEBUG_BB ) printf(" : now %d \n",tList[i]->shields);
   }
}

// fleeing ships check to see if can leave battleboard yet ...
//
// Ned BUG: check number of squares of move needed to leave board
static void checkFleeing(const std::vector<BattleToken*> &tList, BattleReport *brp)
{
	//foreach token
   for(unsigned i = 0; i < tList.size(); ++i)
   {
		// if dead or not trying to flee, continue;
      if( tList[i]->ignore || !tList[i]->flee ) continue;

		if( tList[i]->fleeCount >= RUNAWAY_COUNT )
		{
			tList[i]->ignore = true;
			tList[i]->xgrid = unsigned(-1);
			tList[i]->ygrid = unsigned(-1);

			brp->addMove(tList[i]);
			if( DEBUG_BB ) printf("token %d (oid=%lu) escaped the battle!\n",i,tList[i]->ship->objectId);
		}
   }
}

// full Battle Engine
// fList - fleets gathered in this location
void GameData::processBattle(std::vector<Fleet*> fList, GameData &gameData)
{
	//Ned random control should probably not be here ...
	unsigned seed = 1;
	srand(seed);

	std::map<unsigned,unsigned> playerMap; //what players were involved in this battle
	std::vector<unsigned> playerList;
	string mString = "A battle took place between the following Fleets: ";
	bool printComma = false;
	char temp[256];
	unsigned i; // MS bug
	for(i = 0; i < fList.size(); ++i)
	{
		if( playerMap[fList[i]->ownerID] == 0 )
		{
			playerMap[fList[i]->ownerID] = 1;
			playerList.push_back(fList[i]->ownerID);
		}

		// update the battle message
		if( printComma ) mString += ", ";
		sprintf(temp, "%lu", fList[i]->objectId);
		mString += temp;
		printComma = true;
	}

	// need at least two players to have a battle
	if( playerList.size() < 2 ) return;

	const Location coordinates = fList[0]->coordinates;

	// Step1: combat identification
	// for now assume all players hostile to each other
	// -- TBD: parse out hostilities (reduce fList and playerList)

	// notify the players of the impending battle
	for(i = 0; i < playerList.size(); ++i)
		gameData.playerMessage(playerList[i], 0, mString);

	// Step2: Battle Board Preperation
	// - Step2A : Identify Share of Tokens allowed on battleboard
	const unsigned fair_tokens = maxBattleTokens(fList, playerList);
	// printf("fair tokens = %d\n", fair_tokens);

	// - Step2B : Identify Specific Tokens for battle
	std::vector<BattleToken*> tList = buildBattleTokenList(fList, playerList, fair_tokens, gameData);
	// printf("tList size= %d\n", tList.size());

	// - Step2C : Assign various other token attributes
	// -- Position Battle Board (set x,y)
	placeBattleTokens(tList, playerList);

	//Ned allow participants to see details of designs in combat
	BattleReport *brp = new BattleReport;
	brp->addTokens(tList);

	// -- Assign Battle Mass
	assignBattleMass( tList );
	// -- Re-sort tList based on bmass (for ease in movement loop)
	tList = sortBattleTokensByMass( tList );
	// -- build the firing order list
	std::vector<WeaponToken*> fireList = buildFireList(tList, gameData);

	// Step3: Main Battle Loop
	// - Step3A : 16 battle rounds (max)
	for(unsigned round = 0; round < MAX_ROUNDS; ++round)
	{
		if( DEBUG_BB )
			printf("Combat Round %2d : Let's Fight!\n", round);

		// - Step3B : 3 phases of battle movement per round
		for(unsigned phase = 0; phase < MAX_PHASES; ++phase)
		{
			// NOTE: is it really necessary to retarget each phase?
			retargetBattleTokens(tList, gameData);
			moveBattleTokens(tList, round, phase, brp);
		}

		// - Step3C : Firing Phase
		fireWeaponTokens(fireList, tList, gameData, brp);

		// - Step3D : Shield Regeneration
		regenerateShields( tList );

      // - Step3E : Flee Check
      checkFleeing(tList, brp);
	}

	// Step4 : Post Battle Actions
	// - Step4A : Compute Salvage (1/3 minerals from each thing destroyed)
	createBattleSalvage(tList, playerList, coordinates, gameData);

	// post the battle report to the players involved
	for(std::vector<unsigned>::const_iterator pi = playerList.begin(); pi != playerList.end(); ++pi)
	{
		PlayerData *pd = gameData.getPlayerData(*pi);
		for(std::vector<BattleToken*>::const_iterator ti = tList.begin(); ti != tList.end(); ++ti)
		{
			// check to see if this player has seen the design in this token
			if( pd->race->ownerID == (**ti).ownerID ) continue; // it's us

			Ship *shipDes = gameData.getDesignByID((**ti).ship->designhullID);

			std::vector<Ship*>::iterator ki;
			for(ki = pd->knownEnemyDesignList.begin();
			    ki != pd->knownEnemyDesignList.end() && (**ki).objectId != shipDes->objectId; ++ki)
			{
				//do nothing
			}

			if( ki == pd->knownEnemyDesignList.end() )
			{
				// not seen, add it directly
				pd->knownEnemyDesignList.push_back(new Ship(*shipDes));
			}
			else
			{
				// seen, check if it is just a space scan (no detail)
				if( (**ki).getName().empty() )
					**ki = *shipDes; // copy in the details
			}
		}

		pd->battles_.push_back(brp);
	}

	// - Step4B : Compute Tech
	// -- Ned TBD: compute tech for participants who survived

	// - Step4x : Misc
	// -- 4x1 : propagate kills/damage from battleToken to Ships
	assignDamageToShip(tList);

	// - Step4C : Clean Up Memory
	// -- TBD: walk the lists (battleTokens,weaponTokens) and cleanup any allocated objects

	// printf("got to the end of processBattle\n");
}

