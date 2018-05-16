set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/tiny_sparse_P1_Y2402.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

set fid [findFleetByName "Long Range Scout #1274205627917"]
newStars $::ns_planet $fid $::ns_deleteAtIndex 0
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderMove 164 175

for {set i 0} {$i < 8} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 3
}
newStars $::ns_planet $hwPID $::ns_addAtIndex 1 0 4

newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 $::designMap(1)

newStars $::ns_save $owd/tiny_sparse_P1_Y2402.xml

