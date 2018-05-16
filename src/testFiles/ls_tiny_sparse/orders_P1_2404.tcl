set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set file $owd/tiny_sparse_P1_Y2404.xml
set nsGui::playerNum [newStars $::ns_open $file]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

# change orders armed probe
set fid [findFleetByName "Armed Probe #1339845579629"]
newStars $::ns_planet $fid $::ns_deleteAtIndex 0
sendFleetToPlanet "Armed Probe #1339845579629" \
    [findPlanetByName "Quark"]

# change orders destroyer
set fid [findFleetByName "Stalwart Defender #1346313939706"]
newStars $::ns_planet $fid $::ns_deleteAtIndex 0
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderMove 274 114
newStars $::ns_planet $fid $::ns_speed 0 6

for {set i 0} {$i < 8} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 3
}

newStars $::ns_save $file

