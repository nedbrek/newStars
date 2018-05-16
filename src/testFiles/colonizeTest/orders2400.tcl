set owd [pwd]
cd ../../../gui/tcl
load ./nstclgui.so
source nsCli.tcl

set nsGui::playerNum [newStars $::ns_open $owd/colonizeTest_P1_Y2400.xml]
buildMyPlanetMap

# find scout
set fid [findFleetByName "Long Range Scout #32003"]
if {$fid == -1} {
   puts "Can't find Long range scout"
   exit
}

set hwPID $nsGui::myPlanetMap(0)

# find a potential planet to colonize
set x [newStars $::ns_planet $hwPID $::ns_getX]
set y [newStars $::ns_planet $hwPID $::ns_getY]
set scoutPid [findNearestUnexploredPlanet $x $y]
set sx [newStars $::ns_planet $scoutPid $::ns_getX]
set sy [newStars $::ns_planet $scoutPid $::ns_getY]

# go there
newStars $::ns_planet $fid $::ns_addAtIndex 0 $::ns_orderMove $sx $sy

#### copy the colony ship
set colShipId -1
for {set i 0} {$i < $::numHulls} {incr i} {
   set tmpId $::hullMap($i)
   if {[newStars $::ns_hull $tmpId $::ns_getName] eq "Colony Ship"} {
      set colShipId $tmpId
   }
}

# find the engine
set engineId -1
set colzrId  -1
for {set i 0} {$i < $::numComponents} {incr i} {
   set ii $::componentIdxList($i)
   set cmpName [newStars $::ns_component $ii $::ns_getName]
   if {$cmpName eq "Quick Jump 5"} {
      set engineId $ii
   } elseif {$cmpName eq "Colonization Module"} {
      set colzrId  $ii
   }
}

newStars $::ns_design $::ns_loadPre 1 $colShipId
newStars $::ns_design $::ns_setName "Pinta"
newStars $::ns_design $::ns_addAtIndex 0 $engineId
newStars $::ns_design $::ns_addAtIndex 1 $colzrId
newStars $::ns_design $::ns_save

newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 1 $::designMap(7)

for {set i 0} {$i < 4} {incr i} {
   newStars $::ns_planet $hwPID $::ns_addAtIndex 0 0 4
}

# auto build factory
#  newStars $::ns_planet $hwPID $::ns_addAtIndex 0 1 3

newStars $::ns_save $owd/colonizeTest_P1_Y2400.xml

