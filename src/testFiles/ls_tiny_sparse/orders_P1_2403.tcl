set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set file $owd/tiny_sparse_P1_Y2403.xml
set nsGui::playerNum [newStars $::ns_open $file]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

# send out colony ship
set fid [findFleetByName "Santa Maria #1138879964928"]
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderTransport 0
newStars $::ns_orders $fid 0 $::ns_orderTransport 0 25

sendFleetToPlanet "Santa Maria #1138879964928" \
   [findPlanetByName "Quark"]
newStars $::ns_planet $fid $::ns_speed 1 6
newStars $::ns_planet $fid $::ns_addAtIndex 2 $::ns_orderColonize

# send out new scout
set fid [findFleetByName "Long Range Scout #1099857924542"]
sendFleetToPlanet "Long Range Scout #1099857924542" \
   [findPlanetByName "Zarquon"]
newStars $::ns_planet $fid $::ns_speed 0 7

# redirect destroyer
set fid [findFleetByName "Stalwart Defender #1346313939706"]
newStars $::ns_planet $fid $::ns_deleteAtIndex 0
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderMove 252 110
newStars $::ns_planet $fid $::ns_speed 0 6

# move orig lrs
set fid [findFleetByName "Long Range Scout #1274205627917"]
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderMove 197 271
newStars $::ns_planet $fid $::ns_speed 0 6

for {set i 0} {$i < 12} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 3
}
newStars $::ns_planet $hwPID $::ns_addAtIndex 1 0 4

newStars $::ns_save $file

