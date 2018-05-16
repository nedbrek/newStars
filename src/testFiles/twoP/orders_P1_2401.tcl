set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P1_Y2401.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

set fid  [findFleetByName "Teamster #1184372354219"]
set fid2 [findFleetByName "Santa Maria #1138879964928"]
newStars $::ns_planet $fid  $::ns_orderTransport $fid2 1 1

newStars $::ns_save $owd/twoPtest_P1_Y2401.xml

