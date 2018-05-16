set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P2_Y2401.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

# add current dests to the ignore list
lappend nsGui::explorePlanets [findPlanetByName "Zaran II"]
lappend nsGui::explorePlanets [findPlanetByName "Equuleus"]

set fid [findFleetByName "Smaugarian Peeping Tom #2199369552318"]
sendFleetToPlanet "Smaugarian Peeping Tom #2199369552318"
newStars $::ns_planet $fid $::ns_speed 0 7

sendFleetToPlanet "Smaugarian Peeping Tom #2199369552318"
sendFleetToPlanet "Smaugarian Peeping Tom #2199369552318"
sendFleetToPlanet "Smaugarian Peeping Tom #2199369552318"

newStars $::ns_save $owd/twoPtest_P2_Y2401.xml

