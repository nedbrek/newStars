set paneFnSz {0   0   0  12 12 12   12 12 12}

# draw a rectangle from min hab to max hab
proc drawHabBar {canvas ctr rng color} {
   $canvas create rectangle [expr ($ctr - $rng)*2.5]  0 \
                            [expr ($ctr + $rng)*2.5+1] 21 -fill $color
}

# draw the plus at planet orig hab, draw a diamond on cur hab (c2)
proc drawPlus {can ctr c2 color} {
   $can create line [expr $ctr * 2.5]      6 [expr $ctr * 2.5]     17 -fill $color -tags plus
   $can create line [expr $ctr * 2.5 - 5] 11 [expr $ctr * 2.5 + 5] 11 -fill $color -tags plus

   $can create polygon [expr $c2 * 2.5    ]  4 \
                       [expr $c2 * 2.5 + 7] 11 \
                       [expr $c2 * 2.5    ] 18 \
                       [expr $c2 * 2.5 - 7] 11 -outline $color -fill "" -tags plus
}

# draw a scanner circle for planet (or fleet) 'pid'
proc paintScan {pane magnify pid pen} {
   set px [newStars $::ns_planet $pid $::ns_getX]
   set py [newStars $::ns_planet $pid $::ns_getY]
   set scanList [newStars $::ns_planet $pid $::ns_getScan 0]

   set x [expr floor($px * $magnify)]
   set y [expr floor($py * $magnify)]

   set rad [expr [lindex $scanList $pen] * $magnify]
   if {$rad > 0} {
       if {$pen} {
          set clr "#606000"
       } else {
          set clr "#7F0000"
       }
       $pane create oval [expr $x - $rad] [expr $y - $rad] \
           [expr $x + $rad] [expr $y + $rad] -fill $clr -outline ""
   }
}

# query C for planet info and draw them in 'pane'
proc drawPlanets {pane magnify} {
   set ps [$nsGui::turnRoot getElementsByTagName PLANET_LIST]
   set ps [$ps getElementsByTagName PLANET]

   for {set pnum 0} {$pnum < $::numPlanets} {incr pnum} {
      set p [lindex $ps $pnum]
      set pid [[[$p getElementsByTagName OBJECTID] firstChild] nodeValue]

      set x [[[$p getElementsByTagName X_COORD] firstChild] nodeValue]
      set y [[[$p getElementsByTagName Y_COORD] firstChild] nodeValue]
      set x [expr floor($x * $magnify)]
      set y [expr floor($y * $magnify)]

      if {[lindex $::paneFnSz $::magnidx] > 0} {
         set tName [$pane create text $x [expr $y + 10] -font $::paneFont \
 -text [newStars $::ns_planet $pid $::ns_getName] -fill white -anchor n]
         $pane lower $tName inspaceobj

         set ::planetMapId($tName) $pid
         set ::paneMapName($tName) "PLANET"
      }

      # assume normal (neutral owner) view
      set sz  1
      set clr white
      set fclr white

      set year [[[$p getElementsByTagName YEARDATAGATHERED] firstChild] nodeValue]
      if {$year == 0} {
         set clr gray; # unknown world
      } else {
         set oNode [lindex [$p getElementsByTagName OWNERID] 0]
         set owner [[$oNode firstChild] nodeValue]

	      if {$owner != 0} {
		      set fclr red
			   if {$owner == $nsGui::playerNum} {
				   set fclr green
			   } else {
					set rel [newStars $::ns_player $::ns_playerRelation $owner 0]
					if {$rel == 2} {
						set fclr yellow
				   }
			   }
			}

			if {$nsGui::viewMode == 4} {
				# hab view, with flags for ownership
	         if {$owner != 0} {
	            set tName [$pane create rectangle [expr $x - 1] [expr $y - 20] \
		             [expr $x + 7] [expr $y - 12] -fill $fclr]
					set ::planetMapId($tName) $pid
					set ::paneMapName($tName) "PLANET"

	            set tName [$pane create line $x [expr $y - 12] $x $y -fill black -tags flag]
					set ::planetMapId($tName) $pid
					set ::paneMapName($tName) "PLANET"
				}

				# override with hab
				set hab    [newStars $::ns_planet $pid $::ns_getHab 0]
				set maxHab [newStars $::ns_planet $pid $::ns_getHab 1]
				if {$hab < 0} {
					set clr red
					if {$maxHab > 0} {
						set clr yellow
					}
				} else {
					set clr green
					set sz [expr (($hab - 1) / 10) + 2]
					if {$sz > 10} {set sz 10}
				}
	      } else {
				# transfer flag color to planet
				set clr $fclr
	      }
      }

      set tmp [$pane create oval [expr $x - $sz] [expr $y - $sz] \
          [expr $x + $sz] [expr $y + $sz] -fill $clr -tag inspaceobj -outline ""]
      $pane raise flag $tmp

      set ::planetMapId($tmp) $pid
      set ::paneMapName($tmp) "PLANET"
   }
}

