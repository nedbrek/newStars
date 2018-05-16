set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set file $owd/tiny_sparse_P1_Y2405.xml
set nsGui::playerNum [newStars $::ns_open $file]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

# change orders lrs
set fid [findFleetByName "Long Range Scout #1099857924542"]
newStars $::ns_planet $fid $::ns_deleteAtIndex 0
sendFleetToPlanet "Long Range Scout #1099857924542" \
   [findPlanetByName "409"]
newStars $::ns_planet $fid $::ns_speed 0 6

# change orders lrs
set fid [findFleetByName "Long Range Scout #1274205627917"]
newStars $::ns_planet $fid $::ns_deleteAtIndex 0
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderMove 148 319
newStars $::ns_planet $fid $::ns_speed 0 7

# change orders destroyer
set fid [findFleetByName "Stalwart Defender #1346313939706"]
newStars $::ns_planet $fid $::ns_deleteAtIndex 0
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderMove 284 142
newStars $::ns_planet $fid $::ns_speed 0 5

# teamster gets 210 col
set fid [findFleetByName "Teamster #1184372354219"]
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderTransport 0
newStars $::ns_orders $fid 0 $::ns_orderTransport 0 210

for {set i 0} {$i < 10} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 3
}

set nextResearch 1

newStars $::ns_save $file

