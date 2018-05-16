#ifndef GEN_HPP
#define GEN_HPP

#include <vector>
#include <string>
#undef HUGE

struct UnivParms
{
public: // types
   enum UniverseDensity   { SPARSE, NORMAL, DENSE, PACKED, UD_MAX };
   enum UniversePositions { CLOSE, MODERATE, FARTHER, DISTANT, UP_MAX };
   enum UniverseSize      { TINY, SMALL, MEDIUM, LARGE, HUGE, US_MAX };

	static const char *const udNames[UD_MAX+1];
	static const char *const upNames[UP_MAX+1];
	static const char *const usNames[US_MAX+1];

public: // data
   std::string       universeName_;
	std::string       baseName_;
   UniverseDensity   density_;
   UniversePositions positions_;
   UniverseSize      size_;

   // x&y size derived from size at setup
   // tiny=400; small=800; medium=1200; large=1600; huge=2000
   /*
   size   density #    x   y    % full
   -----------------------------------
   tiny   sparse  24  400  400  0.0150
   tiny   normal  32  400  400  0.0200
   tiny   dense   40  400  400  0.0250
   tiny   packed  60  400  400  0.0375

   small  sparse  96  800  800  0.0150
   small  normal 128  800  800  0.0200
   small  dense  160  800  800  0.0250
   small  packed 240  800  800  0.0375

   medium sparse 216 1200 1200  0.0150
   medium normal 288 1200 1200  0.0200
   medium dense  360 1200 1200  0.0250
   medium packed 540 1200 1200  0.0375

   large  sparse 384 1600 1600  0.0150
   large  normal 512 1600 1600  0.0200
   large  dense  640 1600 1600  0.0250
   large  packed 929 1600 1600  0.0362890625  // 937,923,916

   huge   sparse 600 2000 2000  0.0150
   huge   normal 800 2000 2000  0.0200
   huge   dense  946 2000 2000  0.02365   // 949,953,946
   huge   packed 949 2000 2000  0.023725  // 956,952,951
   */

   bool max_minerals_; // Beginner: Maximum Minerals
   bool slow_tech_;    // Slower Tech Advances
   bool accelerated_;  // Accelerated BBS play
   bool random_;       // Random Events
   bool alliances_;    // Computer Players Form Alliances
   bool pubPlayScore_; // Public Player Scores
   bool clumping_;     // Galaxy Clumping

	std::vector<class Race*> races_;

public: // methods
   UnivParms(void);

   void dumpXMLToString(std::string &theString);
   bool parseXMLtree(struct mxml_node_s *tree);
};

#endif

