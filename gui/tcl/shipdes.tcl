# ship design wizard
proc removeCmp {src tgt op type data rst} {
}
#   puts "Ship Removing $src $tgt $op $type $data $rst"
#   set srcWidget [regsub {..$} $src ""]
#   $srcWidget configure -image "" -text "Engine\nneeds 1" -width 11 -height 5 -dragenabled 0

proc shipDragInit {path x y top} {
   set type   NS_COMP
   set oplist [list copy {} move shift link control]
   set data   "Ned?"

   return [list $type $oplist $data]
}

proc shipDropCmd {tgt src x y op type data} {
   set slot [regsub {.*lBox} $tgt ""]

   set shipXML  [xml2list [newStars $::ns_design $::ns_getXML]]
   set mainXML  [lindex $shipXML 2]
   set slots    [findXMLbyTag "HULLSLOTS" $mainXML 2]
   set tuple    [lindex $slots 1]
   set allSlots [split $tuple ":"]

   if {$src eq ".tShipDes.fComp.lComponents.c"} {
      set curSlot  [lindex $allSlots $slot]
      set curCt    [lindex $curSlot 0]
      set max      [lindex $curSlot 2]
      set rem      [expr $max - $curCt]

      set ct 1
      if {$op eq "move"} {
         set ct 4
      } elseif {$op eq "link"} {
         set ct $rem
      }
      if {$ct > $rem} { set ct $rem }
      if {$ct == 0} { return }

      set cmpOff [regsub {cp-} $data ""]
      for {set i 0} {$i < $ct} {incr i} {
         set msg [newStars $::ns_design $::ns_addAtIndex $slot $cmpOff]
         if {$msg ne ""} {
            return 1
         }
      }
   } else {
      set slot2 [regsub {.*lBox} $src ""]
      if {$slot == $slot2} { return }

      set cmpOff [newStars $::ns_design $::ns_deleteAtIndex 0 $slot2]
      set msg [newStars $::ns_design $::ns_addAtIndex $slot $cmpOff]
      if {$msg ne ""} {
         # rejected, put it back
         newStars $::ns_design $::ns_addAtIndex $slot2 $cmpOff
         return 1
      }
   }

   set shipXML  [xml2list [newStars $::ns_design $::ns_getXML]]
   set mainXML  [lindex $shipXML 2]
   drawShip .tShipDes.fRightSide $mainXML

   return 1
}
#      puts "Dropping new cmp into slot $slot [newStars $::ns_component $cmpId $::ns_getName] ct=$ct"
#   $tgt configure -height 64 -width 64 -image [.tShipDes.fComp.lComponents itemcget $data -image] \
#       -dragenabled 1 -dragtype NS_COMP -dragevent 1 -dragendcmd removeCmp

proc shipDiscard {tgt src name op type data} {
   set srcWidget [regsub {..$} $src ""]

   if {$srcWidget eq ".tShipDes.fComp.lComponents"} return
   set slot [regsub {.*lBox} $src ""]

   set shipXML  [xml2list [newStars $::ns_design $::ns_getXML]]
   set mainXML  [lindex $shipXML 2]
   set slots    [findXMLbyTag "HULLSLOTS" $mainXML 2]
   set tuple    [lindex $slots 1]
   set allSlots [split $tuple ":"]
   set curSlot  [lindex $allSlots $slot]
   set curCt    [lindex $curSlot 0]

   set ct 1
   if {$op eq "move"} {
      set ct 4
   } elseif {$op eq "link"} {
      set ct $curCt
   }
   if {$ct > $curCt} { set ct $curCt }
   if {$ct == 0} { return }

   for {set i 0} {$i < $ct} {incr i} {
      newStars $::ns_design $::ns_deleteAtIndex 0 $slot
   }

   set shipXML  [xml2list [newStars $::ns_design $::ns_getXML]]
   set mainXML  [lindex $shipXML 2]
   drawShip .tShipDes.fRightSide $mainXML
}
#   puts "Discarding slot $slot $ct"

