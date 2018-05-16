set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P1_Y2400.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

# xfer test
set fid [findFleetByName "Teamster #1184372354219"]
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderTransport
newStars $::ns_orders $fid 0 $::ns_orderTransport 1 1

# send fleets
sendFleetToPlanet "Armed Probe #1339845579629"
sendFleetToPlanet "Armed Probe #1339845579629"
sendFleetToPlanet "Armed Probe #1339845579629"
sendFleetToPlanet "Armed Probe #1339845579629" $hwPID

sendFleetToPlanet "Cotton Picker #1112332835768"
sendFleetToPlanet "Cotton Picker #1112332835768" $hwPID

set fid [findFleetByName "Stalwart Defender #1346313939706"]
sendFleetToPlanet "Stalwart Defender #1346313939706"
newStars $::ns_planet $fid $::ns_speed 0 6

# slow down the scout, to save fuel
set fid [findFleetByName "Long Range Scout #1274205627917"]
sendFleetToPlanet "Long Range Scout #1274205627917" \
         [findPlanetByName "Oby VI"]
newStars $::ns_planet $fid $::ns_speed 0 5
sendFleetToPlanet "Long Range Scout #1274205627917"
newStars $::ns_planet $fid $::ns_speed 1 5
sendFleetToPlanet "Long Range Scout #1274205627917"
newStars $::ns_planet $fid $::ns_speed 2 6
sendFleetToPlanet "Long Range Scout #1274205627917"

# find the remote miner, scrap it
set fid [findFleetByName "Cotton Picker #1112332835768"]
if {$fid == -1} {
   puts "Can't find remote miner"
   exit
}
newStars $::ns_planet $fid $::ns_addAtIndex 2 $::ns_deleteAtIndex

# dump remaining resouces into factories
for {set i 0} {$i < 300} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 0 1 3
   newStars $::ns_planet $hwPID $::ns_addAtIndex 1 1 4
}

newStars $::ns_save $owd/twoPtest_P1_Y2400.xml

