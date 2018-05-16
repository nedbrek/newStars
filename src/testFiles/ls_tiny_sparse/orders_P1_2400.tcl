set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/tiny_sparse_P1_Y2400.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

# send fleets
set fid [findFleetByName "Armed Probe #1339845579629"]
sendFleetToPlanet "Armed Probe #1339845579629"
newStars $::ns_planet $fid $::ns_speed 0 6

set fid [findFleetByName "Long Range Scout #1274205627917"]
sendFleetToPlanet "Long Range Scout #1274205627917"
newStars $::ns_planet $fid $::ns_speed 0 6

set fid [findFleetByName "Stalwart Defender #1346313939706"]
sendFleetToPlanet "Stalwart Defender #1346313939706" \
   [findPlanetByName "Tull"]
newStars $::ns_planet $fid $::ns_speed 0 6

# find the remote miner, scrap it
set fid [findFleetByName "Cotton Picker #1112332835768"]
if {$fid == -1} {
   puts "Can't find remote miner"
   exit
}
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_deleteAtIndex

# dump remaining resouces into factories
for {set i 0} {$i < 7} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 3
}

set ::curResearch 3
newStars $::ns_player $::ns_researchTax 15

newStars $::ns_save $owd/tiny_sparse_P1_Y2400.xml

