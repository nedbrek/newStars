set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P1_Y2409.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

newStars $::ns_player $::ns_researchTax 0

set pid [findPlanetByName "M16"]
set x [newStars $::ns_planet $pid $::ns_getX]
set y [newStars $::ns_planet $pid $::ns_getY]

set fid [findFleetByName "Santa Maria #32009"]
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders $fid 0 $::ns_orderTransport 0 25
newStars $::ns_planet $fid $::ns_addAtIndex 1 $::ns_orderMove $x $y
newStars $::ns_planet $fid $::ns_addAtIndex 2 $::ns_orderColonize

set fid [findFleetByName "Teamster #32012"]
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders $fid 0 $::ns_orderTransport 0 210
newStars $::ns_planet $fid $::ns_addAtIndex 1 $::ns_orderMove $x $y

#design a new privateer
newStars $::ns_design $::ns_loadPre 1 11
newStars $::ns_design $::ns_addAtIndex 0 67
newStars $::ns_design $::ns_addAtIndex 1 83
newStars $::ns_design $::ns_addAtIndex 2 83
newStars $::ns_design $::ns_addAtIndex 4 83
newStars $::ns_design $::ns_save

newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 1000
newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 1000

newStars $::ns_save $owd/twoPtest_P1_Y2409.xml