proc loadComponents {w} {
   for {set i 0} {$i < $::numComponents} {incr i} {
      set cmpIdx $::componentIdxList($i)
      set xml [xml2list [newStars $::ns_component $cmpIdx $::ns_getXML]]
      set mainXML [lindex $xml 2]
      set compName [lindex [findXMLbyTag "NAME" $mainXML 2] 1]
      set im $::cmpMap($compName)

      .tShipDes.fComp.lComponents insert end "cp-$cmpIdx" -image $im -text "               $compName"
   }
}

proc drawShip {f mainXML {all 1}} {
   eval "destroy [pack slaves $f.fCurShip]"

   set slots [findXMLbyTag "HULLSLOTS" $mainXML 2]
   set tuple [lindex $slots 1]
   set allSlots [split $tuple ":"]

   set curFrame ""
   for {set i 0} {$i < [llength $allSlots]} {incr i} {
      if {($i % 4) == 0} {
         if {$curFrame ne ""} {
            pack $curFrame -side top
         }
         set curFrame [frame $f.fCurShip.fRow$i]
      }

      set curBox [canvas $curFrame.lBox$i -width 64 -height 64]
      DropSite::register $curBox -dropcmd shipDropCmd -droptypes \
          [list NS_COMP [list copy {} move shift link control]]
      DragSite::register $curBox -draginitcmd shipDragInit -dragevent 1 -dragendcmd removeCmp

      pack $curBox -side left
      $curBox create rectangle 2 2 64 64 -outline black

      set curSlot [lindex $allSlots $i]
      set ct  [lindex $curSlot 0]
      set max [lindex $curSlot 2]
      if {$ct > 0} {
         set cpId [lindex $curSlot 1]
         $curBox create text  4 2 -anchor nw -width 60 \
            -text [newStars $::ns_component $cpId $::ns_getName]
      } else {
         set desc ""
         for {set j 3} {$j < [llength $curSlot]} {incr j} {
            set desc "$desc [lindex $curSlot $j]"
         }
         $curBox create text 4 2 -anchor nw -width 60 -text $desc
      }
      $curBox create text 15 50 -anchor w -text "$ct of $max"
   }
   pack $curFrame -side top

   if {!$all} {return}

   ### update bottom labels
   $f.fCost.l configure -text "Cost of one [lindex [findXMLbyTag "NAME" $mainXML 2] 1]"
   set minCost [lindex [findXMLbyTag "MINERALCOSTBASE" $mainXML 2] 1]
   $f.fCost.fL.fI.lC configure -text "[lindex $minCost 0] kt"
   $f.fCost.fL.fB.lC configure -text "[lindex $minCost 1] kt"
   $f.fCost.fL.fG.lC configure -text "[lindex $minCost 2] kt"
   $f.fCost.fL.fR.lC configure -text [lindex [findXMLbyTag "BASERESOURCECOST" $mainXML 2] 1]
   $f.fCost.fL.fM.lC configure -text [lindex [findXMLbyTag "MASS" $mainXML 2] 1]

   $f.fCost.fR.fM.lC configure -text "[lindex [findXMLbyTag "FUELCAPACITY" $mainXML 2] 1] mg"
   $f.fCost.fR.fA.lC configure -text "[lindex [findXMLbyTag "ARMOR" $mainXML 2] 1] dp"
   $f.fCost.fR.fS.lC configure -text "[lindex [findXMLbyTag "SHIELDMAX" $mainXML 2] 1] dp"

   set    cloakJam "[lindex [findXMLbyTag "CLOAK" $mainXML 2] 1]%/"
   append cloakJam "[lindex [findXMLbyTag "JAM" $mainXML 2] 1]%"
   $f.fCost.fR.fC.lC configure -text $cloakJam

   set    initMove "[lindex [findXMLbyTag "INITIATIVE" $mainXML 2] 1]/"
   append initMove "[lindex [findXMLbyTag "COMBATMOVE" $mainXML 2] 1]"
   $f.fCost.fR.fI.lC configure -text $initMove
}
#         $curBox create image 0 0 -image [.tShipDes.fComp.lComponents itemcget 0 -image]

