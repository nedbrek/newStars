set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/tiny_sparse_P2_Y2400.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

set fid [findFleetByName "Armed Probe #2443026239740"]
sendFleetToPlanet "Armed Probe #2443026239740" [findPlanetByName Pervo]
sendFleetToPlanet "Armed Probe #2443026239740"
sendFleetToPlanet "Armed Probe #2443026239740"
#newStars $::ns_planet $fid $::ns_speed 2 5
#newStars $::ns_planet $fid $::ns_speed 3 5

# get a useful scout
newStars $::ns_design $::ns_loadPre scout "Smaugarian Peeping Tom"
newStars $::ns_design $::ns_save

# build it
newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 $::designMap(3)

# set orders for max growth
for {set i 0} {$i < 100} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 1 1 3
   newStars $::ns_planet $hwPID $::ns_addAtIndex 2 1 4
}

newStars $::ns_save $owd/tiny_sparse_P2_Y2400.xml