# draw a line for each jump in 'fid' orders
proc drawPaths {magnify fid} {
   set x1 [expr floor([newStars $::ns_planet $fid $::ns_getX] * $magnify)]
   set y1 [expr floor([newStars $::ns_planet $fid $::ns_getY] * $magnify)]

   set tmp [newStars $::ns_planet $fid $::ns_getNumOrders]
   for {set i 0} {$i < $tmp} {incr i} {
      set oxml    [xml2list [newStars $::ns_planet $fid $::ns_getOrderXMLfromIndex $i]]
      set mainXML [lindex $oxml 2]
      set orderID [lindex [findXMLbyTag "ORDERID" $mainXML 2] 1]
      if {$orderID != 1} { continue }

      set x2 [expr [lindex [findXMLbyTag "X_COORD" $mainXML 2] 1] * $magnify]
      set y2 [expr [lindex [findXMLbyTag "Y_COORD" $mainXML 2] 1] * $magnify]

      set lid [.tGui.cScannerPane create line $x1 $y1 $x2 $y2 -fill blue -tags "paths path$fid"]
      .tGui.cScannerPane lower $lid inspaceobj

      set x1 $x2
      set y1 $y2
   }
}

# query C for fleet in space info and draw them in 'pane'
proc drawFleets {pane magnify fleetAry kind} {
   set clabel "FLEET"
   if {$kind eq "enemy"} {
      set clabel "EFLEET"
   }

   foreach fnum [array names $fleetAry] {
      set fid [subst [join "\$ $fleetAry (\$fnum)" ""]]
      set x [expr floor([newStars $::ns_planet $fid $::ns_getX] * $magnify)]
      set y [expr floor([newStars $::ns_planet $fid $::ns_getY] * $magnify)]

      set tmp [$pane create rectangle [expr $x - 1] [expr $y - 1] \
          [expr $x + 1] [expr $y + 1] -fill white -tag inspaceobj]

      set ::planetMapId($tmp) $fid
      set ::paneMapName($tmp) $clabel
   }
}

# query C for minefield info and draw them in 'pane'
proc drawMines {pane magnify} {
   for {set mnum 0} {$mnum < $::numMinefields} {incr mnum} {
      set mfId $::mineFieldList($mnum)

      set x [expr floor([newStars $::ns_planet $mfId $::ns_getX] * $magnify)]
      set y [expr floor([newStars $::ns_planet $mfId $::ns_getY] * $magnify)]

      set fieldXML  [xml2list [newStars $::ns_planet $mfId  $::ns_getXML]]
      set mainXML   [lindex $fieldXML 2]

      set radiusXML [findXMLbyTag "RADIUS" $mainXML 2]
      if {[lindex $radiusXML 0] ne "#text"} { return }

      set radius [expr floor([lindex $radiusXML 1] * $magnify)]
      set x1 [expr $x - $radius]
      set y1 [expr $y - $radius]
      set x2 [expr $x + $radius]
      set y2 [expr $y + $radius]

      set tmp [$pane create oval $x1 $y1 $x2 $y2 -fill red -stipple @./mf-st.bmp]
      $pane lower $tmp inspaceobj

      set ::planetMapId($tmp) $mfId
      set ::paneMapName($tmp) "MINEFIELD"

      set tmp [$pane create rectangle [expr $x - 2] [expr $y - 2] [expr $x + 2] [expr $y + 2] \
               -fill #e84040 -tag inspaceobj -outline ""]
      set ::planetMapId($tmp) $mfId
      set ::paneMapName($tmp) "MINEFIELD"
   }
}

proc drawPackets {pane magnify} {
   for {set pnum 0} {$pnum < $::numPackets} {incr pnum} {
      set pktId $::packetList($pnum)

      set x [expr floor([newStars $::ns_planet $pktId $::ns_getX] * $magnify)]
      set y [expr floor([newStars $::ns_planet $pktId $::ns_getY] * $magnify)]

      set tmp [$pane create rectangle [expr $x - 2] [expr $y - 2] \
          [expr $x + 2] [expr $y + 2] -outline red -tag inspaceobj]

      set ::planetMapId($tmp) $pktId
      set ::paneMapName($tmp) "PACKET"
   }
}

# query C for universe info and draw it in 'top'.cScannerPane
proc drawStarMap {top magnify} {
   set pane $top.cScannerPane

   set width  [expr floor([newStars $::ns_universe $::ns_getX] * $magnify)]
   set height [expr floor([newStars $::ns_universe $::ns_getY] * $magnify)]

   unset -nocomplain ::planetMapId ::paneMapName
   $pane delete all
   $pane configure -scrollregion "0 0 $width $height"

   drawScanners $pane $magnify
   drawPlanets  $pane $magnify
   drawFleets   $pane $magnify ::fleetMap      self
   drawFleets   $pane $magnify ::enemyFleetMap enemy
   drawMines    $pane $magnify
   drawPackets  $pane $magnify

   foreach idx [array names ::allFleetMap] {
      drawPaths $magnify $::allFleetMap($idx)
   }
}

proc makeHabText {hab ctr} {
   if {$hab == 0} {
      return "$nsconst::gravLabel($ctr)g"
   }

   if {$hab == 1} {
      set bot [expr ($ctr - 50) * 4]
      return "$bot C"
   }

   if {$hab == 2} {
      return "$ctr mR"
   }
}

proc makeRangeText {hab ctr rng} {
   if {$hab == 0} {
      set bot [expr $ctr - $rng]
      set top [expr $ctr + $rng]
      return "$nsconst::gravLabel($bot)g\n to\n $nsconst::gravLabel($top)g"
   }

   if {$hab == 1} {
      set bot [expr ($ctr - $rng - 50) * 4]
      set top [expr ($ctr + $rng - 50) * 4]
      .tRaceWiz.fTemp.lRange configure -text "$bot C\n to\n $top C"
   }

   if {$hab == 2} {
      set bot [expr ($ctr - $rng)]
      set top [expr ($ctr + $rng)]
      .tRaceWiz.fRad.lRange configure -text "$bot mR\n to\n $top mR"
   }

   return ""
}