proc doneDesign {} {
   pack forget .tShipDes.fRightSide.eShipName
   pack forget .tShipDes.fComp
   pack forget .tShipDes.fBotBut.bNewOk
   pack forget .tShipDes.fBotBut.bNewCancel
   pack forget .tShipDes.fBotBut.bNewHelp

   pack .tShipDes.fLeftSide -side left -anchor nw
   pack .tShipDes.fRightSide.cbShipList -side top -before .tShipDes.fRightSide.fCurShip
   pack .tShipDes.fBotBut.bDone -side left
   pack .tShipDes.fBotBut.bHelp -side right

   switch $shipDes::viewSel {
   0 {
     set hullCpIdx $::designMap([.tShipDes.fRightSide.cbShipList current])
     set shipXML [xml2list [newStars $::ns_planet $hullCpIdx $::ns_getXML]]
     }

   1 {
     set hullCpIdx $::hullMap([.tShipDes.fRightSide.cbShipList current])
     set shipXML [xml2list [newStars $::ns_hull $hullCpIdx $::ns_getXML]]
     }

   2 {
     set hullCpIdx $::enemyDesMap([.tShipDes.fRightSide.cbShipList current])
     set shipXML [xml2list [newStars $::ns_hull $hullCpIdx $::ns_getXML]]
     }
   }
   set mainXML [lindex $shipXML 2]

   drawShip .tShipDes.fRightSide $mainXML
}

proc copyDesign {} {
   switch $shipDes::viewSel {
   0 {
     set hullCpIdx $::designMap([.tShipDes.fRightSide.cbShipList current])
     }

   1 {
     set hullCpIdx $::hullMap([.tShipDes.fRightSide.cbShipList current])
     }

   2 {
     set hullCpIdx $::enemyDesMap([.tShipDes.fRightSide.cbShipList current])
     }
   }

   newStars $::ns_design $::ns_loadPre $shipDes::viewSel $hullCpIdx
   .tShipDes.fRightSide.eShipName delete 0 end
   .tShipDes.fRightSide.eShipName insert 0 [newStars $::ns_design $::ns_getName]

   pack forget .tShipDes.fLeftSide
   pack forget .tShipDes.fRightSide.cbShipList
   pack forget .tShipDes.fBotBut.bDone
   pack forget .tShipDes.fBotBut.bHelp

   pack .tShipDes.fRightSide.eShipName -side top -before .tShipDes.fRightSide.fCurShip
   pack .tShipDes.fComp -side left -anchor nw
   pack .tShipDes.fBotBut.bNewOk     -side left
   pack .tShipDes.fBotBut.bNewCancel -side left
   pack .tShipDes.fBotBut.bNewHelp   -side right
}
#   pack [Label .tShipDes.fRightSide.fCurShip.lDiscard -bd 1 -relief solid -dropenabled 1 \
#      -dropcmd shipDiscard -text "Discard" -width 20 -height 10 \
#      -droptypes [list NS_COMP [list copy {} move shift link control]]] -side bottom

proc selectDesign {wnd} {
   if {[$wnd current] eq ""} { return }

   switch $shipDes::viewSel {
   0 {
     set hullCpIdx $::designMap([.tShipDes.fRightSide.cbShipList current])
     set shipXML [xml2list [newStars $::ns_planet $hullCpIdx $::ns_getXML]]
     }

   1 {
     set hullCpIdx $::hullMap([.tShipDes.fRightSide.cbShipList current])
     set shipXML [xml2list [newStars $::ns_hull $hullCpIdx $::ns_getXML]]
     }

   2 {
     set hullCpIdx $::enemyDesMap([.tShipDes.fRightSide.cbShipList current])
     set shipXML [xml2list [newStars $::ns_planet $hullCpIdx $::ns_getXML]]
     }
   }

   set mainXML [lindex $shipXML 2]

   drawShip .tShipDes.fRightSide $mainXML
}

