set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/twoPtest_P1_Y2408.xml]
buildMyPlanetMap
set hwPID $nsGui::myPlanetMap(0)

set ::nextResearch 3
newStars $::ns_player $::ns_researchTax 50

newStars $::ns_save $owd/twoPtest_P1_Y2408.xml

