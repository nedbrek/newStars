#ifndef TURNGEN__H
#define TURNGEN__H
//
// C++ Interface: turnGen
//
// Description:
//
// Copyright: See COPYING file that comes with this distribution
//
#include "classDef.h"
#include "race.h"

/// handler for generating the next turn
class ActionDispatcher
{
public:
    ActionDispatcher(GameData *gd);

	 /// determine which planets can be seen by players
	 void calcScan(void);

    /// generate next turn, (calls private phase generate functions)
    void dispatchAll(void);

protected: // data
    GameData *gd_;

protected: // methods
    void clearWorkingSet(); //clear the working variables that propagate data through
                            //the turn sequence as required
    void cleanEndOfTurn();  //recalculate everything and get ready for outputting data
    void scrapFleets(void);
    //# Scrapping fleets (with possible tech gain)
    void waypointLoad(bool guiBased);//# * Waypoint 0 load tasks (if done by hand) *
    void waypointUnload(bool guiBased);//# Waypoint 0 unload tasks
    //# Waypoint 0 Colonization/Ground Combat resolution (with possible tech gain)
    //# Waypoint 0 load tasks (if done via task tile order)
    //# * Other Waypoint 0 tasks *
    //# MT moves
    //# In-space packets move and decay
    //
    //   1. PP packets (de)terraform
    //   2. Packets cause damage
    //   3. * Planets that where hit and end up with 0 colonists become uninhabited (lose starbase and defences) *
    //
    void packetMove();

    //# Fleets move (run out of fuel, hit minefields (fields     //# Wormhole entry points jiggle
    void fleetMove(void);//# Fleets move (run out of fuel, hit minefields (fields reduce as they are hit), stargate, wormhole travel)
    //# Inner Strength colonists grow in fleets (if orbiting planet owned by same race, excess overflows onto it)
    //# Mass Packets still in space and Salvage decay
    //# Wormhole exit points jiggle
    //# Wormhole endpoints degrade/jump
    //# SD Minefields detonate (possibly damaging again fleet that hit minefield during movement)
    //# Mining
    //# Production (incl. research, packet launch, fleet/starbase construction)

    void planetaryUpdateVariables(void);
	 void fleetUpdateFuel(void);

    void planetaryExtractMineralWealth(void);
    void planetaryProcessBuildQueue(void);
    void performResearch();
    //# SS Spy bonus obtained
    void stealResearch();
    void planetaryPopulationGrowth(void); //# Population grows/dies
    //# Packets that just launched and reach their destination cause damage
    //# Random events (comet strikes, etc.)

    //# Fleet battles (with possible tech gain)
    void fleetBattles();
    //# Meet MT

    void bombingRuns();
    //# Bombing

    //   1. * Player 1 bombing calculated *
    //            * Normal/LBU Bomb Damage Calculated *
    //            * Smart Bomb Damage Calculated *
    //            * Defences Recalculated *
    //   2. * Player 2 bombing calculated and so on in order with players 3, 4... *

    //# * Planets with 0 colonists become uninhabited (lose starbase and defences) *
//    void waypoint1Unload(void);  //# Waypoint 1 unload tasks
    void waypointColonize(void);
    void resolveGroundCombat(void);  //resolves pending ground combat or colonizes stored in the planetary WaypointDropRecords
    //# Waypoint 1 Colonization/Ground Combat resolution (with possible tech gain)
//    void waypoint1Load(void); //# Waypoint 1 load tasks
    //# Mine Laying
    void mineLayingPhase(int phase);
    //# Fleet Transfer
    //# * Waypoint 1 Fleet Merge *
    //# CA Instaforming
    //# * Minefield Decay *
    void decayMinefields();
    //# Mine sweeping
    //# Starbase and fleet repair
    //# Remote Terraforming
};

#endif

