set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P1_Y2411.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

sendFleetToPlanet "Long Range Scout #32003" \
         [findPlanetByName "Liver"]
set pid [findPlanetByName "Argon"]
set x [newStars $::ns_planet $pid $::ns_getX]
set y [newStars $::ns_planet $pid $::ns_getY]
set fid [findFleetByName "Long Range Scout #32003"]
newStars $::ns_planet $fid $::ns_addAtIndex 1 $::ns_orderMove \
   [expr $x - 1] [expr $y]

set pid [findPlanetByName "M16"]
set x [newStars $::ns_planet $pid $::ns_getX]
set y [newStars $::ns_planet $pid $::ns_getY]

newStars $::ns_planet 48012 $::ns_speed 0 7
newStars $::ns_planet 48014 $::ns_speed 0 7

newStars $::ns_planet 48016 $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders 48016 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48016 $::ns_addAtIndex 1 $::ns_orderMove $x $y
newStars $::ns_planet 48016 $::ns_speed 1 7
newStars $::ns_planet 48016 $::ns_addAtIndex 2 $::ns_orderTransport 1
newStars $::ns_orders 48016 2 $::ns_orderTransport 0 250

newStars $::ns_planet 48018 $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders 48018 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48018 $::ns_addAtIndex 1 $::ns_orderMove $x $y
newStars $::ns_planet 48018 $::ns_speed 1 7
newStars $::ns_planet 48018 $::ns_addAtIndex 2 $::ns_orderTransport 1
newStars $::ns_orders 48018 2 $::ns_orderTransport 0 250

newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 48011
newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 48011

newStars $::ns_save $owd/twoPtest_P1_Y2411.xml

