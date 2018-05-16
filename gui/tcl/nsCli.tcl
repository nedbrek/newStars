package require tdom

namespace eval nsGui {
   set playerNum      -1

   set myPlanetMap(0) 0
   set myPlanetCt     0

   set explorePlanets ""
}

proc buildMyPlanetMap {} {
   set nsGui::myPlanetCt 0
   for {set i 0} {$i < $::numPlanets} {incr i} {
      set planetId $::planetMap($i)
      set planetXML [newStars $::ns_planet $planetId $::ns_getXML]
      set doc [dom parse $planetXML]
      set node [$doc documentElement root]
      set ownnode [lindex [$node getElementsByTagName OWNERID] 0]
      set ownnode2 [$ownnode firstChild]
      if {[$ownnode2 nodeValue] == $nsGui::playerNum} {
         set nsGui::myPlanetMap($nsGui::myPlanetCt) $planetId
         incr nsGui::myPlanetCt
      }
      $doc delete
   }
}

proc findPlanetByName {name} {
   for {set i 0} {$i < $::numPlanets} {incr i} {
      set curPid $::planetMap($i)
      if {[newStars $::ns_planet $curPid $::ns_getName] eq $name} {
         return $curPid
      }
   }
   return -1
}

proc findFleetByName {name} {
   foreach i [array names ::allFleetMap] {
      set curFid $::allFleetMap($i)
      if {[newStars $::ns_planet $curFid $::ns_getName] eq $name} {
         return $curFid
      }
   }
   return -1
}

proc findNearestUnexploredPlanet {x y} {
   set bestPid -1
   set dst 0

   for {set i 0} {$i < $::numPlanets} {incr i} {
      set curPid $::planetMap($i)

      if {[lsearch $nsGui::explorePlanets $curPid] != -1} {
         continue
      }

      set planetXML [newStars $::ns_planet $curPid $::ns_getXML]
      set doc [dom parse $planetXML]
      set node [$doc documentElement root]
      set ownnode [lindex [$node getElementsByTagName YEARDATAGATHERED] 0]
      set yearData [[$ownnode firstChild] nodeValue]
      if {$yearData != 0} {
         $doc delete
         continue
      }
      $doc delete

      set nx [newStars $::ns_planet $curPid $::ns_getX]
      set ny [newStars $::ns_planet $curPid $::ns_getY]

      set ndst [expr sqrt(pow($nx-$x,2)+pow($ny-$y,2))]

      if {$bestPid == -1 || $ndst < $dst} {
         set bestPid $curPid
         set dst     $ndst
      }
   }
   return $bestPid
}

proc sendFleetToPlanet {fname {pid -1} {fid -1}} {
   if {$fid == -1} {
      set fid [findFleetByName $fname]
   }

   # find a planet
   if {$pid == -1} {
      # use find nearest logic
      set numOrders [newStars $::ns_planet $fid $::ns_getNumOrders]
      if {$numOrders > 0} {
         set x [newStars $::ns_orders $fid [expr $numOrders - 1] $::ns_getX]
         set y [newStars $::ns_orders $fid [expr $numOrders - 1] $::ns_getY]
      } else {
         set x [newStars $::ns_planet $fid $::ns_getX]
         set y [newStars $::ns_planet $fid $::ns_getY]
      }
      set pid [findNearestUnexploredPlanet $x $y]
   }
   set sx [newStars $::ns_planet $pid $::ns_getX]
   set sy [newStars $::ns_planet $pid $::ns_getY]

   set oid [newStars $::ns_planet $fid $::ns_getNumOrders]

   # go there
   newStars $::ns_planet $fid $::ns_addAtIndex $oid $::ns_orderMove $sx $sy

   # add it to the ignore list
   lappend nsGui::explorePlanets $pid

   return $pid
}

