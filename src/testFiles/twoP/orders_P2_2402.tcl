set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P2_Y2402.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

newStars $::ns_save $owd/twoPtest_P2_Y2402.xml

