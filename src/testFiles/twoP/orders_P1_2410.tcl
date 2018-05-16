set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P1_Y2410.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

set pid [findPlanetByName "M16"]
set x [newStars $::ns_planet $pid $::ns_getX]
set y [newStars $::ns_planet $pid $::ns_getY]

newStars $::ns_planet 48012 $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders 48012 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48012 $::ns_addAtIndex 1 $::ns_orderMove $x $y

newStars $::ns_planet 48014 $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders 48014 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48014 $::ns_addAtIndex 1 $::ns_orderMove $x $y

newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 48011
newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 48011

newStars $::ns_save $owd/twoPtest_P1_Y2410.xml