proc fillPlayerDesigns {} {
   eval "destroy [pack slaves .tShipDes.fRightSide.fCurShip]"
   set shipDes::shipCB ""

	set ship_des [list]
   for {set i 0} {$i < $::numDesigns} {incr i} {
		lappend ship_des [newStars $::ns_planet $::designMap($i) $::ns_getName]
   }
	.tShipDes.fRightSide.cbShipList configure -values $ship_des

   if {$::numDesigns > 0} {
      .tShipDes.fRightSide.cbShipList current 0

      set hullCpIdx $::designMap([.tShipDes.fRightSide.cbShipList current])
      set shipXML [xml2list [newStars $::ns_planet $hullCpIdx $::ns_getXML]]
      set mainXML [lindex $shipXML 2]

      drawShip .tShipDes.fRightSide $mainXML
   }
}

proc fillHullDesigns {} {
   eval "destroy [pack slaves .tShipDes.fRightSide.fCurShip]"
   set shipDes::shipCB ""

	set hull_list [list]
   for {set i 0} {$i < $::numHulls} {incr i} {
		lappend hull_list [newStars $::ns_hull $::hullMap($i) $::ns_getName]
   }
   .tShipDes.fRightSide.cbShipList configure -values $hull_list

   if {$::numHulls > 0} {
      .tShipDes.fRightSide.cbShipList current 0

      set hullCpIdx $::hullMap([.tShipDes.fRightSide.cbShipList current])
      set shipXML [xml2list [newStars $::ns_hull $hullCpIdx $::ns_getXML]]
      set mainXML [lindex $shipXML 2]

      drawShip .tShipDes.fRightSide $mainXML
   }
}

proc fillEnemyDesigns {} {
   eval "destroy [pack slaves .tShipDes.fRightSide.fCurShip]"
   set shipDes::shipCB ""

	set enemy_list [list]
   foreach i [lsort [array names ::enemyDesMap]] {
		lappend enemy_list [newStars $::ns_planet $::enemyDesMap($i) $::ns_getName]
   }
	.tShipDes.fRightSide.cbShipList configure -values $enemy_list

   if {$::numEnemyDesigns > 0} {
		.tShipDes.fRightSide.cbShipList current 0

      set hullCpIdx $::enemyDesMap([.tShipDes.fRightSide.cbShipList current])
      set shipXML [xml2list [newStars $::ns_planet $hullCpIdx $::ns_getXML]]
      set mainXML [lindex $shipXML 2]

      drawShip .tShipDes.fRightSide $mainXML
   }
}

proc handleViewSel {} {
   if {$shipDes::viewSelOld == 3} {
      pack forget .tShipDes.fComp

      pack .tShipDes.fRightSide -side right -anchor ne

      .tShipDes.fLeftSide.fBot.bCopy configure -state normal
      .tShipDes.fLeftSide.fBot.bDele configure -state normal
      .tShipDes.fLeftSide.fBot.bEdit configure -state normal
   }

   switch $shipDes::viewSel {
      0 fillPlayerDesigns
      1 fillHullDesigns
      2 fillEnemyDesigns
   }

   set shipDes::viewSelOld $shipDes::viewSel
}

proc handleViewSelCmp {} {
   pack forget .tShipDes.fRightSide

   pack .tShipDes.fComp -side right -anchor ne

   .tShipDes.fLeftSide.fBot.bCopy configure -state disabled
   .tShipDes.fLeftSide.fBot.bDele configure -state disabled
   .tShipDes.fLeftSide.fBot.bEdit configure -state disabled

   set shipDes::viewSelOld $shipDes::viewSel
}

