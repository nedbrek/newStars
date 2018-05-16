set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P1_Y2412.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

sendFleetToPlanet "Long Range Scout #32003" [findPlanetByName "Cherry"]

set pid [findPlanetByName "M16"]
set x [newStars $::ns_planet $pid $::ns_getX]
set y [newStars $::ns_planet $pid $::ns_getY]

newStars $::ns_planet 32012 $::ns_addAtIndex 0 $::ns_orderTransport 1
newStars $::ns_orders 32012 0 $::ns_orderTransport 0 210
sendFleetToPlanet "" $hwPID 32012

newStars $::ns_planet 48012 $::ns_addAtIndex 0 $::ns_orderTransport 1
newStars $::ns_orders 48012 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48012 $::ns_addAtIndex 1 $::ns_orderTransport 1 32012
newStars $::ns_orders 48012 1 $::ns_orderTransport 4 22
sendFleetToPlanet "" $hwPID 48012
newStars $::ns_planet 48012 $::ns_speed 2 7

newStars $::ns_planet 48014 $::ns_addAtIndex 0 $::ns_orderTransport 1
newStars $::ns_orders 48014 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48014 $::ns_addAtIndex 1 $::ns_orderTransport 1 32012
newStars $::ns_orders 48014 1 $::ns_orderTransport 4 22
sendFleetToPlanet "" $hwPID 48014
newStars $::ns_planet 48014 $::ns_speed 2 7

newStars $::ns_planet 48016 $::ns_speed 0 8
newStars $::ns_planet 48018 $::ns_speed 0 8

newStars $::ns_planet 48020 $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders 48020 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48020 $::ns_addAtIndex 1 $::ns_orderMove $x $y
newStars $::ns_planet 48020 $::ns_speed 1 7
newStars $::ns_planet 48020 $::ns_addAtIndex 2 $::ns_orderTransport 1
newStars $::ns_orders 48020 2 $::ns_orderTransport 0 250

newStars $::ns_planet 48022 $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders 48022 0 $::ns_orderTransport 0 250
newStars $::ns_planet 48022 $::ns_addAtIndex 1 $::ns_orderMove $x $y
newStars $::ns_planet 48022 $::ns_speed 1 7
newStars $::ns_planet 48022 $::ns_addAtIndex 2 $::ns_orderTransport 1
newStars $::ns_orders 48022 2 $::ns_orderTransport 0 250

newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 48011

for {set i 0} {$i < 300} {incr i} {
   newStars $::ns_planet 27 $::ns_addAtIndex 0 1 3
   newStars $::ns_planet 27 $::ns_addAtIndex 1 1 4
}

newStars $::ns_save $owd/twoPtest_P1_Y2412.xml

