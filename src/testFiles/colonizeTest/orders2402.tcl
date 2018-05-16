set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/colonizeTest_P1_Y2402.xml]

#### find the colony ship
set fid [findFleetByName "NF-Marge 2401"]
if {$fid == -1} {
   puts "Can't find Pinta fleet"
   exit
}

# load colonists
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderTransport 0
newStars $::ns_orders $fid 0 $::ns_orderTransport 0 25

# find dest planet
set curPid [findPlanetByName "Romeo"]
set x [newStars $::ns_planet $curPid $::ns_getX]
set y [newStars $::ns_planet $curPid $::ns_getY]

newStars $::ns_planet $fid $::ns_addAtIndex 1 $::ns_orderMove $x $y
newStars $::ns_planet $fid $::ns_addAtIndex 2 $::ns_orderColonize

newStars $::ns_save $owd/colonizeTest_P1_Y2402.xml

