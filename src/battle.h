#ifndef BATTLE_H
#define BATTLE_H

#include <vector>
#include <string>

class BattleToken;

// ---------------------------------------------------------------------------
// what happened during the battle, to send to players involved for their VCR
class BattleReport
{
public: // types
	class Token;
	class Event;

protected:
	std::vector<Token*> tokens_;
	std::vector<Event*> events_;

public:
	BattleReport(void);
	~BattleReport(void);

	/// parse/send data for 'this' in xml
   bool parseXML(struct mxml_node_s *tree);
	void toXMLString(std::string &theString) const;

	/// create token reports from a token list
	void addTokens(const std::vector<BattleToken*> &tList);

	void addMove(const BattleToken *bt);
	void addFire(unsigned damage, class WeaponToken *wt);
};

#endif