proc shipDesigner {} {
   bind <F4> {}
   destroy  .tShipDes
   toplevel .tShipDes
   wm geometry .tShipDes 512x480
   bind .tShipDes <Destroy> {bind <F4> shipDesigner}
   bind .tShipDes <Escape> "destroy .tShipDes"

   wm title .tShipDes "Ship & Starbase Designer"

   frame .tShipDes.fComp
   pack [ListBox .tShipDes.fComp.lComponents -dragenabled 1 -dragevent 1 -width 26 \
       -dropenabled 1 -dropcmd shipDiscard -droptypes \
       [list NS_COMP [list copy {} move shift link control]]\
       -dragtype NS_COMP -deltay 64 -height 4 -yscrollcommand {.tShipDes.fComp.lCv set}] -side left
   pack [scrollbar .tShipDes.fComp.lCv -orient vertical -command {.tShipDes.fComp.lComponents yview}] -side left -expand 1 -fill y
   loadComponents 0

   namespace eval shipDes {
      set shipSel    0
      set viewSel    0
      set viewSelOld 0
      set shipCB     ""
   }
   frame .tShipDes.fLeftSide
   pack [labelframe  .tShipDes.fLeftSide.fTop -text "Design"] -side top -anchor nw
   pack [radiobutton .tShipDes.fLeftSide.fTop.bShips -text "Ships           " \
      -variable shipDes::shipSel -value 0] -side top -anchor nw
   pack [radiobutton .tShipDes.fLeftSide.fTop.bStarb -text "Starbases       " \
      -variable shipDes::shipSel -value 1] -side top

   pack [labelframe  .tShipDes.fLeftSide.fMid -text "View"] -side top
   pack [radiobutton .tShipDes.fLeftSide.fMid.bED -text "Existing Designs     " \
      -variable shipDes::viewSel -value 0 -command handleViewSel] -side top -anchor nw
   pack [radiobutton .tShipDes.fLeftSide.fMid.bAH -text "Available Hull Types " \
      -variable shipDes::viewSel -value 1 -command handleViewSel] -side top -anchor nw
   pack [radiobutton .tShipDes.fLeftSide.fMid.bEH -text "Enemy Hulls          " \
      -variable shipDes::viewSel -value 2 -command handleViewSel] -side top -anchor nw
   pack [radiobutton .tShipDes.fLeftSide.fMid.bCP -text "Components           " \
      -variable shipDes::viewSel -value 3 -command handleViewSelCmp] -side top -anchor nw

   pack [frame  .tShipDes.fLeftSide.fBot] -side top
   pack [button .tShipDes.fLeftSide.fBot.bCopy -text "Copy Selected Design" \
      -command copyDesign -width 19] -side top
   pack [button .tShipDes.fLeftSide.fBot.bDele -text "Delete Selected Design" -width 19] -side top
   pack [button .tShipDes.fLeftSide.fBot.bEdit -text "Edit Selected Design"   -width 19] -side top

   frame .tShipDes.fRightSide
   entry .tShipDes.fRightSide.eShipName
   pack [ttk::combobox .tShipDes.fRightSide.cbShipList \
      -textvariable shipDes::shipCB -state readonly] -side top
	bind .tShipDes.fRightSide.cbShipList <<ComboboxSelected>> {selectDesign %W}
   pack [frame .tShipDes.fRightSide.fCurShip] -side top
   pack [frame .tShipDes.fRightSide.fCost] -side bottom -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.l] -side top

   pack [frame .tShipDes.fRightSide.fCost.fL] -side left -expand 1 -fill x -anchor nw

   pack [frame .tShipDes.fRightSide.fCost.fL.fI] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fL.fI.lN -text "Ironium" -fg blue] -side left
   pack [label .tShipDes.fRightSide.fCost.fL.fI.lC -text "0 kt"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fL.fB] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fL.fB.lN -text "Boranium" -fg green4] -side left
   pack [label .tShipDes.fRightSide.fCost.fL.fB.lC -text "0 kt"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fL.fG] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fL.fG.lN -text "Germanium" -fg yellow4] -side left
   pack [label .tShipDes.fRightSide.fCost.fL.fG.lC -text "0 kt"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fL.fR] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fL.fR.lN -text "Resources"] -side left
   pack [label .tShipDes.fRightSide.fCost.fL.fR.lC -text "0"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fL.fM] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fL.fM.lN -text "Mass:"] -side left
   pack [label .tShipDes.fRightSide.fCost.fL.fM.lC -text "0 kt"] -side right


   pack [frame .tShipDes.fRightSide.fCost.fR] -side right -expand 1 -fill x -anchor ne

   pack [frame .tShipDes.fRightSide.fCost.fR.fM] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fR.fM.lN -text "Max Fuel:"] -side left
   pack [label .tShipDes.fRightSide.fCost.fR.fM.lC -text "0 mg"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fR.fA] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fR.fA.lN -text "Armor:"] -side left
   pack [label .tShipDes.fRightSide.fCost.fR.fA.lC -text "0 dp"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fR.fS] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fR.fS.lN -text "Shields:"] -side left
   pack [label .tShipDes.fRightSide.fCost.fR.fS.lC -text "0 dp"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fR.fC] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fR.fC.lN -text "Cloak/Jam:"] -side left
   pack [label .tShipDes.fRightSide.fCost.fR.fC.lC -text "0%/0%"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fR.fI] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fR.fI.lN -text "Initiative/Moves:"] -side left
   pack [label .tShipDes.fRightSide.fCost.fR.fI.lC -text "1/1"] -side right

   pack [frame .tShipDes.fRightSide.fCost.fR.fR] -side top -expand 1 -fill x
   pack [label .tShipDes.fRightSide.fCost.fR.fR.lN -text "Scanner Range:"] -side left
   pack [label .tShipDes.fRightSide.fCost.fR.fR.lC -text "0/0"] -side right


   fillPlayerDesigns

   pack [frame  .tShipDes.fBotBut] -side bottom -anchor se
   button .tShipDes.fBotBut.bNewOk -width 8 -text "Ok" -command {
      set shipName [.tShipDes.fRightSide.eShipName get]
      newStars $::ns_design $::ns_setName $shipName

      set msg [newStars $::ns_design $::ns_save]
      if {$msg eq ""} {

         fillPlayerDesigns

         if [winfo exists .tProd] {
            .tProd.lbAvail delete 0 end
            buildAvailList
         }

         .tShipDes.fRightSide.cbShipList current end
         set shipDes::viewSel 0
         doneDesign
      } else {
         tk_messageBox -message "Design rejected due to: $msg" -type ok -icon error -title "NStars"
      }
   }
   button .tShipDes.fBotBut.bNewCancel -width 8 -text "Cancel" -command doneDesign
   button .tShipDes.fBotBut.bNewHelp   -width 8 -text "Help"

   pack [button .tShipDes.fBotBut.bDone -text "Done" \
      -command "destroy .tShipDes" -width 8] -side left
   pack [button .tShipDes.fBotBut.bHelp -text "Help" -width 8] -side right -padx 5

   pack .tShipDes.fLeftSide  -side left  -anchor nw
   pack .tShipDes.fRightSide -side right -anchor ne -expand 1 -fill y
}
#   frame .tShipDes.fShip
#   pack [Label .tShipDes.fShip.lTgtE -bd 1 -relief solid -dropenabled 1 \
#      -dropcmd shipDropCmd -text "Engine\nneeds 1" -droptypes \
#      [list NS_COMP [list copy {} move shift link control]] -width 11 -height 5]
#   pack [Label .tShipDes.fShip.lDiscard -bd 1 -relief solid -dropenabled 1 \
#      -dropcmd shipDiscard -text "Discard" -width 20 -height 10 \
#      -droptypes [list NS_COMP [list copy {} move shift link control]]]
#   pack .tShipDes.fComp -side left
#   pack .tShipDes.fShip -side right

