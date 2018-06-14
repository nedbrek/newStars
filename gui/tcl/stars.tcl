#!/usr/bin/wish8.5
package require BWidget
package require tablelist
package require tdom

wm withdraw .

# check for starkit
set inStarkit 0
if {[namespace exists starkit]} {
	set inStarkit 1
} else {
	# all file references will be relative to starkit::topdir
	namespace eval starkit {
		set topdir [pwd]
	}
}

# all the Tcl sources we need
set externalSources {
	"cmpmap.tcl"
	"combobox.tcl"
	"draw.tcl"
	"libxml.tcl"
	"ngwiz.tcl"
	"nsCli.tcl"
	"nsconst.tcl"
	"racewiz.tcl"
	"shipdes.tcl"
	"stdlib.tcl"
}

foreach s $externalSources {
	if {$inStarkit} {
		# starkit checks for updates
		if {[file exists $s]} {
			file rename -force $s [file join $starkit::topdir $s]
		}
	}
	source [file join $starkit::topdir $s]
}

# all the non-Tcl files we need
set externalResources {
	"cp-e1.gif"
	"names.txt"
	"newStarsComponents.xml"
	"newStarsHulls.xml"
	"nstclgui.so"
	"planet.gif"
	"sampleNewGame.xml"
	"ship.gif"
	"view1.gif"
	"view4.gif"
}

if {$inStarkit} {
	# check for updates
	foreach r $externalResources {
		if {[file exists ./$r]} {
			file rename -force $r [file join $starkit::topdir $r]
		}
	}

	# if main file is updated
	if {[file exists ./stars.tcl]} {
		file rename -force stars.tcl [file join $starkit::topdir lib/app-stars/stars.tcl]
		tk_messageBox -message "Applyed update, please restart."
		exit
	}

	# mostly done
	package provide app-stars 1.0

	# provide for C++ (can't load files in starkit)
	file copy -force [file join $starkit::topdir newStarsComponents.xml] .
	file copy -force [file join $starkit::topdir newStarsHulls.xml] .
	file copy -force [file join $starkit::topdir names.txt] .
}

# load C++ support code
if {![info exists isExe]} {
	load [file join $starkit::topdir nstclgui.so]
}

# variables
#############################################################################
set resourceFileName ".newstarsrc"

set labelBoldFont [font create -weight bold]
font configure $labelBoldFont -size 8

set magnidx  0
set maglist  {.25 .38 .5 .75 1 1.25 1.5 2 4}

set shipImage [image create photo -file [file join $starkit::topdir ship.gif]]
set planImage [image create photo -file [file join $starkit::topdir planet.gif]]

set numPlanets 0; # set by C++ code

# current index into map (for next and prev)
set walkNum  0

namespace eval nsGui {
	set ofileDir [pwd]
	set filename ""

	# player messages
	set msgNum  1
	set msgTxt  "You have received one battle recording this year."

	# walk planets or fleets
	set walkPlanet    1
	set walkPlanetOld  1

	set internalCbTask 0
	set curPlayRel     1

	set shftDn      0
	set ctrlDn      0

	set viewMode 1
	set vb1 [image create photo -file [file join $starkit::topdir view1.gif]]
	set vb4 [image create photo -file [file join $starkit::topdir view4.gif]]

	set techName(0) "Energy"
	set techName(1) "Weapons"
	set techName(2) "Propulsion"
	set techName(3) "Construction"
	set techName(4) "Electronics"
	set techName(5) "Biotechnology"

	set turnRoot ""

	array set prevFiles ""

	set xferAmt(0) 0
	set xferAmt(1) 0
	set xferAmt(2) 0
	set xferAmt(3) 0
	set xferAmt(4) 0

}
# namespace nsGui

# not ready yet
set paneFont [font create -family Times -size 8]

### handle resource file
if {![file exists [file join ~ $resourceFileName]]} {
	set f [open [file join ~ $resourceFileName] w]
} else {
	set f [open [file join ~ $resourceFileName]]
	eval [read $f]
}
close $f
unset f

# bottom right info pane (actually bottom left in large view)
namespace eval updateBottomRight {
	set oldWhat ""
	set oldId   ""

	set width 250
	set scale 5000.0

	set plusColor(0) "#0000FF"
	set plusColor(1) "#FF0000"
	set plusColor(2) "#00FF00"
	set plusColor(3) "#0000FF"
	set plusColor(4) "#00FF00"
	set plusColor(5) "#FFFF00"

	set oldLine ""
}

proc clearFleetDisplay {} {
	set ::ltrFuel  "Fuel 0 mg"
	set ::ltrCargo "Cargo 0 kT"

	set ::ltrIron  0
	set ::ltrBoron 0
	set ::ltrGerm  0
	set ::ltrCol   0
}

clearFleetDisplay
#############################################################################

trace add variable nsGui::walkPlanet write switchPlanFleet
trace add variable fleetSelect       write updateViewFleet2
trace add variable curResearch       write updateCurResearch
#############################################################################

# function library
#############################################
proc nop args {
}

# find the index into the allFleetMap (walkNum) for a given unique id
proc findFleetWalkFromId {fleetId} {
	foreach i [array names ::allFleetMap] {
		if {$fleetId == $::allFleetMap($i)} {
			return $i
		}
	}
}

# find the index into the planetMap (walkNum) for a given unique id
proc findPlanetWalkFromId {planetId} {
	for {set i 0} {$i < $::numPlanets} {incr i} {
		if {$planetId == $::planetMap($i)} {
			return $i
		}
	}
}

# find all canvas ids for a given minefield unique id
proc findMinefieldCanvasFromId {mfId} {
	set ret ""

	foreach i [array names ::planetMapId] {
		if {$mfId == $::planetMapId($i)} {
			lappend ret $i
		}
	}

	return $ret
}

#############################################
# update the bottom right pane with planet info
proc ubrPlanet {id} {
	set planXML [xml2list [newStars $::ns_planet $id $::ns_getXML]]
	set mainXML  [lindex $planXML 2]

	set reportAge [lindex [findXMLbyTag "YEARDATAGATHERED" $mainXML 2] 1]

	if {$reportAge != 0} {
		set delta [expr {$::year - $reportAge}]
		if {$delta == 0} {
			.tGui.fBottomRight.fPlanet.fRow2.lAge configure -text "Report is current"
		} else {
			.tGui.fBottomRight.fPlanet.fRow2.lAge configure -text "Report from year: $reportAge (-$delta)"
		}

		set cargoXML [findXMLbyTag "CARGOMANIFEST" $mainXML 2]
		set cargo_tuple [lindex $cargoXML 1]
		set col   [commify [expr [lindex $cargo_tuple 4] * 100]]
		.tGui.fBottomRight.fPlanet.fRow1.lPop configure -text "Population: $col"

		set habXML [findXMLbyTag "HAB" $mainXML 2]
		set tuple [lindex $habXML 1]
		for {set i 0} {$i < 3} {incr i} {
			.tGui.fBottomRight.fPlanet.fRow3.c$i delete plus

			set curEnv [lindex $tuple [expr $i + 3]]

			drawPlus .tGui.fBottomRight.fPlanet.fRow3.c$i [lindex $tuple $i] \
			      $curEnv $updateBottomRight::plusColor($i)

			.tGui.fBottomRight.fPlanet.fRow3.lVal$i configure -text [makeHabText $i $curEnv]
		}

		set concXML [findXMLbyTag "MINERALCONCENTRATIONS" $mainXML 2]
		set tuple [lindex $concXML 1]
		for {set i 3} {$i < 6} {incr i} {
			.tGui.fBottomRight.fPlanet.fRow3.c$i delete plus
			.tGui.fBottomRight.fPlanet.fRow3.c$i delete bar

			set offset [expr {$i-3}]
			set scaleX [expr {min(100, [lindex $tuple $offset]) / 100.0 * $updateBottomRight::width}]

			set coords [list]
			lappend coords [expr {$scaleX - 7}] 10
			lappend coords $scaleX 3
			lappend coords [expr {$scaleX + 7}] 10
			lappend coords $scaleX 17

			.tGui.fBottomRight.fPlanet.fRow3.c$i create polygon {*}$coords -fill $updateBottomRight::plusColor($i) -tags plus

			set cargoX [expr {[lindex $cargo_tuple $offset] / $updateBottomRight::scale * $updateBottomRight::width}]
			.tGui.fBottomRight.fPlanet.fRow3.c$i create rectangle 0 18 $cargoX 2 -fill $updateBottomRight::plusColor($i) -tags bar
		}

		set hab    [newStars $::ns_planet $id $::ns_getHab 0]
		set maxHab [newStars $::ns_planet $id $::ns_getHab 1]
		.tGui.fBottomRight.fPlanet.fRow1.lVal configure -text "Value: $hab% ($maxHab%)"
	} else {
		.tGui.fBottomRight.fPlanet.fRow1.lVal configure -text "Unexplored"
		.tGui.fBottomRight.fPlanet.fRow1.lPop configure -text ""
      .tGui.fBottomRight.fPlanet.fRow2.lAge configure -text ""
		for {set i 0} {$i < 3} {incr i} {
			.tGui.fBottomRight.fPlanet.fRow3.c$i delete plus
			.tGui.fBottomRight.fPlanet.fRow3.lVal$i configure -text ""
		}
		for {set i 3} {$i < 6} {incr i} {
			.tGui.fBottomRight.fPlanet.fRow3.c$i delete plus
			.tGui.fBottomRight.fPlanet.fRow3.c$i delete bar
		}
   }

   pack .tGui.fBottomRight.fPlanet -side top -expand 1 -fill x
}

# update the display when the user clicks something in the map
proc updateBottomRight {what id} {
	# deselect what was selected
	if {$updateBottomRight::oldWhat eq "MINEFIELD"} {
		foreach canId [findMinefieldCanvasFromId $updateBottomRight::oldId] {
			.tGui.cScannerPane itemconfigure $canId -fill red
		}
	} elseif {($updateBottomRight::oldWhat eq "FLEET" ||
				  $updateBottomRight::oldWhat eq "EFLEET") && \
				 $updateBottomRight::oldLine ne ""} {
		.tGui.cScannerPane delete $updateBottomRight::oldLine
		set updateBottomRight::oldLine ""
	}

	set updateBottomRight::oldWhat $what
	set updateBottomRight::oldId   $id

	# remove old stuff
	eval [list pack forget {*}[pack slaves .tGui.fBottomRight]]

	# calculate distance from current/walk obj to bottom right obj
	if {$nsGui::walkPlanet} {
		set pid $::planetMap($::walkNum)
	} else {
		set pid $::allFleetMap($::walkNum)
	}

	set prx [newStars $::ns_planet $pid $::ns_getX]
	set pry [newStars $::ns_planet $pid $::ns_getY]
	set brx [newStars $::ns_planet $id  $::ns_getX]
	set bry [newStars $::ns_planet $id  $::ns_getY]
	set dist [expr sqrt(pow($bry - $pry, 2) + pow($brx - $prx, 2))]
	set dist [format "%0.2f" $dist]
	set walkName [newStars $::ns_planet $pid $::ns_getName]

	set name [newStars $::ns_planet $id $::ns_getName]

	# update map status
	.tGui.fLoc.lId configure -text "ID #$id"
	.tGui.fLoc.lX  configure -text "X: $brx"
	.tGui.fLoc.lY  configure -text "Y: $bry"
	.tGui.fLoc.lN  configure -text $name
	.tGui.fLoc.lDistTxt configure -text "$dist light years from $walkName"

	# start filling bottom right pane
	.tGui.fBottomRight.lT configure -text "$name Summary"
	pack .tGui.fBottomRight.lT -expand 1 -fill x -side top

	switch $what {
		PLANET    { ubrPlanet $id }

      MINEFIELD {
         foreach canId [findMinefieldCanvasFromId $id] {
            .tGui.cScannerPane itemconfigure $canId -fill #FF8080
         }
         pack .tGui.fBottomRight.lPic -side left
      }

      EFLEET -
      FLEET {
			# project course based on last known direction
         set xml [newStars $::ns_planet $id $::ns_getXML]
         set doc [dom parse $xml]

         set node [$doc documentElement root]
         set dirnode [lindex [$node getElementsByTagName LASTDIRECTION] 0]
         set dirnode2 [$dirnode firstChild]
         set dir [$dirnode2 nodeValue]

         set pi [expr atan(1.0) * 4]
         set t [expr $dir / 2147483647.0 * $pi]
         set mag [lindex $::maglist $::magnidx]
         set x1 [expr 100 * cos($t) + $brx * $mag]
         set y1 [expr 100 * sin($t) + $bry * $mag]
         set x2 [expr 100 * cos($t - $pi) + $brx * $mag]
         set y2 [expr 100 * sin($t - $pi) + $bry * $mag]
         set updateBottomRight::oldLine [.tGui.cScannerPane create line \
             $x1 $y1 $x2 $y2 -fill blue]
         .tGui.cScannerPane lower $updateBottomRight::oldLine inspaceobj

         $doc delete


         .tGui.fBottomRight.lPic configure -image $::shipImage
         pack .tGui.fBottomRight.lPic -side left
      }

      PACKET {
			# Ned packet image
         pack .tGui.fBottomRight.lPic -side left
      }
   }
}

# return fuel in 'fleetId'
proc getFuel {fleetId} {
   set fleetXML [xml2list [newStars $::ns_planet $fleetId  $::ns_getXML]]
   set mainXML  [lindex $fleetXML 2]
   set cargoXML [findXMLbyTag "CARGOMANIFEST" $mainXML 2]

   if {[lindex $cargoXML 0] ne "#text"} { return }

   return [lindex [lindex $cargoXML 1] 3]
}

# update the left-top-right with the cargo for 'fleetId'
proc calcCargo {fleetId} {
   set fleetXML [xml2list [newStars $::ns_planet $fleetId  $::ns_getXML]]
   set mainXML  [lindex $fleetXML 2]
   set cargoXML [findXMLbyTag "CARGOMANIFEST" $mainXML 2]

   if {[lindex $cargoXML 0] ne "#text"} { return }

   set tuple [lindex $cargoXML 1]

   set iron  [lindex $tuple 0]
   set boron [lindex $tuple 1]
   set germ  [lindex $tuple 2]
   set col   [lindex $tuple 4]

   set ::ltrCargo "Cargo [expr $iron + $boron + $germ + $col] kT"

   if {$nsGui::walkPlanet} { return }

   set ::ltrIron  $iron
   set ::ltrBoron $boron
   set ::ltrGerm  $germ
   set ::ltrCol   $col
}

# update the planet display values
proc calcPlanet {planetId} {
   if {!$nsGui::walkPlanet} { return }

   set planXML [xml2list [newStars $::ns_planet $planetId  $::ns_getXML]]
   set mainXML  [lindex $planXML 2]

   set cargoXML [findXMLbyTag "CARGOMANIFEST" $mainXML 2]
   if {[lindex $cargoXML 0] ne "#text"} { return }

   set tuple [lindex $cargoXML 1]

   set iron  [commify [lindex $tuple 0]]
   set boron [commify [lindex $tuple 1]]
   set germ  [commify [lindex $tuple 2]]
   set col   [commify [expr [lindex $tuple 4] * 100]]

   .tGui.fMinerals.fIron.lVal  configure -text "$iron kt"
   .tGui.fMinerals.fBoron.lVal configure -text "$boron kt"
   .tGui.fMinerals.fGerm.lVal  configure -text "$germ kt"

   .tGui.fPStatus.fPop.lV configure -text "$col"

   set rsc [lindex [findXMLbyTag "RESOURCESGENERATED" $mainXML 2] 1]
   .tGui.fPStatus.fRsc.lV configure -text "$rsc of $rsc"

   set infraXML [findXMLbyTag "INFRASTRUCTURE" $mainXML 2]
   if {[lindex $infraXML 0] ne "#text"} { return }

   set tuple    [lindex $infraXML 1]
   set tupleMax [lindex [findXMLbyTag "MAXINFRASTRUCTURE" $mainXML 2] 1]

   .tGui.fMinerals.fInfra.fMines.lText configure -text "[lindex $tuple 0] of [lindex $tupleMax 0]"
   .tGui.fMinerals.fInfra.fFactories.lText configure -text "[lindex $tuple 1] of [lindex $tupleMax 1]"
   .tGui.fPStatus.fDef.lV configure -text "[lindex $tuple 2] of [lindex $tupleMax 2]"

   set hasScanner  [lindex $tuple 3]
   if {!$hasScanner} {
      .tGui.fPStatus.fScan.lV configure -text "None"
      .tGui.fPStatus.fSRan.lV configure -text "0/0"
   } else {
      .tGui.fPStatus.fScan.lV configure -text [newStars $::ns_planet $planetId $::ns_getScan 1]
      .tGui.fPStatus.fSRan.lV configure -text [join \
          [newStars $::ns_planet $planetId $::ns_getScan 0] "/"]
   }
}

proc updateViewFleet2 {n1 n2 op} {
   if {$n1 ne "fleetSelect"} { return }
   if {$n2 ne ""} { return }
   if {$op ne "write"} { return }

   if {!$nsGui::walkPlanet} { return }

   set planetId $::planetMap($::walkNum)
   set numFleetsHere [newStars $::ns_planet $planetId $::ns_getNumFleets]

   for {set i 0} {$i < $numFleetsHere} {incr i} {
      set fleetId [newStars $::ns_planet $planetId $::ns_getFleetIdFromIndex $i]
      if {[newStars $::ns_planet $fleetId $::ns_getName] eq $::fleetSelect} {
			set ::ltrFuel "Fuel [getFuel $fleetId] mg"
         calcCargo $fleetId
         return
      }
   }
}

proc updateViewFleet {path val} {
   if {!$nsGui::walkPlanet} { return }
   set planNum $::walkNum

   set planetId $::planetMap($planNum)
   set numFleetsHere [newStars $::ns_planet $planetId $::ns_getNumFleets]

   set ::fleetSelect $val
   for {set i 0} {$i < $numFleetsHere} {incr i} {
      set fleetId [newStars $::ns_planet $planetId $::ns_getFleetIdFromIndex $i]
      if {[newStars $::ns_planet $fleetId $::ns_getName] eq $::fleetSelect} {
         set ::ltrFuel "Fuel [getFuel $fleetId] mg"
         calcCargo $fleetId
         return
      }
   }
}

proc drawScanners {pane magnify} {
	# do non-pen before pen
   for {set pnum 0} {$pnum < $::numPlanets} {incr pnum} {
      set pid $::planetMap($pnum)

      paintScan $pane $magnify $pid 0
   }

   foreach fnum [array names ::allFleetMap] {
      set fid $::allFleetMap($fnum)

      paintScan $pane $magnify $fid 0
   }

   for {set pnum 0} {$pnum < $::numPlanets} {incr pnum} {
      set pid $::planetMap($pnum)

      paintScan $pane $magnify $pid 1
   }

   foreach fnum [array names ::allFleetMap] {
      set fid $::allFleetMap($fnum)

      paintScan $pane $magnify $fid 1
   }
}

# build the production queue window
proc drawTprod {} {
   toplevel .tProd
   wm title .tProd "Production Queue"
   bind .tProd <Escape> "destroy .tProd"

   pack [listbox  .tProd.lbAvail -height 20 -selectmode single -width 30 -exportselection 0] -side left
   pack [listbox  .tProd.lbQueue -height 20 -selectmode single -width 30 -exportselection 0] -side right
   pack [button   .tProd.bAdd    -text "Add ->"    -underline 0 -width 15 -command "addBuildOrder 1"] -side top -pady 20
   pack [button   .tProd.bRemove -text "<- Remove" -underline 3 -width 15 -command "deleteOrder 1"] -side top -pady 20
   pack [button   .tProd.bItemUp -text "Item Up"   -underline 5 -width 15 -command "orderMv -1"] -side top -pady 20
   pack [button   .tProd.bItemDn -text "Item Down" -underline 5 -width 15 -command "orderMv 1"] -side top -pady 20

   bind .tProd.bAdd    <Shift-1>         "addBuildOrder   9"
   bind .tProd.bRemove <Shift-1>         "deleteOrder     10; break"
   bind .tProd.bAdd    <Control-1>       "addBuildOrder  99"
   bind .tProd.bRemove <Control-1>       "deleteOrder   100; break"
   bind .tProd.bAdd    <Shift-Control-1> "addBuildOrder 999"
   bind .tProd.bRemove <Shift-Control-1> "deleteOrder  1000; break"
   bind .tProd <Key-Delete>              "deleteOrder   all"
}

# set the speed for the current fleet, current order
# (triggered by changes in the spinner)
proc fleetSpeed {} {
   set fid $::allFleetMap($::walkNum)
   set oid [expr [.tGui.fWaypoints.lbWaypoints curselection] - 1]
   newStars $::ns_planet $fid $::ns_speed $oid [.tGui.fWayDetails.fWarp.s get]
}

# adjust the split resource 'row' by 'val'
# (checks for underflow, but not overflow)
proc adjustSplitRes {row val} {
	set left [.tSplitter.fTop.fRow$row.lLC cget -text]
	set rght [.tSplitter.fTop.fRow$row.lRC cget -text]

	# left goes down by val
	# right goes up by val
	if {$left < $val} {return}
	if {$rght < -$val} {return}

	set left [expr $left - $val]
	set rght [expr $rght + $val]

	lset ::splitRes $row 0 $left
	lset ::splitRes $row 1 $rght

	.tSplitter.fTop.fRow$row.lLC configure -text $left
	.tSplitter.fTop.fRow$row.lRC configure -text $rght
}

# create Split Fleet window
proc fleetSplit {} {
   set fid $::allFleetMap($::walkNum)
	set numDesigns [newStars $::ns_planet $fid $::ns_design]
	if {$numDesigns == 0} { return }

	toplevel .tSplitter
	wm title .tSplitter "Split Fleet"

	set ::splitRes ""

	pack [frame .tSplitter.fTop] -side top
	for {set i 0} {$i < $numDesigns} {incr i} {
		pack [frame .tSplitter.fTop.fRow$i] -side top

		set shipName [newStars $::ns_planet $fid $::ns_design $i $::ns_getName]
		pack [label .tSplitter.fTop.fRow$i.lL -text $shipName] -side left

		set shipCt [newStars $::ns_planet $fid $::ns_design $i $::ns_getNumFleets]
		pack [label .tSplitter.fTop.fRow$i.lLC -text $shipCt] -side left

		pack [button .tSplitter.fTop.fRow$i.bL -text "<" -command "adjustSplitRes $i -1"] -side left
		pack [button .tSplitter.fTop.fRow$i.bR -text ">" -command "adjustSplitRes $i  1"] -side left

		pack [label .tSplitter.fTop.fRow$i.lRC -text 0] -side left

		lappend ::splitRes [list $shipCt 0]
	}
	pack [frame  .tSplitter.fBot] -side top
	pack [button .tSplitter.fBot.bOk -text "Ok" -command doSplit] -side left
	pack [button .tSplitter.fBot.bCl -text "Cancel" -command "destroy .tSplitter"] -side left
}

# handle splitting a fleet (called on confirmation of Split Fleet window)
proc doSplit {} {
	# did anything actually change?
	set found 0
	foreach tuple $::splitRes {
		set rght [lindex $tuple 1]
		if {$rght != 0} {
			set found 1
			break
		}
	}
	if {!$found} { return }

   set fid $::allFleetMap($::walkNum)

	# create a new fleet for the split out ships
   set newFleetId [newStars $::ns_planet $fid $::ns_generate]

	# handle the case where all ships move over
	set found 0

	set row 0
	foreach tuple $::splitRes {
		set left [lindex $tuple 0]
		set rght [lindex $tuple 1]

		if {$rght == 0} {
			incr row
			continue
		}
		if {$left != 0} {
			# some ships left behind
			set found 1
		}

   	newStars $::ns_planet $newFleetId $::ns_addAtIndex $row $::ns_addAtIndex $fid $rght

		incr row
	}
	if {!$found} { set fid $newFleetId }

	destroy .tSplitter

	# refresh display
	set ::walkNum 0
	set nsGui::walkPlanet 1
	set ::walkNum [findFleetWalkFromId $fid]
	set nsGui::walkPlanet 0
}

# return the fid of the fleet in the "Other Fleets Here" combox
proc findOtherFid {} {
	set idx [.tGui.fOtherFleet.cb curselection]
	set fid $::allFleetMap($::walkNum)

   set nf [newStars $::ns_planet $fid $::ns_getNumFleets]
	for {set i 0} {$i < $nf} {incr i} {
		set f [newStars $::ns_planet $fid $::ns_getFleetIdFromIndex $i]
		if {$f == $fid} { break }
	}

	if {$idx >= $i} {
		incr idx
	}

	set f [newStars $::ns_planet $fid $::ns_getFleetIdFromIndex $idx]
	return $f
}

# set the current fleet displayed to the fleet in the
# "Other Fleets Here" dropdown
proc gotoOther {} {
	set f [findOtherFid]

	set ::walkNum [findFleetWalkFromId $f]
	updateFleet
}

proc makeDesList {fid} {
	set curDes ""
	set numDesigns [newStars $::ns_planet $fid $::ns_design]
	for {set i 0} {$i < $numDesigns} {incr i} {
		set shipName [newStars $::ns_planet $fid $::ns_design $i $::ns_getName]
		lappend curDes $shipName
	}
	return $curDes
}

# proceed with the merge (result of confirming "Merge Fleets")
proc doMerge {} {
   set fid $::allFleetMap($::walkNum)
   set newFleetId [findOtherFid]

	set row 0
	foreach tuple $::mergeRes {
		set curIdx [lindex $tuple 0]
		set othIdx [lindex $tuple 1]
		set curCt  [lindex $tuple 2]
		set othCt  [lindex $tuple 3]

		if {$curIdx == -1} {
			# new to fid, move from newfleet
	   	newStars $::ns_planet $fid $::ns_addAtIndex $othIdx $::ns_addAtIndex $newFleetId $curCt
		} elseif {$othIdx == -1 } {
			# new to newfid, move from fid
	   	newStars $::ns_planet $newFleetId $::ns_addAtIndex $curIdx $::ns_addAtIndex $fid $othCt
		} else {
			# common to both, figure delta
			set shipCt [newStars $::ns_planet $fid $::ns_design $curIdx $::ns_getNumFleets]
			if {$shipCt < $curCt} {
				# need to move some in to fid
				set delta [expr $curCt - $shipCt]
		   	newStars $::ns_planet $fid $::ns_addAtIndex $othIdx $::ns_addAtIndex $newFleetId $delta
			} elseif {$shipCt > $curCt} {
				# need to move some out of fid
				set delta [expr $shipCt - $curCt]
		   	newStars $::ns_planet $newFleetId $::ns_addAtIndex $curIdx $::ns_addAtIndex $fid $delta
			}
			#else, same - do nothing
		}

		incr row
	}

	destroy .tMerge
	set ::walkNum 0
	set nsGui::walkPlanet 1
	set ::walkNum [findFleetWalkFromId $fid]
	set nsGui::walkPlanet 0
}

proc adjustMergeRes {row val} {
	set left [.tMerge.fTop.fRow$row.lLC cget -text]
	set rght [.tMerge.fTop.fRow$row.lRC cget -text]

	if {$left < $val} {return}
	if {$rght < -$val} {return}

	set left [expr $left - $val]
	set rght [expr $rght + $val]

	lset ::mergeRes $row 2 $left
	lset ::mergeRes $row 3 $rght

	.tMerge.fTop.fRow$row.lLC configure -text $left
	.tMerge.fTop.fRow$row.lRC configure -text $rght
}

# create the "Merge Fleets" window
proc fleetToFleetMerge {} {
	toplevel .tMerge
	wm title .tMerge "Merge Fleets"

   set fid $::allFleetMap($::walkNum)
	set curDes [makeDesList $fid]

	set f [findOtherFid]
	set othDes [makeDesList $f]

	set allDes [lunion $curDes $othDes]

	set ::mergeRes ""

	pack [frame .tMerge.fTop] -side top
	set i 0
	foreach shipName $allDes {
		pack [frame .tMerge.fTop.fRow$i] -side top

		pack [label .tMerge.fTop.fRow$i.lL -text $shipName] -side left

		set curIdx [lsearch -exact $curDes $shipName]
		set shipCt 0
		if {$curIdx != -1} {
			set shipCt [newStars $::ns_planet $fid $::ns_design $curIdx $::ns_getNumFleets]
		}
		pack [label .tMerge.fTop.fRow$i.lLC -text $shipCt] -side left

		pack [button .tMerge.fTop.fRow$i.bL -text "<" -command "adjustMergeRes $i -1"] -side left
		pack [button .tMerge.fTop.fRow$i.bR -text ">" -command "adjustMergeRes $i  1"] -side left

		set othCt 0
		set othIdx [lsearch -exact $othDes $shipName]
		if {$othIdx != -1} {
			set othCt [newStars $::ns_planet $f $::ns_design $othIdx $::ns_getNumFleets]
		}
		pack [label .tMerge.fTop.fRow$i.lRC -text $othCt] -side left

		lappend ::mergeRes [list $curIdx $othIdx $shipCt $othCt]

		incr i
	}
	pack [frame  .tMerge.fBot] -side top
	pack [button .tMerge.fBot.bOk -text "Ok" -command doMerge] -side left
	pack [button .tMerge.fBot.bCl -text "Cancel" -command "destroy .tMerge"] -side left
}

# helper for the cargo xfer gui
proc xfer {idx amt} {
	set v1 [[join [list .tCargoXfer.l $idx V1] ""] cget -text]
	set v1 [expr $v1 + $amt]

	set v2 [[join [list .tCargoXfer.l $idx V2] ""] cget -text]
	set v2 [expr $v2 - $amt]

	if {$v1 < 0 || $v2 < 0} return

	set nsGui::xferAmt($idx) [expr $nsGui::xferAmt($idx) - $amt]

	[join [list .tCargoXfer.l $idx V1] ""] configure -text $v1
	[join [list .tCargoXfer.l $idx V2] ""] configure -text $v2
}

# given an XML index, return the code index
proc remapCargoXMLtoCode {i} {
	switch $i {
	0 {return 1}
	1 {return 2}
	2 {return 3}
	3 {return 4}
	4 {return 0}
	}
}

# transfer cargo from currently selected fleet to the one in the
# "Other Fleets" combox
proc cargoXfer {} {
   set fid $::allFleetMap($::walkNum)
	set fid2 [findOtherFid]

	set t [toplevel .tCargoXfer]
	grid [label $t.lF1 -text [newStars $::ns_planet $fid  $::ns_getName] -wraplength 60] \
	   -row 0 -column 0
	grid [label $t.lF2 -text [newStars $::ns_planet $fid2 $::ns_getName] -wraplength 60] \
	   -row 0 -column 1 -columnspan 4

	grid [label  $t.lIronT  -text "Ironium" -fg blue]             -row 1 -column 0
	grid [label  $t.l1V1 -text 0]                                 -row 1 -column 1
	grid [button $t.bIronAdd -text "<" -command [list xfer 1  1]] -row 1 -column 2
	grid [button $t.bIronSub -text ">" -command [list xfer 1 -1]] -row 1 -column 3
	grid [label  $t.l1V2 -text 0]                                 -row 1 -column 4

	grid [label  $t.lBoronT -text "Boranium" -fg green4]           -row 2 -column 0
	grid [label  $t.l3V1 -text 0]                                  -row 2 -column 1
	grid [button $t.bBoronAdd -text "<" -command [list xfer 3  1]] -row 2 -column 2
	grid [button $t.bBoronSub -text ">" -command [list xfer 3 -1]] -row 2 -column 3
	grid [label  $t.l3V2 -text 0]                                  -row 2 -column 4

	grid [label  $t.lGermT -text "Germanium" -fg yellow4]         -row 3 -column 0
	grid [label  $t.l2V1 -text 0]                                 -row 3 -column 1
	grid [button $t.bGermAdd -text "<" -command [list xfer 2  1]] -row 3 -column 2
	grid [button $t.bGermSub -text ">" -command [list xfer 2 -1]] -row 3 -column 3
	grid [label  $t.l2V2 -text 0]                                 -row 3 -column 4

	grid [label  $t.lColT -text "Colonists" -fg #808080]         -row 4 -column 0
	grid [label  $t.l0V1 -text 0]                                -row 4 -column 1
	grid [button $t.bColAdd -text "<" -command [list xfer 0  1]] -row 4 -column 2
	grid [button $t.bColSub -text ">" -command [list xfer 0 -1]] -row 4 -column 3
	grid [label  $t.l0V2 -text 0]                                -row 4 -column 4

	grid [label  $t.lFuelT -text "Fuel" -fg #800000]              -row 5 -column 0
	grid [label  $t.l4V1 -text 0]                                 -row 5 -column 1
	grid [button $t.bFuelAdd -text "<" -command [list xfer 4  1]] -row 5 -column 2
	grid [button $t.bFuelSub -text ">" -command [list xfer 4 -1]] -row 5 -column 3
	grid [label  $t.l4V2 -text 0]                                 -row 5 -column 4

   set fleetXML [xml2list [newStars $::ns_planet $fid $::ns_getXML]]
   set mainXML  [lindex $fleetXML 2]
   set cargoXML [findXMLbyTag "CARGOMANIFEST" $mainXML 2]
   set fc1 [lindex $cargoXML 1]

   set fleetXML [xml2list [newStars $::ns_planet $fid2 $::ns_getXML]]
   set mainXML  [lindex $fleetXML 2]
   set cargoXML [findXMLbyTag "CARGOMANIFEST" $mainXML 2]
   set fc2 [lindex $cargoXML 1]

	for {set i 0} {$i < 5} {incr i} {
		set nsGui::xferAmt($i) 0
		[join [list $t.l [remapCargoXMLtoCode $i] V1] ""] configure -text [lindex $fc1 $i]
		[join [list $t.l [remapCargoXMLtoCode $i] V2] ""] configure -text [lindex $fc2 $i]
	}

	tkwait window $t

	for {set i 0} {$i < 5} {incr i} {
		newStars $::ns_planet $fid  $::ns_orderTransport $fid2 $i $nsGui::xferAmt($i)
	}
	updateFleet
}

# switch the gui from displaying a planet to displaying a fleet
proc switchPlanFleet {n1 n2 o} {
   if {$nsGui::walkPlanet == $nsGui::walkPlanetOld} { return }

   clearFleetDisplay

   if {$nsGui::walkPlanet} {
      pack forget .tGui.pfWalker.bRen .tGui.fFleetLocation .tGui.fWayDetails .tGui.fWaypoints \
        .tGui.fWayPtTask .tGui.fFleetComp .tGui.fOtherFleet .tGui.fLeftTopRight.fIron \
        .tGui.fLeftTopRight.fBoron .tGui.fLeftTopRight.fGerm .tGui.fLeftTopRight.fCol

      .tGui.pfWalker.lPic configure -image $::planImage

      .tGui.lName configure -text "Fleets in Orbit"
      pack .tGui.fLeftTopRight.fButtons -side top -expand 1 -fill x
      pack .tGui.lbFleetList -in .tGui.fLeftRight -side top    -anchor ne -after .tGui.lName
      pack .tGui.fProduction -in .tGui.fLeftRight -side top -anchor n
      pack .tGui.fMinerals   -in .tGui.fLeftLeft  -side top -anchor nw -expand 1 -fill x
      pack .tGui.fPStatus    -in .tGui.fLeftLeft  -side top -anchor nw -expand 1 -fill x

      if [winfo exists .tProd] {
         wm state .tProd normal
      }

      .tGui.cScannerPane itemconfigure paths -fill blue
      updatePlanet $::walkNum
   } else {
      pack .tGui.pfWalker.bRen -side top -anchor n -pady 5

      .tGui.pfWalker.lPic configure -image $::shipImage
      .tGui.lName configure -text "Fuel & Cargo"

      pack forget .tGui.lbFleetList .tGui.fLeftTopRight.fButtons .tGui.fProduction \
        .tGui.fMinerals .tGui.fPStatus

      pack .tGui.fFleetLocation -in .tGui.fLeftLeft
      pack .tGui.fWaypoints     -in .tGui.fLeftLeft
      pack .tGui.fWayDetails    -in .tGui.fLeftLeft -expand 1 -fill x
      pack .tGui.fWayPtTask     -in .tGui.fLeftLeft

      if [winfo exists .tProd] {
         wm state .tProd withdrawn
      }

      pack .tGui.fLeftTopRight.fIron  -side top -expand 1 -fill x
      pack .tGui.fLeftTopRight.fBoron -side top -expand 1 -fill x
      pack .tGui.fLeftTopRight.fGerm  -side top -expand 1 -fill x
      pack .tGui.fLeftTopRight.fCol   -side top -expand 1 -fill x

      pack .tGui.fFleetComp -side top -in .tGui.fLeftRight
		pack .tGui.fOtherFleet -side top -in .tGui.fLeftRight

      updateFleet
   }

   set nsGui::walkPlanetOld $nsGui::walkPlanet
}

proc getFleetListAtPlanet {planetId} {
   set fleetList ""

   set numFleetsHere [newStars $::ns_planet $planetId $::ns_getNumFleets]

   for {set i 0} {$i < $numFleetsHere} {incr i} {
      set fleetId [newStars $::ns_planet $planetId $::ns_getFleetIdFromIndex $i]
      lappend fleetList [newStars $::ns_planet $fleetId $::ns_getName]
   }

   return $fleetList
}

proc drawProdItem {lb pid i sel} {
   set qitem [newStars $::ns_planet $pid $::ns_getOrderNameFromIndex $i]
   set autobuild [lindex $qitem 0]
   set qitem [lrange $qitem 1 end]

   $lb insert $sel $qitem

   if {$autobuild eq "A"} {
      $lb itemconfigure end -background green
   }
}

proc fillProdQueue {lb planetId} {
   for {set i 0} {$i < [newStars $::ns_planet $planetId $::ns_getNumOrders]} {incr i} {
      drawProdItem $lb $planetId $i end
   }
   $lb insert end "---End of Queue---"
}

proc updatePlanet {planNum} {
   global planetMap

   set planetId $planetMap($planNum)

   .tGui.pfWalker.lName configure -text [newStars $::ns_planet $planetId $::ns_getName]

   set numFleetsHere [newStars $::ns_planet $planetId $::ns_getNumFleets]
   if {$numFleetsHere > 0} {
      set fleetId  [newStars $::ns_planet $planetId $::ns_getFleetIdFromIndex 0]
      set ::ltrFuel "Fuel [getFuel $fleetId] mg"
      calcCargo $fleetId
   } else {
      clearFleetDisplay
   }

   set fleetList [getFleetListAtPlanet $planetId]

   .tGui.lbFleetList list delete 0 end

   if {$fleetList eq ""} {
      .tGui.lbFleetList list insert end {<none>}
      set ::fleetSelect {<none>}
      .tGui.fLeftTopRight.fButtons.bGoto configure -state disabled
   } else {
      eval [subst ".tGui.lbFleetList list insert end $fleetList"]
      set ::fleetSelect [lindex $fleetList 0]
      .tGui.fLeftTopRight.fButtons.bGoto configure -state normal
   }

   if [winfo exists .tProd.lbQueue] {
      .tProd.lbQueue delete 0 end
      fillProdQueue .tProd.lbQueue $planetId
      .tProd.lbQueue selection set end
   }

   .tGui.fProduction.lbPlanetQueue delete 0 end
   fillProdQueue .tGui.fProduction.lbPlanetQueue $planetId
   .tGui.fProduction.lbPlanetQueue selection set end

   calcPlanet $planetId
   updateBottomRight "PLANET" $planetId
}

proc fillShips {fleetId} {
   .tGui.fFleetComp.lb delete 0 end

   set totalCt 0

   set doc [dom parse [newStars $::ns_planet $fleetId $::ns_getXML]]
   set node [$doc documentElement root]
   set shipNodes [$node getElementsByTagName HULL]
   foreach sn $shipNodes {
      set nm [[[$sn getElementsByTagName NAME] firstChild] nodeValue]
      set ct [[[$sn getElementsByTagName QUANTITY] firstChild] nodeValue]

      .tGui.fFleetComp.lb insert end [list $nm $ct]

      incr totalCt $ct
   }

   if {$totalCt > 1} {
      .tGui.fFleetComp.fButtons.bSplit  configure -state normal
      .tGui.fFleetComp.fButtons.bSplall configure -state normal
   } else {
      .tGui.fFleetComp.fButtons.bSplit  configure -state disabled
      .tGui.fFleetComp.fButtons.bSplall configure -state disabled
   }

   $doc delete
}

proc fillOther {fid} {
   .tGui.fOtherFleet.cb list delete 0 end

   set nf [newStars $::ns_planet $fid $::ns_getNumFleets]
	for {set i 0} {$i < $nf} {incr i} {
	   set f [newStars $::ns_planet $fid $::ns_getFleetIdFromIndex $i]
		if {$f == $fid} { continue }
		set name [newStars $::ns_planet $f $::ns_getName]
		.tGui.fOtherFleet.cb list insert end $name
	}
	.tGui.fOtherFleet.cb select 0
}

# update the gui to represent the current fleet (walkNum)
proc updateFleet {} {
   set fleetId $::allFleetMap($::walkNum)

	# turn off all the triggers, while we shuffle things around
   set nsGui::internalCbTask 1

   .tGui.fWayPtTask.fExtras.eAmt delete 0 end

   .tGui.cScannerPane itemconfigure paths -fill blue
   .tGui.cScannerPane itemconfigure path$fleetId -fill #00ffff
   .tGui.pfWalker.lName configure -text [newStars $::ns_planet $fleetId $::ns_getName]

   set slaves [pack slaves .tGui.fWayPtTask.fExtras]
   if {$slaves ne ""} {
      eval [list pack forget {*}$slaves]
   }
   .tGui.fWaypoints.lbWaypoints delete 0 end

   .tGui.fWayPtTask.cbTask select 0
   set nsGui::internalCbTask 0

   set ::ltrFuel "Fuel [getFuel $fleetId] mg"
   calcCargo $fleetId
   fillShips $fleetId
	fillOther $fleetId

   set location [newStars $::ns_planet $fleetId $::ns_getPlanetId]
   if {$location eq ""} {
      .tGui.fFleetLocation.lFLname configure -text "In Deep Space"
      .tGui.fFleetLocation.bGoto configure -state disabled
      set x [newStars $::ns_planet $fleetId $::ns_getX]
      set y [newStars $::ns_planet $fleetId $::ns_getY]
      .tGui.fWaypoints.lbWaypoints insert end "Space ($x, $y)"
   } else {
      set pname [newStars $::ns_planet $location $::ns_getName]
      .tGui.fFleetLocation.lFLname configure -text "Orbiting $pname"
      .tGui.fFleetLocation.bGoto configure -state normal
      .tGui.fWaypoints.lbWaypoints insert end "$pname"
   }

   set tmp [newStars $::ns_planet $fleetId $::ns_getNumOrders]
   for {set i 0} {$i < $tmp} {incr i} {
      .tGui.fWaypoints.lbWaypoints insert end \
          [newStars $::ns_planet $fleetId $::ns_getOrderNameFromIndex $i]
   }

   .tGui.fWaypoints.lbWaypoints selection set end
   hndWayPtSel

   set wpNum [expr [.tGui.fWaypoints.lbWaypoints curselection] - 1]
   if {$wpNum >= 0} {
      .tGui.fWayDetails.fWarp.s set [newStars $::ns_planet $fleetId $::ns_speed $wpNum]
   } else {
      .tGui.fWayDetails.fWarp.s set 0
   }

   updateBottomRight "FLEET" $fleetId
}

# go to the prev planet or fleet
proc prevWalk {} {
   if {$nsGui::walkPlanet} {
      if {$::numPlanets == 0} { return }

      set walkMax $::numPlanets
   } else {
      set walkMax [array size ::allFleetMap]
   }

   if {$::walkNum == 0} {
      set ::walkNum [expr $walkMax - 1]
   } else {
      incr ::walkNum -1
   }

   if {$nsGui::walkPlanet} {
      updatePlanet $::walkNum
   } else {
      updateFleet
   }
}

# go to the next planet or fleet
proc nextWalk {} {
   if {$nsGui::walkPlanet} {
      if {$::numPlanets == 0} { return }

      set walkMax $::numPlanets
   } else {
      set walkMax [array size ::allFleetMap]
   }

   incr ::walkNum
   if {$::walkNum == $walkMax} {
      set ::walkNum 0
   }

   if {$nsGui::walkPlanet} {
      updatePlanet $::walkNum
   } else {
      updateFleet
   }
}

proc deleteOrder {ct} {
   global walkNum

   if {$nsGui::walkPlanet} {
      set walkId $::planetMap($walkNum)

      set sel [.tGui.fProduction.lbPlanetQueue curselection]
      if [winfo exists .tProd.lbQueue] {
         set sel [.tProd.lbQueue curselection]
      }
      if {$sel eq ""} { return }

      for {set i 0} {$i < $ct} {incr i} {
         set res [newStars $::ns_planet $walkId $::ns_deleteAtIndex $sel]
         if {$res} {break}
      }

      if [winfo exists .tProd.lbQueue] {
         .tProd.lbQueue delete 0 end
         fillProdQueue .tProd.lbQueue $walkId
         .tProd.lbQueue selection set $sel
      }

      .tGui.fProduction.lbPlanetQueue delete 0 end
      fillProdQueue .tGui.fProduction.lbPlanetQueue $walkId
      .tGui.fProduction.lbPlanetQueue selection set $sel

   } else {
      set walkId $::allFleetMap($walkNum)

      set sel [.tGui.fWaypoints.lbWaypoints curselection]
      if {$sel eq 0} { return }

      .tGui.fWaypoints.lbWaypoints delete $sel
      .tGui.fWaypoints.lbWaypoints selection set $sel
      incr sel -1

      newStars $::ns_planet $walkId $::ns_deleteAtIndex $sel

      .tGui.cScannerPane delete path$walkId
      drawPaths [lindex $::maglist $::magnidx] $walkId
      .tGui.cScannerPane itemconfigure path$walkId -fill #00ffff
   }
}

proc getItemId {itemStr} {
   if {$itemStr eq "Factory" || $itemStr eq "Factory (Auto Build)"} {
      return 3
   } elseif {$itemStr eq "Mine" || $itemStr eq "Mines (Auto Build)"} {
      return 4
   } elseif {$itemStr eq "Defenses" || $itemStr eq "Defenses (Auto Build)"} {
      return 2
   } elseif {$itemStr eq "Mineral Alchemy" || $itemStr eq "Alchemy (Auto Build)"} {
      return 12
   } elseif {$itemStr eq "Planetary Scanner"} {
      return 5
   } elseif {$itemStr eq "Terraform Environment" || $itemStr eq "Max Terraform (Auto Build)"} {
      return 11
   } elseif {$itemStr eq "Min Terraform (Auto Build)"} {
      return 10
   }
   return 1
}

# select transport which action (currently disabled)
proc selectTWA {path val} {
}

proc wptEntry {newE} {
   if {$nsGui::internalCbTask} {return 1}

   set wpNum [.tGui.fWaypoints.lbWaypoints curselection]
   if {$wpNum == 0} {return 1}

   incr wpNum -1

   set fleetId $::allFleetMap($::walkNum)

   set orderXML  [xml2list [newStars $::ns_planet $fleetId $::ns_getOrderXMLfromIndex $wpNum]]
   set mainXML   [lindex $orderXML 2]

   set cargo     [lindex [findXMLbyTag "CARGOMANIFEST" $mainXML 2] 1]

   set val [enumCargoId [.tGui.fWayPtTask.fExtras.cbTranWhich curselection]]

   set amt       [lindex $cargo $val]

   if {$newE eq ""} {return 1}
   newStars $::ns_orders $fleetId $wpNum $::ns_orderTransport $val $newE

   return 1
}

proc enumCargoId {d} {
   switch $d {
      0  { return 4 }
      1  { return 1 }
      2  { return 2 }
      3  { return 3 }
      4  { return 0 }
   }
}

proc unmapCargoId {d} {
   switch $d {
      0  { return 3 }
      1  { return 0 }
      2  { return 1 }
      3  { return 2 }
      4  { return 4 }
   }
}

proc findIndexFromCargo {cstr} {
   switch $cstr {
      Fuel      { return 0 }
      Ironium   { return 1 }
      Boranium  { return 2 }
      Germanium { return 3 }
      Colonists { return 4 }
   }
}

proc selectTW {path val} {
   set wpNum [.tGui.fWaypoints.lbWaypoints curselection]
   if {$wpNum == 0} {return}

   set nsGui::internalCbTask 1
   .tGui.fWayPtTask.fExtras.eAmt delete 0 end

   incr wpNum -1
   set fleetId $::allFleetMap($::walkNum)

   set orderXML  [xml2list [newStars $::ns_planet $fleetId $::ns_getOrderXMLfromIndex $wpNum]]
   set mainXML   [lindex $orderXML 2]

   set cargo     [lindex [findXMLbyTag "CARGOMANIFEST" $mainXML 2] 1]
   set amt       [lindex $cargo [unmapCargoId [findIndexFromCargo $val]]]

   .tGui.fWayPtTask.fExtras.eAmt insert 0 $amt
   set nsGui::internalCbTask 0
}

proc packExtrasByTask {task} {
   switch $task {
      "GUI Load"   -
      "GUI Unload" -
      "Load Cargo" -
      "Unload Cargo"   {
         pack .tGui.fWayPtTask.fExtras.cbTranWhich -side top
         pack .tGui.fWayPtTask.fExtras.eAmt        -side top
      }

      "Transfer Fleet" {
         pack .tGui.fWayPtTask.fExtras.cbTgtPlayer -side top
         .tGui.fWayPtTask.fExtras.cbTgtPlayer select 0
      }
   }
}
# with cargo
#         pack .tGui.fWayPtTask.fExtras.cbAction    -side top

proc selectWPT {path val} {
   set slaves [pack slaves .tGui.fWayPtTask.fExtras]
   if {$slaves ne ""} {
      eval [list pack forget {*}$slaves]
   }

   if {!$nsGui::internalCbTask} {
      set curWpt [.tGui.fWaypoints.lbWaypoints curselection]
      if {$curWpt ne ""} {
         set fleetId $::allFleetMap($::walkNum)
         if {$val eq "Load Cargo"} {
            newStars $::ns_planet $fleetId $::ns_addAtIndex $curWpt $::ns_orderTransport 0
         } elseif {$val eq "Unload Cargo"} {
            newStars $::ns_planet $fleetId $::ns_addAtIndex $curWpt $::ns_orderTransport 1
         } elseif {$val eq "Colonize"} {
            newStars $::ns_planet $fleetId $::ns_addAtIndex $curWpt $::ns_orderColonize
         } elseif {$val eq "Scrap Fleet"} {
            newStars $::ns_planet $fleetId $::ns_addAtIndex $curWpt $::ns_deleteAtIndex
         }
         updateFleet
      }
   }

   packExtrasByTask $val
}

proc selectTFP {path val} {
}

proc remapOrderEnum {e} {
   switch $e {
   0 -
   1
      { return 0 }

   2  { return 4 }
   3  { return 7 }
   4  { return 6 }

   5 -
   12 { return 1 }

   13 -
   6  { return 2 }

   7  { return 3 }
   8  { return 8 }
   9  { return 10 }
   10 { return 0 }
   11 { return 0 }
   }
}

# given the display order, return the manifest idx
proc remapCargoId {c} {
   switch $c {
      0  { return 1 }
      1  { return 2 }
      2  { return 3 }
      3  { return 0 }
      4  { return 4 }
   }
}

proc hndWayPtSel {} {
   .tGui.fWayDetails.fWarp.s set 0

   set fleetId $::allFleetMap($::walkNum)
   set tmp [newStars $::ns_planet $fleetId $::ns_getNumOrders]
   if {$tmp == 0} { return }

   set slaves [pack slaves .tGui.fWayPtTask.fExtras]
   if {$slaves ne ""} {
      eval [list pack forget {*}$slaves]
   }

   set wpNum [expr [.tGui.fWaypoints.lbWaypoints curselection] - 1]
   if {$wpNum < 0} { return }

   set orderXML  [xml2list [newStars $::ns_planet $fleetId $::ns_getOrderXMLfromIndex $wpNum]]
   set mainXML   [lindex $orderXML 2]
   set orderEnum [lindex [findXMLbyTag "ORDERID" $mainXML 2] 1]

   set remapOrderEnum [remapOrderEnum $orderEnum]
   set nsGui::internalCbTask 1
   .tGui.fWayPtTask.cbTask select $remapOrderEnum
   set nsGui::internalCbTask 0

   .tGui.fWayDetails.fWarp.s set [newStars $::ns_planet $fleetId $::ns_speed $wpNum]

   if {$remapOrderEnum == 1 || $remapOrderEnum == 2} {
      packExtrasByTask "Transport"

      set cargo [lindex [findXMLbyTag "CARGOMANIFEST" $mainXML 2] 1]
      for {set i 0} {$i < 5} {incr i} {
         set amt [lindex $cargo $i]
         if {$amt} {
            .tGui.fWayPtTask.fExtras.cbTranWhich select [remapCargoId $i]
            if {$orderEnum == 5} {
               .tGui.fWayPtTask.fExtras.cbAction select 4
            } else {
               .tGui.fWayPtTask.fExtras.cbAction select 3
            }

				selectTW  "" [.tGui.fWayPtTask.fExtras.cbTranWhich get]
            break
         }
      }
   }
}

proc addBuildOrder {ct} {
   set addItem [.tProd.lbAvail curselection]
   if {$addItem eq ""} { return }

   set autoBuild 0
   if {[.tProd.lbAvail itemcget $addItem -background] eq "green"} {
      set autoBuild 1
   }

   set buildItem [getItemId [.tProd.lbAvail get $addItem]]
   set shipId 0
   if {$buildItem == 1 } {
      set shipId $::designMap($addItem)
   }

   set lb .tProd.lbQueue
   set sel [.tProd.lbQueue curselection]
   if {$sel eq ""} {
      set sel [.tProd.lbQueue size]
   }

   set planetId $::planetMap($::walkNum)
   set newI 0
   for {set i 0} {$i < $ct} {incr i} {
      set res [newStars $::ns_planet $planetId $::ns_addAtIndex \
          $sel $autoBuild $buildItem $shipId]
      set newI [expr $newI || [lindex $res 0]]
      incr sel [lindex $res 1]
   }

   if {!$newI && [.tProd.lbQueue size] > 1} {
      if {$sel != [$lb size]-1} {
         $lb delete $sel
      }
   }

   drawProdItem $lb $planetId $sel $sel

   $lb selection clear 0 end
   $lb selection set $sel

   .tGui.fProduction.lbPlanetQueue delete 0 end
   fillProdQueue .tGui.fProduction.lbPlanetQueue $planetId
   .tGui.fProduction.lbPlanetQueue selection set $sel
}

proc orderMv {dir} {
   set lb .tProd.lbQueue
   set sel [$lb curselection]
   if {$sel eq ""} {
      return
   }
   set planetId $::planetMap($::walkNum)
   newStars $::ns_planet $planetId $::ns_orderMove $sel $dir

   set other [expr $sel + $dir]
   if {$other == [$lb size]-1 || $other == -1} {return}

   if {$sel < $other} {
      $lb delete $sel $other
      drawProdItem $lb $planetId $sel $sel
      drawProdItem $lb $planetId $other $other
   } else {
      $lb delete $other $sel
      drawProdItem $lb $planetId $other $other
      drawProdItem $lb $planetId $sel $sel
   }

   $lb selection clear 0 end
   $lb selection set $other

   .tGui.fProduction.lbPlanetQueue delete 0 end
   fillProdQueue .tGui.fProduction.lbPlanetQueue $planetId
   .tGui.fProduction.lbPlanetQueue selection set $sel
}

#############################################################################
# support for do Open, build initial production list
proc buildAvailList {} {
   for {set i 0} {$i < $::numDesigns} {incr i} {
      .tProd.lbAvail insert end [newStars $::ns_planet $::designMap($i) $::ns_getName]
   }

   .tProd.lbAvail insert end "Factory" "Mine" "Defenses" "Mineral Alchemy"
   .tProd.lbAvail insert end "Planetary Scanner"
   .tProd.lbAvail insert end "Terraform Environment"

   .tProd.lbAvail insert end "Mines (Auto Build)"
   .tProd.lbAvail itemconfigure end -background green
   .tProd.lbAvail insert end "Factory (Auto Build)"
   .tProd.lbAvail itemconfigure end -background green
   .tProd.lbAvail insert end "Defenses (Auto Build)"
   .tProd.lbAvail itemconfigure end -background green
   .tProd.lbAvail insert end "Alchemy (Auto Build)"
   .tProd.lbAvail itemconfigure end -background green
   .tProd.lbAvail insert end "Min Terraform (Auto Build)"
   .tProd.lbAvail itemconfigure end -background green
   .tProd.lbAvail insert end "Max Terraform (Auto Build)"
   .tProd.lbAvail itemconfigure end -background green
   .tProd.lbAvail insert end "Mineral Packets (Auto Build)"
   .tProd.lbAvail itemconfigure end -background green
}

proc setPlanetPrefs {} {
   set raceXML [xml2list [newStars $::ns_race $::ns_getXML]]
   set mainXML [lindex $raceXML 2]

   for {set i 0} {$i < 3} {incr i} {
      .tGui.fBottomRight.fPlanet.fRow3.c$i delete all

      set tmpXML [findXMLbyTag "IMMUNITY" $mainXML 2]
      set tuple  [lindex $tmpXML 1]

      set imm [lindex $tuple $i]
      if {$imm == 0} {
         set tmpXML [findXMLbyTag "HAB" $mainXML 2]
         set tuple  [lindex $tmpXML 1]
         set ctr    [lindex $tuple $i]

         set tmpXML [findXMLbyTag "TOLERANCE" $mainXML 2]
         set tuple  [lindex $tmpXML 1]
         set rng    [lindex $tuple $i]

         drawHabBar .tGui.fBottomRight.fPlanet.fRow3.c$i $ctr $rng $raceWiz::envColor($i)
      }
   }
}

proc setMsg {idx} {
   if {$::numMessages > $idx} {
      set msgXML [xml2list [newStars $::ns_message $idx $::ns_getXML]]
      set mainXML [lindex $msgXML 2]
      set body [findXMLbyTag "MSG" $mainXML 2]

      set nsGui::msgTxt [lindex $body 1]
      .tGui.fMsg.msgPaneHdr.lYear configure -text "Year: $::year Messages: [expr $idx + 1] of $::numMessages"
   } else {
      set nsGui::msgTxt "You have no messages this year"
      .tGui.fMsg.msgPaneHdr.lYear configure -text "Year: $::year No Messages"
   }
}

proc nxtMsg {} {
   incr nsGui::msgNum
   if {$::numMessages <= $nsGui::msgNum} {
      set nsGui::msgNum 0
   }
   setMsg $nsGui::msgNum
}

proc prvMsg {} {
   incr nsGui::msgNum -1
   if {$nsGui::msgNum < 0} {
      set nsGui::msgNum [expr $::numMessages - 1]
   }
   setMsg $nsGui::msgNum
}

### battle board
proc findBatTokenByTid {tgtTid} {
   foreach t [$batBrd::bNode getElementsByTagName TOKEN] {
      set tidEl [$t getElementsByTagName TOKENID]
      set tid [[$tidEl firstChild] nodeValue]
      if {$tid == $tgtTid} { return $t }
   }
   return ""
}

proc selectToken {t} {
   set batBrd::selTok $t
   set desEl [$t getElementsByTagName DESIGNID]
   set des [[$desEl firstChild] nodeValue]

   set name [newStars $::ns_planet $des $::ns_getName]
   .tBattleVCR.fInfo.lMvNm configure -text $name

   set ownName [newStars $::ns_player $::ns_getName [newStars $::ns_planet $des $::ns_getOwnerId]]
   .tBattleVCR.fInfo.lMvOwn configure -text $ownName

   .tBattleVCR.fInfo.lOwn configure -text "$ownName $name"
}

proc getBatTokStat {tgtTid evtNum} {
   # look through token list
   foreach t [$batBrd::bNode getElementsByTagName TOKEN] {
      set tidEl [$t getElementsByTagName TOKENID]
      set tid [[$tidEl firstChild] nodeValue]

      if {$tid != $tgtTid} { continue }

      set xEl [$t getElementsByTagName POS_X]
      set x [[$xEl firstChild] nodeValue]

      set yEl [$t getElementsByTagName POS_Y]
      set y [[$yEl firstChild] nodeValue]
   }

   for {set i 0} {$i < $evtNum} {incr i} {
      set t     [lindex $batBrd::eList $i]
      if {[$t nodeName] ne "MOVE"} {continue}

      set tidEl [$t getElementsByTagName TOKENID]
      set tid  [[$tidEl firstChild] nodeValue]

      if {$tid != $tgtTid} { continue }

      set xEl [$t getElementsByTagName POS_X]
      set x [[$xEl firstChild] nodeValue]

      set yEl [$t getElementsByTagName POS_Y]
      set y [[$yEl firstChild] nodeValue]
   }
   return [list $x $y]
}

proc showFire {e} {
   set tidEl [$e getElementsByTagName TOKENID]
   set aTid [[$tidEl firstChild] nodeValue]

   set tidEl [$e getElementsByTagName TARGET]
   set tTid [[$tidEl firstChild] nodeValue]

   set tidEl [$e getElementsByTagName SHIELD]
   set s [[$tidEl firstChild] nodeValue]

   set tidEl [$e getElementsByTagName ARMOR]
   set a [[$tidEl firstChild] nodeValue]

   set tidEl [$e getElementsByTagName COUNT]
   set c [[$tidEl firstChild] nodeValue]

   .tBattleVCR.fInfo.lDet configure -text "reduces token $tTid to\
       shields: $s armor: $a count: $c"

   set tokStat [getBatTokStat $tTid $batBrd::curEvt]
   set tX [lindex $tokStat 0]
   set tY [lindex $tokStat 1]

   set tokStat [getBatTokStat $aTid $batBrd::curEvt]
   set aX [lindex $tokStat 0]
   set aY [lindex $tokStat 1]

   if {$aX != $tX || $aY != $tY} {
      if {$aX != $tX} {
         .tBattleVCR.c create line [expr $aX * 60 + 30] [expr $aY * 60] \
             [expr $tX * 60 + 30] [expr $tY * 60 + 30] \
             [expr $aX * 60 + 30] [expr $aY * 60 + 60] -fill red -tags firePath
      } else {
      }
   }
}

# goto first (first != 0) (or last (first == 0)) event
proc evtFirst {first} {
   #Ned clear selection
   .tBattleVCR.c delete firePath
   .tBattleVCR.fBut.bNext configure -state normal
   .tBattleVCR.fBut.bPrev configure -state disabled

   # look through token list
   foreach t [$batBrd::bNode getElementsByTagName TOKEN] {
      set tidEl [$t getElementsByTagName TOKENID]
      set tid [[$tidEl firstChild] nodeValue]

      set xEl [$t getElementsByTagName POS_X]
      set x [[$xEl firstChild] nodeValue]

      set yEl [$t getElementsByTagName POS_Y]
      set y [[$yEl firstChild] nodeValue]

      .tBattleVCR.c coords token$tid [expr $x *60] [expr $y * 60]
   }

   set batBrd::curEvt 0
   if {$first} { return }

   .tBattleVCR.fBut.bNext configure -state disabled
   .tBattleVCR.fBut.bPrev configure -state normal
   foreach t $batBrd::eList {
      if {[$t nodeName] ne "MOVE"} {continue}
      set tidEl [$t getElementsByTagName TOKENID]
      set tid  [[$tidEl firstChild] nodeValue]

      set xEl [$t getElementsByTagName POS_X]
      set x [[$xEl firstChild] nodeValue]

      set yEl [$t getElementsByTagName POS_Y]
      set y [[$yEl firstChild] nodeValue]

      .tBattleVCR.c coords token$tid [expr $x *60] [expr $y * 60]
   }
   set batBrd::curEvt [llength $batBrd::eList]

   set e [lindex $batBrd::eList end]
   if {[$e nodeName] eq "FIRE"} {
      showFire $e
   }
}

# rolling back is harder...
proc evtPrv {} {
   .tBattleVCR.c delete firePath
   .tBattleVCR.fBut.bNext configure -state normal
   incr batBrd::curEvt -1

   .tBattleVCR.fInfo.lDet configure -text ""

   # find the last moved tid
   set prevE [lindex $batBrd::eList $batBrd::curEvt]
   set tidEl [$prevE getElementsByTagName TOKENID]
   set tgtTid [[$tidEl firstChild] nodeValue]

   selectToken [findBatTokenByTid $tgtTid]

   if {[$prevE nodeName] ne "MOVE"} {return}

   set tokStat [getBatTokStat $tgtTid $batBrd::curEvt]
   set x [lindex $tokStat 0]
   set y [lindex $tokStat 1]

   # if back to start
   if {$batBrd::curEvt == 0} {
      .tBattleVCR.fBut.bPrev configure -state disabled
   }

   .tBattleVCR.c coords token$tgtTid [expr $x *60] [expr $y * 60]
}

proc evtNxt {} {
   .tBattleVCR.fBut.bPrev configure -state normal
   .tBattleVCR.c delete firePath

   incr batBrd::curEvt
   if {$batBrd::curEvt == [llength $batBrd::eList]} {
      .tBattleVCR.fBut.bNext configure -state disabled
   }

   set e [lindex $batBrd::eList [expr $batBrd::curEvt - 1]]
   set tidEl [$e getElementsByTagName TOKENID]
   set tid [[$tidEl firstChild] nodeValue]

   switch [$e nodeName] {
      MOVE {
         set xEl [$e getElementsByTagName POS_X]
         set x [[$xEl firstChild] nodeValue]

         set yEl [$e getElementsByTagName POS_Y]
         set y [[$yEl firstChild] nodeValue]

         .tBattleVCR.c coords token$tid [expr $x *60] [expr $y * 60]
      }
      FIRE {
         showFire $e
      }
   }
   selectToken [findBatTokenByTid $tid]
}

proc viewBatSel {x y} {
   set t $batBrd::selTok
	if {$t eq ""} {return}

   frame .tBattleVCR.fViewDes

   set desEl [$t getElementsByTagName DESIGNID]
   set des [[$desEl firstChild] nodeValue]

   set name [newStars $::ns_player $::ns_getName [newStars $::ns_planet $des $::ns_getOwnerId]]
   append name " " [newStars $::ns_planet $des $::ns_getName]
   pack [label .tBattleVCR.fViewDes.lName -text $name] -side top

   set shipXML  [xml2list [newStars $::ns_planet $des $::ns_getXML]]
   set mainXML  [lindex $shipXML 2]

   pack [frame .tBattleVCR.fViewDes.fCurShip]
   pack [frame .tBattleVCR.fViewDes.fCost]
   drawShip .tBattleVCR.fViewDes $mainXML 0

   set nx [expr $x - [winfo rootx .tBattleVCR]]
   set ny [expr $y - [winfo rooty .tBattleVCR]]
   place .tBattleVCR.fViewDes -anchor se -x $nx -y $ny

   bind .tBattleVCR <ButtonRelease-1> "destroy .tBattleVCR.fViewDes"
}

# create the battle VCR window
proc viewBattle {bNode} {
   toplevel .tBattleVCR

   # create battle board
   pack [frame  .tBattleVCR.fTop] -side top
   pack [canvas .tBattleVCR.c -width 600 -height 600 -bg black] -side left -in .tBattleVCR.fTop

   namespace eval batBrd {
      set eList  ""
      set curEvt 0
      set bNode  ""
      set selTok ""
   }
   set batBrd::eList [[$bNode getElementsByTagName EVENTS] childNodes]
   set batBrd::bNode $bNode

   pack [frame  .tBattleVCR.fBut] -side bottom
   pack [button .tBattleVCR.fBut.bFirst -text "|<<"  -width 6 -command {evtFirst 1}] -side left -padx 6
   pack [button .tBattleVCR.fBut.bPrev  -text "<"    -width 6 -command {evtPrv}] -side left -padx 6
   pack [button .tBattleVCR.fBut.bPlay  -text ">/||" -width 6 -command {}] -side left -padx 6
   pack [button .tBattleVCR.fBut.bNext  -text ">"    -width 6 -command {evtNxt}] -side left -padx 6
   pack [button .tBattleVCR.fBut.bLast  -text ">>|"  -width 6 -command {evtFirst 0}] -side left -padx 6
   pack [button .tBattleVCR.fBut.bDone  -text "Done" -width 6 -command {destroy .tBattleVCR}] -side left -padx 6
   pack [button .tBattleVCR.fBut.bHelp  -text "Help" -width 6 -command {}] -side left -padx 6

   pack [frame .tBattleVCR.fInfo] -side right -in .tBattleVCR.fTop -expand 1 -fill both
   pack [label .tBattleVCR.fInfo.lPhase -text "Phase 1 of [llength $batBrd::eList]"] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lMvOwn -text "<Owner>"] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lMvNm  -text "<Name>"]  -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lDet   -text ""] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lSel   -text "Selection:"] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lOwn   -text "<Owner>"] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lArm   -text "Armor:"] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lDam   -text "Damage:"] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lShd   -text "Shields:"] -side top -anchor nw
   pack [label .tBattleVCR.fInfo.lView  -text "?"] -side top -anchor nw

   bind .tBattleVCR.fInfo.lView <ButtonPress-1> "viewBatSel %X %Y"

   .tBattleVCR.fBut.bPrev configure -state disabled

   # load the tokens
   set setFirst 0
   foreach t [$bNode getElementsByTagName TOKEN] {
      set tidEl [$t getElementsByTagName TOKENID]
      set tid [[$tidEl firstChild] nodeValue]

      set xEl [$t getElementsByTagName POS_X]
      set x [[$xEl firstChild] nodeValue]

      set yEl [$t getElementsByTagName POS_Y]
      set y [[$yEl firstChild] nodeValue]

      set cid [.tBattleVCR.c create image [expr $x * 60] [expr $y * 60] -anchor nw -tags "token$tid" \
         -image $::shipImage]

      .tBattleVCR.c bind $cid <1> [list selectToken $t]
   }

   # create the grid lines
   for {set i 1} {$i < 10} {incr i} {
      .tBattleVCR.c create line [expr $i * 60] 0 [expr $i * 60] 600 -fill gray60
      .tBattleVCR.c create line 0 [expr $i * 60] 600 [expr $i * 60] -fill gray60
   }
}

# create the planet summary window
proc showPlanets {} {
   if {![winfo exists .tStatus]} {
      toplevel .tStatus
   } else {
      eval "destroy [pack slaves .tStatus]"
   }
   wm title .tStatus "Planet Summary Report"

   grid [label .tStatus.lP0   -text "Name"       ] -row 0 -column 0
   grid [label .tStatus.lS0   -text "Starbase"   ] -row 0 -column 1
   grid [label .tStatus.lPop0 -text "Population" ] -row 0 -column 2
   grid [label .tStatus.lCap0 -text "Cap"        ] -row 0 -column 3
   grid [label .tStatus.lVal0 -text "Value"      ] -row 0 -column 4
   grid [label .tStatus.lPrd0 -text "Production" ] -row 0 -column 5
   grid [label .tStatus.lMne0 -text "Mine"       ] -row 0 -column 6
   grid [label .tStatus.lFct0 -text "Fact"       ] -row 0 -column 7
   grid [label .tStatus.lDef0 -text "Defense"    ] -row 0 -column 8
   grid [label .tStatus.lMnl0 -text "Minerals"   ] -row 0 -column 9  -columnspan 3
   grid [label .tStatus.lMrt0 -text "Mining Rate"] -row 0 -column 12 -columnspan 3
   grid [label .tStatus.lMcn0 -text "Min Conc"   ] -row 0 -column 15 -columnspan 3
   grid [label .tStatus.lRes0 -text "Resources"  ] -row 0 -column 18

	# foreach planet we own
   set i 1
   foreach p [array names nsGui::myPlanetMap] {
      set pid $nsGui::myPlanetMap($p)
      set n [$nsGui::turnRoot selectNodes //PLANET_LIST/PLANET\[OBJECTID=$pid\]]
      set name [[[$n getElementsByTagName NAME] firstChild] nodeValue]
      grid [label .tStatus.lP$i -text "$name"] -row $i -column 0

      set sid  [[[$n getElementsByTagName STARBASEID] firstChild] nodeValue]
      set sname "--"
      if {$sid != 0} {
         set sname [newStars $::ns_planet $sid $::ns_getName]
      }
      grid [label .tStatus.lS$i -text "$sname"] -row $i -column 1

      set tuple [[[$n getElementsByTagName CARGOMANIFEST] firstChild] nodeValue]
      set col   [commify [expr [lindex $tuple 4] * 100]]
      grid [label .tStatus.lPop$i -text "$col"] -row $i -column 2

      grid [label .tStatus.lCap$i -text ""] -row $i -column 3
      grid [label .tStatus.lVal$i -text ""] -row $i -column 4
      grid [label .tStatus.lPrd$i -text ""] -row $i -column 5

      set infra [[[$n getElementsByTagName INFRASTRUCTURE] firstChild] nodeValue]

      grid [label .tStatus.lMne$i -text [lindex $infra 0]] -row $i -column 6
      grid [label .tStatus.lFct$i -text [lindex $infra 1]] -row $i -column 7
      grid [label .tStatus.lDef$i -text [lindex $infra 2]] -row $i -column 8

      grid [label .tStatus.lIrn$i -text [lindex $tuple 0] -fg blue  ] -row $i -column 9
      grid [label .tStatus.lBrn$i -text [lindex $tuple 1] -fg green4] -row $i -column 10
      grid [label .tStatus.lGmn$i -text [lindex $tuple 2] -fg yellow4] -row $i -column 11

      set concs [$n selectNodes string(MINERALCONCENTRATIONS/text())]

      grid [label .tStatus.lIrt$i -text "" -fg blue  ] -row $i -column 12
      grid [label .tStatus.lBrt$i -text "" -fg green4] -row $i -column 13
      grid [label .tStatus.lGrt$i -text "" -fg yellow4] -row $i -column 14

      grid [label .tStatus.lIcn$i -text [lindex $concs 0] -fg blue  ] -row $i -column 15
      grid [label .tStatus.lBcn$i -text [lindex $concs 1] -fg green4] -row $i -column 16
      grid [label .tStatus.lGcn$i -text [lindex $concs 2] -fg yellow4] -row $i -column 17

      set res [[[$n getElementsByTagName RESOURCESGENERATED] firstChild] nodeValue]
      grid [label .tStatus.lRes$i -text $res] -row $i -column 18

      bind .tStatus.lP$i <Double-1> [list rightMouseCmdPlan $nsGui::myPlanetMap($p)]
      incr i
   }
   if {$i == 1} {
      pack [label .tStatus.l -text "<You control zero planets>"]
   }
}

# create the battle summary window
proc showBattles {} {
   if {![winfo exists .tStatus]} {
      toplevel .tStatus
   } else {
      eval "destroy [pack slaves .tStatus]"
   }
   wm title .tStatus "Battle Summary Report"

   set bs [$nsGui::turnRoot getElementsByTagName BATTLES]
   set bList [$bs childNodes]

   set i 0
   foreach b $bList {
      pack [label .tStatus.lB$i -text "Battle $i"]
      bind .tStatus.lB$i <Double-1> [list viewBattle $b]
      incr i
   }
   if {$i == 0} {
      pack [label .tStatus.l -text "<No battles this year>"]
   }
}

# add f to the recent file list
proc addRecentFile {f} {
   set l [lsort -integer [array names nsGui::prevFiles]]
   if {[llength $l] == 0} {
      set nsGui::prevFiles(0) $f
      return
   }

   set n -1
   foreach {i v} [array get nsGui::prevFiles] {
      if {$v eq $f} {
         set n $i
         break
      }
   }
   if {$n != -1} {
      return
   }

   set m [lindex $l end]
   if {$m >= 9} {
      for {set i 1} {$i < 10} {incr i} {
         set nsGui::prevFiles([expr $i - 1]) $nsGui::prevFiles($i)
      }
      set nsGui::prevFiles(9) $f
   } else {
      set nsGui::prevFiles([expr $m + 1]) $f
   }
}

# open a turn file, and reconfigure elements for it
proc doOpen {{fname ""}} {
   if {$fname eq ""} {
      set ofile [tk_getOpenFile -initialdir $nsGui::ofileDir]
      if {$ofile eq ""} { return }
   } else {
      set ofile $fname
   }
   set nsGui::ofileDir [file dirname $ofile]

   addRecentFile $ofile
   .mTopMenu.mFile delete 0 end
   buildFileMenu

   global numPlanets planetMap

   unset -nocomplain planetMap   numPlanets
   unset -nocomplain ::fleetMap
   unset -nocomplain ::enemyFleetMap
   unset -nocomplain ::allFleetMap
   unset -nocomplain designMap   numDesigns
   unset -nocomplain componentIdxList numComponents
   set nsGui::filename $ofile
   if {$nsGui::turnRoot ne ""} { $nsGui::turnRoot delete }

   set nsGui::playerNum [newStars $::ns_open $ofile]
   newStars $::ns_race $::ns_loadPre 7; # custom
   wm title .tGui "NStars! -- [newStars $::ns_race $::ns_getName] -- [file tail $ofile]"

   set fchn [open $ofile]
   set nsGui::turnRoot [dom parse -channel $fchn]

   .tGui.fWayPtTask.fExtras.cbTgtPlayer list delete 0 end
   for {set i 1} {$i <= $::numPlayers} {incr i} {
      if {$i != $nsGui::playerNum} {
         .tGui.fWayPtTask.fExtras.cbTgtPlayer list insert end $i
      }
   }

   buildMyPlanetMap

   if {$nsGui::myPlanetCt > 0} {
      set ::walkNum [findPlanetWalkFromId $nsGui::myPlanetMap(0)]
      set nsGui::walkPlanet 1
   } elseif {[array size ::allFleetMap] > 0} {
      set ::walkNum $::allFleetMap(0)
      set nsGui::walkPlanet 0
   } else {
      exit
   }

   drawStarMap .tGui [lindex $::maglist $::magnidx]

   setPlanetPrefs

   set ::year [newStars $::ns_universe $::ns_getYear]
   set nsGui::msgNum 0
   setMsg $nsGui::msgNum

   if {$nsGui::walkPlanet} {
      updatePlanet $::walkNum
   } else {
      updateFleet $::walkNum
   }

   if [winfo exists .tProd.lbAvail] {
      .tProd.lbAvail delete 0 end
      buildAvailList
   }

   #Ned why is the scrollbar width 1? [winfo width .tGui.cScannerPaneV]
   set scanPaneWidth [expr [winfo width .tGui] - [winfo width .tGui.fBottomRight] - \
                           20]
   .tGui.cScannerPane configure -width $scanPaneWidth
}

# save current state to file
proc doSave {} {
   if {$nsGui::filename eq ""} {
      set nsGui::filename [tk_getSaveFile]
      if {$nsGui::filename eq ""} { return }
   }
   newStars $::ns_save $nsGui::filename
}

proc updateCurResearch {n1 n2 op} {
   .tTechDlg.fR.fT.lWhat configure -text "$nsGui::techName($::curResearch), Tech Level \
[expr $::currentTech($::curResearch) + 1]"
}

proc researchTax {amt} {
   set curVal [.tTechDlg.fR.fB.fBud.s get]
   incr curVal $amt
   if {$curVal < 0} {set curVal 0}

   .tTechDlg.fR.fB.fBud.s set $curVal
   newStars $::ns_player $::ns_researchTax $curVal
}

proc setNextResearch {wnd val} {
	if {[$wnd curselection] eq ""} { return }
	set ::nextResearch [$wnd curselection]
}

# create the research dialog
proc doTech {} {
   toplevel .tTechDlg
   wm title .tTechDlg "Research"
   bind .tTechDlg <Escape> "destroy .tTechDlg"

   pack [frame .tTechDlg.fL] -side left
   pack [frame .tTechDlg.fR] -side right

   pack [labelframe  .tTechDlg.fL.fStat -text "Technology Status"] -side left

   pack [frame .tTechDlg.fL.fStat.fRT] -side top -expand 1 -fill x
   pack [label .tTechDlg.fL.fStat.fRT.lT1 -text "Field of Study"] -side left
   pack [label .tTechDlg.fL.fStat.fRT.lT2 -text "Current\nLevel"] -side right

   pack [frame       .tTechDlg.fL.fStat.fR0] -side top -expand 1 -fill x
   pack [radiobutton .tTechDlg.fL.fStat.fR0.rbEngy -text $nsGui::techName(0) -variable curResearch -value 0] -side left
   pack [label       .tTechDlg.fL.fStat.fR0.lT0    -text $::currentTech(0)] -side right

   pack [frame       .tTechDlg.fL.fStat.fR1] -side top -expand 1 -fill x
   pack [radiobutton .tTechDlg.fL.fStat.fR1.rbWepn -text $nsGui::techName(1) -variable curResearch -value 1] -side left
   pack [label       .tTechDlg.fL.fStat.fR1.lT1    -text $::currentTech(1)] -side right

   pack [frame       .tTechDlg.fL.fStat.fR2] -side top -expand 1 -fill x
   pack [radiobutton .tTechDlg.fL.fStat.fR2.rbProp -text $nsGui::techName(2) -variable curResearch -value 2] -side left
   pack [label       .tTechDlg.fL.fStat.fR2.lT2    -text $::currentTech(2)] -side right

   pack [frame       .tTechDlg.fL.fStat.fR3] -side top -expand 1 -fill x
   pack [radiobutton .tTechDlg.fL.fStat.fR3.rbCons -text $nsGui::techName(3) -variable curResearch -value 3] -side left
   pack [label       .tTechDlg.fL.fStat.fR3.lT3    -text $::currentTech(3)] -side right

   pack [frame       .tTechDlg.fL.fStat.fR4] -side top -expand 1 -fill x
   pack [radiobutton .tTechDlg.fL.fStat.fR4.rbElec -text $nsGui::techName(4) -variable curResearch -value 4] -side left
   pack [label       .tTechDlg.fL.fStat.fR4.lT4    -text $::currentTech(4)] -side right

   pack [frame       .tTechDlg.fL.fStat.fR5] -side top -expand 1 -fill x
   pack [radiobutton .tTechDlg.fL.fStat.fR5.rbBiot -text $nsGui::techName(5) -variable curResearch -value 5] -side left
   pack [label       .tTechDlg.fL.fStat.fR5.lT5    -text $::currentTech(5)] -side right

   pack [labelframe .tTechDlg.fR.fT -text "Currently Researching"] -side top
   pack [label      .tTechDlg.fR.fT.lWhat] -side top
   pack [label      .tTechDlg.fR.fT.lCost -text "Resources needed to complete: 1"] -side top
   pack [label      .tTechDlg.fR.fT.lTime -text "Estimated time to completion: 1 year"] -side top
   pack [frame      .tTechDlg.fR.fT.fNext] -side top
   pack [label      .tTechDlg.fR.fT.fNext.l -text "Next field to research:"] -side left
	pack [combobox::combobox .tTechDlg.fR.fT.fNext.cb -width 13 \
	   -editable false -command setNextResearch]
	foreach t [lsort -integer [array names nsGui::techName]] {
		.tTechDlg.fR.fT.fNext.cb list insert end $nsGui::techName($t)
	}
	.tTechDlg.fR.fT.fNext.cb list insert end "<Same field>"
	.tTechDlg.fR.fT.fNext.cb list insert end "<Lowest field>"
	.tTechDlg.fR.fT.fNext.cb select $::nextResearch

   pack [labelframe .tTechDlg.fR.fB -text "Research Allocation"] -side top
   pack [label      .tTechDlg.fR.fB.lCurRes -text "Annual resources from all planets: 0"] -side top
   pack [label      .tTechDlg.fR.fB.lLast   -text "Total resources spent on research last year: 0"] -side top

   pack [frame      .tTechDlg.fR.fB.fBud] -side top
   pack [label      .tTechDlg.fR.fB.fBud.l -text "Resources budgeted for research:"] -side left
   pack [spinbox    .tTechDlg.fR.fB.fBud.s -from 0 -to 100 -width 3 -command "researchTax 0"] -side right
   bind .tTechDlg.fR.fB.fBud.s <Return>   "researchTax 0"
   bind .tTechDlg.fR.fB.fBud.s <FocusOut> "researchTax 0"
   .tTechDlg.fR.fB.fBud.s set [newStars $::ns_player $::ns_researchTax]

   pack [label      .tTechDlg.fR.fB.lNext   -text "Next year's projected research budget: 0"] -side top

   updateCurResearch curResearch "" write

   focus .tTechDlg
}

proc getPlayRel {} {
   .tPlayRel.fRelation.rbFriend  deselect
   .tPlayRel.fRelation.rbNeutral deselect
   .tPlayRel.fRelation.rbEnemy   deselect

   set sel [.tPlayRel.fPlayers.lb curselection]
   if {$sel ne ""} {
      incr sel
      if {$sel == $nsGui::playerNum} { incr sel }
      set rel [newStars $::ns_player $::ns_playerRelation $sel 0]

      set nsGui::curPlayRel $rel
   }
}

proc setPlayRel {val} {
   set sel [.tPlayRel.fPlayers.lb curselection]
   if {$sel ne ""} {
      incr sel
      if {$sel == $nsGui::playerNum} { incr sel }
      newStars $::ns_player $::ns_playerRelation $sel 1 $nsGui::curPlayRel
   }
}

# create the player relations window
proc doPlayRel {} {
   if [winfo exists .tPlayRel] {
      raise .tPlayRel
      return
   }
   set nsGui::curPlayRel ""

   toplevel .tPlayRel
   wm title .tPlayRel "Player Relations"
   bind .tPlayRel <Escape> "destroy .tPlayRel"

   pack [frame   .tPlayRel.fPlayers] -side left
   pack [label   .tPlayRel.fPlayers.l -text "Player:"] -side top
   pack [listbox .tPlayRel.fPlayers.lb] -side top

   bind .tPlayRel.fPlayers.lb <<ListboxSelect>> getPlayRel

   pack [labelframe  .tPlayRel.fRelation -text "Relation"] -side left
   pack [radiobutton .tPlayRel.fRelation.rbFriend  -text "Friend"  -value 2 -variable nsGui::curPlayRel -command "setPlayRel 2"] -side top
   pack [radiobutton .tPlayRel.fRelation.rbNeutral -text "Neutral" -value 1 -variable nsGui::curPlayRel -command "setPlayRel 1"] -side top
   pack [radiobutton .tPlayRel.fRelation.rbEnemy   -text "Enemy"   -value 0 -variable nsGui::curPlayRel -command "setPlayRel 0"] -side top

   pack [frame  .tPlayRel.fButtons] -side left
   pack [button .tPlayRel.fButtons.bOk   -text "Close" -command "destroy .tPlayRel"] -side top
   pack [button .tPlayRel.fButtons.bHelp -text "Help"] -side top

   for {set i 1} {$i <= $::numPlayers} {incr i} {
      if {$i != $nsGui::playerNum} {
         set pname [newStars $::ns_player $::ns_getName $i]
         if {$pname eq ""} {
            .tPlayRel.fPlayers.lb insert end "Player $i"
         } else {
            .tPlayRel.fPlayers.lb insert end $pname
         }
      }
   }
}

# generate the next turn
proc doGenerate {} {
   doSave
   set hostfile [regsub [join [list "P" $nsGui::playerNum "_Y...."] ""] $nsGui::filename "master"]
	# C++ must run in the directory with all the player files
   cd [file dirname $nsGui::filename]

   newStars $::ns_generate $hostfile

   regexp "_Y(....).xml" $nsGui::filename skip yearNum
   incr yearNum

   set nextTurnFile [regsub [join [list "Y...."] ""] $nsGui::filename "Y$yearNum"]
   set nsGui::filename $nextTurnFile

   doOpen $nextTurnFile
}

proc makeProdQ {} {
   if {[winfo exists .tProd]} { return }

   drawTprod
   buildAvailList
   fillProdQueue .tProd.lbQueue $::planetMap($::walkNum)
   .tProd.lbQueue selection set end
}

proc toggleView {num} {
   if {$nsGui::viewMode == $num} {return}

   set buttons "1 4"
   foreach b $buttons {
      .tGui.fScannerPane.fButtons.b$b configure -relief raised
   }
   .tGui.fScannerPane.fButtons.b$num configure -relief sunken

   set nsGui::viewMode $num

   drawStarMap .tGui [lindex $::maglist $::magnidx]
}

# file menu
proc buildFileMenu {} {
   .mTopMenu.mFile add command -label "New"                -command newGame -underline 0 -accelerator "Ctrl+N"
   .mTopMenu.mFile add command -label "Custom Race Wizard" -command doRace  -underline 7
   .mTopMenu.mFile add command -label "Open"               -command doOpen  -underline 0 -accelerator "Ctrl+O"
   .mTopMenu.mFile add command -label "Close"              -command ""  -underline 0
   .mTopMenu.mFile add command -label "Save"               -command doSave  -underline 0 -accelerator "Ctrl+S"
   .mTopMenu.mFile add command -label "Save And Submit"    -command doSave  -underline 5 -accelerator "Ctrl+A"
   .mTopMenu.mFile add separator
   .mTopMenu.mFile add command -label "Print Map"          -command ""  -underline 0
   .mTopMenu.mFile add separator
   set l [lsort [array names nsGui::prevFiles]]
   if {$l ne ""} {
      foreach i $l {
         .mTopMenu.mFile add command -label "$i $nsGui::prevFiles($i)" -underline 0 -command "doOpen $nsGui::prevFiles($i)"
      }
   .mTopMenu.mFile add separator
   }
   .mTopMenu.mFile add command -label "Exit"               -command exit    -underline 1 -accelerator "Ctrl+Q"
}

#############################################

# gui objects
#############################################
toplevel    .tGui
wm title    .tGui NStars!
wm geometry .tGui 800x700

frame  .tGui.fScannerPane
frame  .tGui.fScannerPane.upper
canvas .tGui.cScannerPane -scrollregion "0 0 400 400" -width 400 -height 540 -bg black \
      -yscrollcommand {.tGui.cScannerPaneV set} -xscrollcommand {.tGui.cScannerPaneH set}
scrollbar .tGui.cScannerPaneV -orient vertical   -command {.tGui.cScannerPane yview}
scrollbar .tGui.cScannerPaneH -orient horizontal -command {.tGui.cScannerPane xview}
pack [frame .tGui.fScannerPane.fButtons] -side top -anchor nw
pack [label .tGui.fScannerPane.fButtons.b1 -image $nsGui::vb1 -bd 1 -relief sunken] -side left
pack [label .tGui.fScannerPane.fButtons.b4 -image $nsGui::vb4 -bd 1 -relief raised] -side left
pack .tGui.cScannerPane  -side left                     -anchor nw -in .tGui.fScannerPane.upper
pack .tGui.cScannerPaneV -side left   -expand 1 -fill y -anchor nw -in .tGui.fScannerPane.upper
pack .tGui.cScannerPaneH -side bottom -expand 1 -fill x -anchor nw -in .tGui.fScannerPane

bind .tGui.fScannerPane.fButtons.b1 <1> "toggleView 1"
bind .tGui.fScannerPane.fButtons.b4 <1> "toggleView 4"
bind .tGui 1 "toggleView 1"
bind .tGui 4 "toggleView 4"

# right context menu
menu .mInSpaceObj -tearoff 0

# main menu
menu .mTopMenu -tearoff 0
menu .mTopMenu.mFile     -tearoff 0
menu .mTopMenu.mView     -tearoff 0
menu .mTopMenu.mTurn     -tearoff 0
menu .mTopMenu.mCommands -tearoff 0
menu .mTopMenu.mReport   -tearoff 0
menu .mTopMenu.mHelp     -tearoff 0

# top level menus
.mTopMenu add cascade -label "File"     -menu .mTopMenu.mFile     -underline 0
.mTopMenu add cascade -label "View"     -menu .mTopMenu.mView     -underline 0
.mTopMenu add cascade -label "Turn"     -menu .mTopMenu.mTurn     -underline 0
.mTopMenu add cascade -label "Commands" -menu .mTopMenu.mCommands -underline 0
.mTopMenu add cascade -label "Report"   -menu .mTopMenu.mReport   -underline 0

buildFileMenu

# zoom menu (part of view)
menu .mTopMenu.mView.mZoom -tearoff 0
.mTopMenu.mView.mZoom add radiobutton -label "25%"  -value 0 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "38%"  -value 1 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "50%"  -value 2 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "75%"  -value 3 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "100%" -value 4 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "125%" -value 5 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "150%" -value 6 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "200%" -value 7 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}
.mTopMenu.mView.mZoom add radiobutton -label "400%" -value 8 -variable magnidx \
-command {drawStarMap .tGui [lindex $maglist $magnidx]}

# view menu
.mTopMenu.mView add checkbutton -label "Toolbar"
.mTopMenu.mView add separator
.mTopMenu.mView add command -label "Find..." -command "" -underline 0 -accelerator "Ctrl+F"
.mTopMenu.mView add cascade -label "Zoom" -menu .mTopMenu.mView.mZoom -underline 0
.mTopMenu.mView add separator
.mTopMenu.mView add command -label "Race..." -accelerator "F8" -command "doRace 1"

# turn menu
.mTopMenu.mTurn add command -label "Generate" -command doGenerate -accelerator "F9" -underline 0

# commands menu
.mTopMenu.mCommands add command -label "Ship Design..."      -underline 0 -accelerator "F4" -command shipDesigner
.mTopMenu.mCommands add command -label "Research..."         -underline 0 -accelerator "F5" -command doTech
.mTopMenu.mCommands add command -label "Battle Plans..."     -underline 0 -accelerator ""   -command {}
.mTopMenu.mCommands add command -label "Player Relations..." -underline 0 -accelerator "F7" -command doPlayRel
.mTopMenu.mCommands add command -label "Change Password..."  -underline 0                   -command {}

# report menu
.mTopMenu.mReport add command -label "Planets..."      -underline 0 -accelerator "F3" -command showPlanets
.mTopMenu.mReport add command -label "Fleets..."       -underline 0 -accelerator "F3" -command {}
.mTopMenu.mReport add command -label "Enemy Fleets..." -underline 0 -accelerator "F3" -command {}
.mTopMenu.mReport add separator
.mTopMenu.mReport add command -label "Battles..."      -underline 0 -accelerator "F3" -command showBattles
.mTopMenu.mReport add separator
.mTopMenu.mReport add command -label "Score..."        -underline 0 -accelerator "F10" -command {}
.mTopMenu.mReport add separator
#.mTopMenu.mReport add command -label "" -underline 0 -accelerator "F3" -command {}

bind .tGui n          nextWalk
bind .tGui p          prevWalk
bind .tGui q          makeProdQ
bind .tGui <Control-n> newGame
bind .tGui <Control-o> doOpen
bind .tGui <Control-s> doSave
bind .tGui <Control-a> doSave
bind .tGui <Control-f> ""
bind .tGui <Control-q> exit
bind .tGui <F4>        shipDesigner
bind .tGui <F5>        doTech
bind .tGui <F7>        doPlayRel
bind .tGui <F8>        "doRace 1"
bind .tGui <F9>        doGenerate

#############################################
# pack menus
.tGui configure -menu .mTopMenu

# frames
#############################################
frame .tGui.fLeftSide
frame .tGui.fLeftLeft
frame .tGui.fLeftRight
#############################################
frame .tGui.fMsg

pack [frame  .tGui.fMsg.fb] -side right -anchor ne
pack [button .tGui.fMsg.fb.bPrev -text "Prev" -command prvMsg] -side top
pack [button .tGui.fMsg.fb.bGoto -text "Goto"] -side top
pack [button .tGui.fMsg.fb.bNext -text "Next" -command nxtMsg] -side top

pack [frame .tGui.fMsg.msgPaneHdr -bd 5 -relief ridge -height 50]
pack [label .tGui.fMsg.msgPaneHdr.lYear -text "Please open a valid turn file" -width 54] \
   -side top -anchor n -expand 1 -fill x

pack [frame .tGui.fMsg.msgPaneBdy -bd 5 -relief ridge -height 200]
pack [label .tGui.fMsg.msgPaneBdy.lMsg -textvariable nsGui::msgTxt -width 54 -wraplength 300] \
   -side top -anchor n

#############################################
frame  .tGui.fFleetLocation
label  .tGui.fFleetLocation.lFLname -text "Orbiting Somewhere" -relief raised -bd 1 -width 25
button .tGui.fFleetLocation.bGoto   -text Goto -command {
   set fleetId $::allFleetMap($::walkNum)
   set location [newStars $::ns_planet $fleetId $::ns_getPlanetId]
   if {$location eq ""} { return }

   set walkNum    [findPlanetWalkFromId $location]
   set nsGui::walkPlanet 1
}

#############################################
frame  .tGui.pfWalker -height 40 -bd 1 -relief raised
label  .tGui.pfWalker.lName -text "Stinky Socks" -width 25 -bd 1 -relief raised -anchor w
label  .tGui.pfWalker.lPic  -image $planImage -bd 2 -relief sunken
button .tGui.pfWalker.bPrev -text Prev   -width 8 -command prevWalk
button .tGui.pfWalker.bNext -text Next   -width 8 -command nextWalk
button .tGui.pfWalker.bRen  -text Rename -width 8

#############################################
combobox::combobox .tGui.lbFleetList -textvariable fleetSelect -editable false -command updateViewFleet \
   -width 32

#############################################
frame  .tGui.fLeftTopRight -height 45 -bd 1 -relief raised
label  .tGui.lName  -text "Fleets in Orbit" -bd 1 -relief raised -width 35
label  .tGui.fLeftTopRight.lFuel  -textvariable ltrFuel -width 35
label  .tGui.fLeftTopRight.lCargo -textvariable ltrCargo
frame  .tGui.fLeftTopRight.fButtons

# on planet, goto fleet in orbit indicated by combobox
button .tGui.fLeftTopRight.fButtons.bGoto -text Goto -command {
   set idx [.tGui.lbFleetList curselection]
   if {$idx eq ""} {set idx 0}

   set planetId $::planetMap($::walkNum)
   set numFleetsHere [newStars $::ns_planet $planetId $::ns_getNumFleets]
   if {$idx < $numFleetsHere} {
      set fleetId [newStars $::ns_planet $planetId $::ns_getFleetIdFromIndex $idx]
      set ::walkNum [findFleetWalkFromId $fleetId]
	   set nsGui::walkPlanet 0
   }
}

button .tGui.fLeftTopRight.fButtons.bCargo -text Cargo -command {
}

#############################################
frame .tGui.fLeftTopRight.fIron
pack [label .tGui.fLeftTopRight.fIron.lIronT   -text "Ironium" -fg blue] -side left
pack [label .tGui.fLeftTopRight.fIron.lIron    -textvariable ltrIron] -side right

frame .tGui.fLeftTopRight.fBoron
pack [label .tGui.fLeftTopRight.fBoron.lBoronT -text "Boranium" -fg green4] -side left
pack [label .tGui.fLeftTopRight.fBoron.lBoron  -textvariable ltrBoron] -side right

frame .tGui.fLeftTopRight.fGerm
pack [label .tGui.fLeftTopRight.fGerm.lGermT   -text "Germanium" -fg yellow4] -side left
pack [label .tGui.fLeftTopRight.fGerm.lGerm    -textvariable ltrGerm] -side right

frame .tGui.fLeftTopRight.fCol
pack [label .tGui.fLeftTopRight.fCol.lColT     -text "Colonists" -fg #808080] -side left
pack [label .tGui.fLeftTopRight.fCol.lCol      -textvariable ltrCol] -side right

#############################################
frame .tGui.fPStatus -bd 1 -relief raised
pack [label .tGui.fPStatus.lTitle -text "Status" -bd 1 -relief raised] -side top -expand 1 -fill x

frame .tGui.fPStatus.fPop
pack [label .tGui.fPStatus.fPop.lT -text "Population"] -side left
pack [label .tGui.fPStatus.fPop.lV -text "0"         ] -side right
pack  .tGui.fPStatus.fPop -side top -expand 1 -fill x

frame .tGui.fPStatus.fRsc
pack [label .tGui.fPStatus.fRsc.lT -text "Resources/Year"] -side left
pack [label .tGui.fPStatus.fRsc.lV -text "0 of 0"        ] -side right
pack  .tGui.fPStatus.fRsc -side top -expand 1 -fill x

frame .tGui.fPStatus.fScan
pack [label .tGui.fPStatus.fScan.lT -text "Scanner Type"] -side left
pack [label .tGui.fPStatus.fScan.lV -text "None"        ] -side right
pack  .tGui.fPStatus.fScan -side top -expand 1 -fill x

frame .tGui.fPStatus.fSRan
pack [label .tGui.fPStatus.fSRan.lT -text "Scanner Range"] -side left
pack [label .tGui.fPStatus.fSRan.lV -text "None"         ] -side right
pack  .tGui.fPStatus.fSRan -side top -expand 1 -fill x

frame .tGui.fPStatus.fDef
pack [label .tGui.fPStatus.fDef.lT -text "Defenses"] -side left
pack [label .tGui.fPStatus.fDef.lV -text "0 of 1"  ] -side right
pack  .tGui.fPStatus.fDef -side top -expand 1 -fill x

frame .tGui.fPStatus.fDType
pack [label .tGui.fPStatus.fDType.lT -text "Defense Type"] -side left
pack [label .tGui.fPStatus.fDType.lV -text "None"        ] -side right
pack  .tGui.fPStatus.fDType -side top -expand 1 -fill x

frame .tGui.fPStatus.fDCov
pack [label .tGui.fPStatus.fDCov.lT -text "Def Coverage"] -side left
pack [label .tGui.fPStatus.fDCov.lV -text "None"        ] -side right
pack  .tGui.fPStatus.fDCov -side top -expand 1 -fill x
#############################################

#############################################
frame   .tGui.fProduction -bd 1 -relief raised
label   .tGui.fProduction.lTitle -text "Production" -width 25 -bd 1 -relief raised
listbox .tGui.fProduction.lbPlanetQueue -height 5 -selectmode single -exportselection 0
button  .tGui.fProduction.bChange -text "Change" -command makeProdQ

#############################################
frame .tGui.fFleetComp -bd 1 -relief raised
pack [label .tGui.fFleetComp.lT -text "Fleet Composition" -width 35 -bd 1 -relief raised] -side top
pack [tablelist::tablelist .tGui.fFleetComp.lb -columns {0 "" 0 ""} -width 35 -height 6] -side top
pack [frame .tGui.fFleetComp.fBattPlan] -side top -expand 1 -fill x
pack [frame .tGui.fFleetComp.fRange]    -side top -expand 1 -fill x
pack [frame .tGui.fFleetComp.fCloak]    -side top -expand 1 -fill x
pack [frame .tGui.fFleetComp.fButtons]  -side top -expand 1 -fill x

pack [label              .tGui.fFleetComp.fBattPlan.lT -text "Battle Plan:"] -side left
pack [combobox::combobox .tGui.fFleetComp.fBattPlan.cb -width 12 -editable false] -side right

pack [label .tGui.fFleetComp.fRange.lT -text "Est. Range"] -side left
pack [label .tGui.fFleetComp.fRange.lV -text "0 l.y."] -side right

pack [label .tGui.fFleetComp.fCloak.lT -text "Percent Cloaked"] -side left
pack [label .tGui.fFleetComp.fCloak.lV -text "None"] -side right

pack [button .tGui.fFleetComp.fButtons.bSplit  -text "Split" -command fleetSplit
     ] -side left
pack [button .tGui.fFleetComp.fButtons.bSplall -text "Split All"] -side left
pack [button .tGui.fFleetComp.fButtons.bMerge  -text "Merge Fleets"] -side right

#############################################
frame .tGui.fOtherFleet -bd 1 -relief raised
pack [label .tGui.fOtherFleet.lT -text "Other Fleets Here" -width 35 -bd 1 -relief raised] -side top
pack [combobox::combobox .tGui.fOtherFleet.cb -width 38 -editable false] -side top
pack [frame .tGui.fOtherFleet.fFuel] -side top -expand 1 -fill x
pack [label .tGui.fOtherFleet.fFuel.l -text "Fuel"] -side left
pack [frame .tGui.fOtherFleet.fCargo] -side top -expand 1 -fill x
pack [label .tGui.fOtherFleet.fCargo.l -text "Cargo"] -side left

pack [frame  .tGui.fOtherFleet.fBut] -side top -expand 1 -fill x
pack [button .tGui.fOtherFleet.fBut.bGoto  -text "Goto" -command gotoOther] -side left
pack [button .tGui.fOtherFleet.fBut.bMerge -text "Merge Ships" -command fleetToFleetMerge
     ] -side left -padx 50
pack [button .tGui.fOtherFleet.fBut.bCargo -text "Cargo" -command cargoXfer] -side left

#############################################
frame   .tGui.fWaypoints -bd 1 -relief raised
label   .tGui.fWaypoints.lTitle -text "Fleet Waypoints" -width 25 -bd 1 -relief raised
listbox .tGui.fWaypoints.lbWaypoints -height 4 -selectmode single -exportselection 0
bind .tGui.fWaypoints.lbWaypoints <<ListboxSelect>> hndWayPtSel

frame .tGui.fWayDetails -bd 1 -relief raised
pack [frame   .tGui.fWayDetails.fWarp] -expand 1 -fill x
pack [label   .tGui.fWayDetails.fWarp.l -text "Warp Factor"] -side left
pack [spinbox .tGui.fWayDetails.fWarp.s -from 0 -to 11 -width 2 -command fleetSpeed] -side right

#############################################
frame   .tGui.fWayPtTask -bd 1 -relief raised
pack [label .tGui.fWayPtTask.lTitle -text "Waypoint Task" -width 25 -bd 1 -relief raised]
pack [combobox::combobox .tGui.fWayPtTask.cbTask -editable false -command selectWPT]
.tGui.fWayPtTask.cbTask list insert 0 "(no task here)" "Unload Cargo" "Load Cargo" "Colonize" \
    "Remote Mining" "Merge with Fleet" "Scrap Fleet" "Lay Mine Field" "Patrol" \
    "Route" "Transfer Fleet"

#############################################
pack [frame .tGui.fWayPtTask.fExtras]
combobox::combobox .tGui.fWayPtTask.fExtras.cbTranWhich -editable false -command selectTW
.tGui.fWayPtTask.fExtras.cbTranWhich list insert 0 "Fuel" "Ironium" "Boranium" "Germanium" "Colonists"
combobox::combobox .tGui.fWayPtTask.fExtras.cbAction -editable false -command selectTWA
.tGui.fWayPtTask.fExtras.cbAction list insert 0 "(no action)" "Load All Available" "Unload All" \
    "Load Exactly..." "Unload Exactly..." "Fill Up to %..." "Wait for %..." "Load Dunnage" \
    "Set Amount to..." "Set Waypoint to..."
entry .tGui.fWayPtTask.fExtras.eAmt -vcmd "wptEntry %P" -validate key
combobox::combobox .tGui.fWayPtTask.fExtras.cbTgtPlayer -editable false -command selectTFP

#############################################
frame .tGui.fMinerals -bd 1 -relief raised
pack [label .tGui.fMinerals.lTitle -text "Minerals On Hand" -bd 1 -relief raised] -expand 1 -fill x

frame .tGui.fMinerals.fIron
pack [label .tGui.fMinerals.fIron.lText -text "Ironium" -fg blue] -side left
pack [label .tGui.fMinerals.fIron.lVal  -text "0 kt"] -side right
pack  .tGui.fMinerals.fIron -expand 1 -fill x

frame .tGui.fMinerals.fBoron
pack [label .tGui.fMinerals.fBoron.lText -text "Boranium" -fg green4] -side left
pack [label .tGui.fMinerals.fBoron.lVal  -text "0 kt"] -side right
pack  .tGui.fMinerals.fBoron -expand 1 -fill x

frame .tGui.fMinerals.fGerm
pack [label .tGui.fMinerals.fGerm.lText -text "Germanium" -fg yellow4] -side left
pack [label .tGui.fMinerals.fGerm.lVal  -text "0 kt"] -side right
pack  .tGui.fMinerals.fGerm -expand 1 -fill x


frame .tGui.fMinerals.fInfra -bd 1 -relief raised

frame .tGui.fMinerals.fInfra.fMines
label .tGui.fMinerals.fInfra.fMines.lTitle -text "Mines"
label .tGui.fMinerals.fInfra.fMines.lText  -text "0 of 0"
pack  .tGui.fMinerals.fInfra.fMines.lTitle -side left
pack  .tGui.fMinerals.fInfra.fMines.lText  -side right
pack  .tGui.fMinerals.fInfra.fMines -side top -expand 1 -fill x

frame .tGui.fMinerals.fInfra.fFactories
label .tGui.fMinerals.fInfra.fFactories.lTitle -text "Factories"
label .tGui.fMinerals.fInfra.fFactories.lText  -text "0 of 0"
pack  .tGui.fMinerals.fInfra.fFactories.lTitle -side left
pack  .tGui.fMinerals.fInfra.fFactories.lText  -side right
pack  .tGui.fMinerals.fInfra.fFactories -side top -expand 1 -fill x

pack .tGui.fMinerals.fInfra -side top -expand 1 -fill x
#############################################

#############################################
# packing
pack .tGui.pfWalker.lName   -side top   -anchor n
pack .tGui.pfWalker.lPic    -side left  -anchor w -padx 10 -pady 5
pack .tGui.pfWalker.bPrev   -side top -anchor n -pady 5
pack .tGui.pfWalker.bNext   -side top -anchor n
pack .tGui.fLeftTopRight.lFuel  -side top -anchor n
pack .tGui.fLeftTopRight.lCargo -side top -anchor n
pack .tGui.fLeftTopRight.fButtons -side top -expand 1 -fill x
pack .tGui.fLeftTopRight.fButtons.bGoto  -side left
pack .tGui.fLeftTopRight.fButtons.bCargo -side right
pack .tGui.fProduction.lTitle        -side top -anchor n
pack .tGui.fProduction.lbPlanetQueue -side top
pack .tGui.fProduction.bChange       -side top -anchor nw

pack .tGui.fFleetLocation.lFLname -side top
pack .tGui.fFleetLocation.bGoto   -side top

pack .tGui.fWaypoints.lTitle -side top
pack .tGui.fWaypoints.lbWaypoints

pack .tGui.pfWalker      -in .tGui.fLeftLeft  -side top -anchor nw
pack .tGui.fMinerals     -in .tGui.fLeftLeft  -side top -anchor nw -expand 1 -fill x
pack .tGui.fPStatus      -in .tGui.fLeftLeft  -side top -anchor nw -expand 1 -fill x
pack .tGui.lName         -in .tGui.fLeftRight -side top -anchor n
pack .tGui.lbFleetList   -in .tGui.fLeftRight -side top -anchor nw
pack .tGui.fLeftTopRight -in .tGui.fLeftRight -side top -anchor ne
pack .tGui.fProduction   -in .tGui.fLeftRight -side top -anchor n
#############################################
pack .tGui.fMsg          -in .tGui.fLeftSide -side bottom -anchor sw -expand 1 -fill x
pack .tGui.fLeftLeft     -in .tGui.fLeftSide -side left   -anchor nw
pack .tGui.fLeftRight    -in .tGui.fLeftSide -side right  -anchor ne

frame .tGui.fLoc
label .tGui.fLoc.lId -text "ID #0"
label .tGui.fLoc.lX  -text "X: 0"
label .tGui.fLoc.lY  -text "Y: 0"
label .tGui.fLoc.lN  -text "Here"
label .tGui.fLoc.lDistTxt -text "0.0 light years from here"

pack .tGui.fLoc.lDistTxt -side bottom

pack .tGui.fLoc -side bottom -in .tGui.fScannerPane
pack .tGui.fLoc.lId -side left
pack .tGui.fLoc.lX  -side left
pack .tGui.fLoc.lY  -side left
pack .tGui.fLoc.lN  -side left

# bottom right info pane (shows selected item from galaxy pane: planet, fleet, etc)
frame .tGui.fBottomRight
label .tGui.fBottomRight.lT -text "Summary" -bd 1 -relief raised
pack  .tGui.fBottomRight.lT -expand 1 -fill x -side top
label .tGui.fBottomRight.lPic -image $::shipImage

pack [frame .tGui.fBottomRight.fPlanet]
frame .tGui.fBottomRight.fPlanet.fRow1
pack [label .tGui.fBottomRight.fPlanet.fRow1.lVal -text "Value: 100%"] -side left
pack [label .tGui.fBottomRight.fPlanet.fRow1.lPop -text "Population: 100,000"] -side right

frame .tGui.fBottomRight.fPlanet.fRow2
pack [label .tGui.fBottomRight.fPlanet.fRow2.lAge -text "Report is current"] -side left

frame .tGui.fBottomRight.fPlanet.fRow3
grid [label  .tGui.fBottomRight.fPlanet.fRow3.lGrav -text "Gravity" -font $labelBoldFont] -row 0 -column 0 -sticky e
grid [canvas .tGui.fBottomRight.fPlanet.fRow3.c0    -width $updateBottomRight::width -height 20 -bg black]  -row 0 -column 1
grid [label  .tGui.fBottomRight.fPlanet.fRow3.lVal0 -text "5.60 g"] -row 0 -column 2 -sticky e

grid [label  .tGui.fBottomRight.fPlanet.fRow3.lTemp -text "Temperature" -font $labelBoldFont] -row 1 -column 0 -sticky e
grid [canvas .tGui.fBottomRight.fPlanet.fRow3.c1    -width $updateBottomRight::width -height 20 -bg black] -row 1 -column 1
grid [label  .tGui.fBottomRight.fPlanet.fRow3.lVal1 -text "36 C"] -row 1 -column 2 -sticky e

grid [label  .tGui.fBottomRight.fPlanet.fRow3.lRad  -text "Radiation" -font $labelBoldFont] -row 2 -column 0 -sticky e
grid [canvas .tGui.fBottomRight.fPlanet.fRow3.c2    -width $updateBottomRight::width -height 20 -bg black] -row 2 -column 1
grid [label  .tGui.fBottomRight.fPlanet.fRow3.lVal2 -text "63 mR"] -row 2 -column 2 -sticky e

grid [label  .tGui.fBottomRight.fPlanet.fRow3.lIron -text "Ironium" -font $labelBoldFont] -row 4 -column 0 -sticky e
grid [canvas .tGui.fBottomRight.fPlanet.fRow3.c3    -width $updateBottomRight::width -height 20 -bg black] -row 4 -column 1

grid [label  .tGui.fBottomRight.fPlanet.fRow3.lBoron -text "Boranium" -font $labelBoldFont] -row 5 -column 0 -sticky e
grid [canvas .tGui.fBottomRight.fPlanet.fRow3.c4    -width $updateBottomRight::width -height 20 -bg black] -row 5 -column 1

grid [label  .tGui.fBottomRight.fPlanet.fRow3.lGerm -text "Germanium" -font $labelBoldFont] -row 6 -column 0 -sticky e
grid [canvas .tGui.fBottomRight.fPlanet.fRow3.c5    -width $updateBottomRight::width -height 20 -bg black] -row 6 -column 1

pack .tGui.fBottomRight.fPlanet.fRow1 -side top -expand 1 -fill x
pack .tGui.fBottomRight.fPlanet.fRow2 -side top -expand 1 -fill x
pack .tGui.fBottomRight.fPlanet.fRow3 -side top -expand 1 -fill x

#############################################
pack .tGui.fLeftSide     -side left   -expand 1 -fill y -anchor sw
pack .tGui.fScannerPane.upper -side top -anchor nw
pack .tGui.fScannerPane       -side top -anchor nw
pack .tGui.fBottomRight  -side bottom -expand 1 -fill x -anchor sw -in .tGui.fLeftSide -before .tGui.fMsg
#############################################

# trap exit to save settings and clean up
rename exit origExit
proc exit {} {
   set f [open [file join ~ $::resourceFileName] w]
   foreach n [lsort -integer [array names nsGui::prevFiles]] {
      puts $f "set nsGui::prevFiles($n) \{$nsGui::prevFiles($n)\}"
   }
   close $f
	if {$::inStarkit} {
		catch {file delete -force newStarsComponents.xml}
		catch {file delete -force newStarsHulls.xml}
		catch {file delete -force names.txt}
	}
   origExit
}

# bindings
#############################################
pack [frame .tGui.secretFrame]
bind .tGui.secretFrame <Destroy> {
   exit
}

#bind . <KeyRelease-Shift_L> { set nsGui::shftDn 0 }
#bind . <KeyPress-Shift_L>   { set nsGui::shftDn 1; puts "Down" }
#bind . <KeyRelease-Shift_R> { set nsGui::shftDn 0 }
#bind . <KeyPress-Shift_R>   { set nsGui::shftDn 1; puts "Down" }

bind .tGui  <Key-Delete> "deleteOrder all"

proc zoomIn {} {
   if {$::magnidx < [expr [llength $::maglist] - 1]} {
      incr ::magnidx
      set curV [lindex [.tGui.cScannerPaneV get] 0]
      set curH [lindex [.tGui.cScannerPaneH get] 0]
      drawStarMap .tGui [lindex $::maglist $::magnidx]
      .tGui.cScannerPane yview moveto $curV
      .tGui.cScannerPane xview moveto $curH
   }
}

bind .tGui <Key-plus>  zoomIn
bind .tGui <KP_Add>    zoomIn

proc zoomOut {} {
   if {$::magnidx > 0} {
      incr ::magnidx -1
      set curV [lindex [.tGui.cScannerPaneV get] 0]
      set curH [lindex [.tGui.cScannerPaneH get] 0]
      drawStarMap .tGui [lindex $::maglist $::magnidx]
      .tGui.cScannerPane yview moveto $curV
      .tGui.cScannerPane xview moveto $curH
   }
}

bind .tGui <Key-minus>       zoomOut
bind .tGui <Key-KP_Subtract> zoomOut

proc isCloseEnough {px py sx sy} {
   set zoom [lindex $::maglist $::magnidx]
   set margin [expr int(6 / $zoom)]

   if {(abs($px - $sx) < $margin) && (abs($sy - $py) < $margin)} {
      return 1
   }
   return 0
}

label .tGui.lMineralPopup -text "None" -background white -bd 1 -relief solid -padx 5

proc displayMineralPopup {mineral x y} {
   set planetId $::planetMap($::walkNum)
   set planXML [xml2list [newStars $::ns_planet $planetId  $::ns_getXML]]
   set mainXML [lindex $planXML 2]

   set raceXML [lindex [xml2list [newStars $::ns_race $::ns_getXML]] 2]
   set mineEff [lindex [findXMLbyTag "MINEOUTPUT" $raceXML 2] 1]

   set infraTuple [lindex [findXMLbyTag "INFRASTRUCTURE"    $mainXML 2] 1]
   set infraMax   [lindex [findXMLbyTag "MAXINFRASTRUCTURE" $mainXML 2] 1]
   set effMines [lindex $infraTuple 0]
   set maxMines [lindex $infraMax   0]
   if {$effMines > $maxMines} { set effMines $maxMines }

   set isHomeworld [lindex [findXMLbyTag "IS_HOMEWORLD" $mainXML 2] 1]

   set cargoXML [findXMLbyTag "MINERALCONCENTRATIONS" $mainXML 2]
   if {[lindex $cargoXML 0] ne "#text"} { return }

   set tuple [lindex $cargoXML 1]

   switch $mineral {
      Ironium {
         set amt [.tGui.fMinerals.fIron.lVal cget -text]
         set cnt [lindex $tuple 0]
      }
      Boranium {
         set amt [.tGui.fMinerals.fBoron.lVal cget -text]
         set cnt [lindex $tuple 1]
      }
      Germanium {
         set amt [.tGui.fMinerals.fGerm.lVal cget -text]
         set cnt [lindex $tuple 2]
      }
   }
   if {$isHomeworld && $cnt < 30} {set cnt 30}

   set rte "[expr ($cnt * $effMines * $mineEff + 500) /1000] kt/yr"

   append minText $mineral "\n" "On surface: " $amt "\n"
   append minText "Mineral Concentration: " $cnt
   if {$isHomeworld} {append minText " (HW)"}
   append minText "\n"
   append minText "Mining Rate: " $rte

   .tGui.lMineralPopup configure -text $minText

   set nx [expr $x - [winfo rootx .tGui]]
   set ny [expr $y - [winfo rooty .tGui]]
   place .tGui.lMineralPopup -anchor sw -x $nx -y $ny
}

bind .tGui.fMinerals.fIron       <Button-1>        { displayMineralPopup "Ironium"  %X %Y }
bind .tGui.fMinerals.fIron.lText <Button-1>        { displayMineralPopup "Ironium"  %X %Y }
bind .tGui.fMinerals.fIron.lVal  <Button-1>        { displayMineralPopup "Ironium"  %X %Y }
bind .tGui.fMinerals.fIron       <ButtonRelease-1> { place forget .tGui.lMineralPopup }
bind .tGui.fMinerals.fIron.lText <ButtonRelease-1> { place forget .tGui.lMineralPopup }
bind .tGui.fMinerals.fIron.lVal  <ButtonRelease-1> { place forget .tGui.lMineralPopup }

bind .tGui.fMinerals.fBoron       <Button-1>        { displayMineralPopup "Boranium" %X %Y }
bind .tGui.fMinerals.fBoron.lText <Button-1>        { displayMineralPopup "Boranium" %X %Y }
bind .tGui.fMinerals.fBoron.lVal  <Button-1>        { displayMineralPopup "Boranium" %X %Y }
bind .tGui.fMinerals.fBoron       <ButtonRelease-1> { place forget .tGui.lMineralPopup }
bind .tGui.fMinerals.fBoron.lText <ButtonRelease-1> { place forget .tGui.lMineralPopup }
bind .tGui.fMinerals.fBoron.lVal  <ButtonRelease-1> { place forget .tGui.lMineralPopup }

bind .tGui.fMinerals.fGerm       <Button-1>        { displayMineralPopup "Germanium" %X %Y }
bind .tGui.fMinerals.fGerm.lText <Button-1>        { displayMineralPopup "Germanium" %X %Y }
bind .tGui.fMinerals.fGerm.lVal  <Button-1>        { displayMineralPopup "Germanium" %X %Y }
bind .tGui.fMinerals.fGerm       <ButtonRelease-1> { place forget .tGui.lMineralPopup }
bind .tGui.fMinerals.fGerm.lText <ButtonRelease-1> { place forget .tGui.lMineralPopup }
bind .tGui.fMinerals.fGerm.lVal  <ButtonRelease-1> { place forget .tGui.lMineralPopup }

bind .tGui <Configure> {
   set scanPaneWidth [expr [winfo width .tGui] - [winfo width .tGui.fBottomRight] - 20]
   .tGui.cScannerPane configure -width $scanPaneWidth

   set scanPaneHeight [expr [winfo height .tGui] - [winfo height .tGui.fLoc] - 20]
   .tGui.cScannerPane configure -height $scanPaneHeight
}

bind .tGui.cScannerPane <Shift-Button-1> {
   if {$nsGui::walkPlanet} {
   } else {
      set sel [.tGui.fWaypoints.lbWaypoints curselection]
      if {$sel eq ""} {
         set sel [expr [.tGui.fWaypoints.lbWaypoints size] - 1]
      }

      set fleetId $::allFleetMap($::walkNum)

      set cx   [.tGui.cScannerPane canvasx %x]
      set cy   [.tGui.cScannerPane canvasy %y]
      set zoom [lindex $::maglist $::magnidx]
      set sx [expr int($cx / $zoom)]
      set sy [expr int($cy / $zoom)]

      set nearPlanetCIDs [ljoin [.tGui.cScannerPane find overlapping [expr $cx - 3] [expr $cy - 3] [expr $cx + 3] [expr $cy + 3]] [array names ::planetMapId]]
      if {$nearPlanetCIDs ne ""} {
         set panePlanetId $::planetMapId([lindex $nearPlanetCIDs 0])

         set px [newStars $::ns_planet $panePlanetId $::ns_getX]
         set py [newStars $::ns_planet $panePlanetId $::ns_getY]

         set closeEnough [isCloseEnough $px $py $sx $sy]
      } else {
         set closeEnough 0
      }

      if {$closeEnough} {
         newStars $::ns_planet $fleetId $::ns_addAtIndex $sel $::ns_orderMove $px $py
      } else {
         newStars $::ns_planet $fleetId $::ns_addAtIndex $sel $::ns_orderMove $sx $sy
      }

      .tGui.cScannerPane delete path$fleetId
      drawPaths [lindex $::maglist $::magnidx] $fleetId

      updateFleet
   }
}

proc rightMouseCmdPlan {planetId} {
   set ::walkNum    [findPlanetWalkFromId $planetId]
   set nsGui::walkPlanet 1
   updatePlanet $::walkNum
}

proc rightMouseCmdFleet {fleetId} {
   set tmpWalk [findFleetWalkFromId $fleetId]
   if {$tmpWalk eq ""} {
      # probably an enemy fleet
      updateBottomRight EFLEET $fleetId
      return
   }
   set ::walkNum $tmpWalk
   set nsGui::walkPlanet 0
   updateFleet
}

.tGui.cScannerPane bind inspaceobj <Button-1> {
   set cx           [.tGui.cScannerPane canvasx %x]
   set cy           [.tGui.cScannerPane canvasy %y]
   set canvasId     [.tGui.cScannerPane find closest $cx $cy 4]
   set panePlanetId $::planetMapId($canvasId)
   set what         $::paneMapName($canvasId)

   updateBottomRight $what $panePlanetId
}

proc doButton3 {w x y} {
   set cx           [.tGui.cScannerPane canvasx $x]
   set cy           [.tGui.cScannerPane canvasy $y]
   set canvasId [.tGui.cScannerPane find closest $cx $cy 4]

   set panePlanetId $::planetMapId($canvasId)
   set panePlanet   [newStars $::ns_planet $panePlanetId $::ns_getName]

   .mInSpaceObj delete 0 last

   if {$::paneMapName($canvasId) eq "PLANET"} {
      .mInSpaceObj add command -label $panePlanet -command "rightMouseCmdPlan $panePlanetId"

      set nF [newStars $::ns_planet $panePlanetId $::ns_getNumFleets]
      if {$nF} { .mInSpaceObj add separator }
      for {set i 0} {$i < $nF} {incr i} {
         set fleetId [newStars $::ns_planet $panePlanetId $::ns_getFleetIdFromIndex $i]
         set fleetName [newStars $::ns_planet $fleetId $::ns_getName]
         .mInSpaceObj add command -label $fleetName -command "rightMouseCmdFleet $fleetId"
      }

   } elseif {$::paneMapName($canvasId) eq "FLEET"} {
      set nF [newStars $::ns_planet $panePlanetId $::ns_getNumFleets]
      for {set i 0} {$i < $nF} {incr i} {
         set fleetId [newStars $::ns_planet $panePlanetId $::ns_getFleetIdFromIndex $i]
         set fleetName [newStars $::ns_planet $fleetId $::ns_getName]
         .mInSpaceObj add command -label $fleetName -command "rightMouseCmdFleet $fleetId"
      }
   } elseif {$::paneMapName($canvasId) eq "MINEFIELD"} {
      .mInSpaceObj add command -label $panePlanet -command "updateBottomRight MINEFIELD $panePlanetId"
   } elseif {$::paneMapName($canvasId) eq "EFLEET"} {
      .mInSpaceObj add command -label $panePlanet -command "updateBottomRight FLEET $panePlanetId"
   }

   set nx [expr [winfo rootx $w] + $x]
   set ny [expr [winfo rooty $w] + $y]
   tk_popup .mInSpaceObj $nx $ny
}

.tGui.cScannerPane bind inspaceobj <Button-3> { doButton3 %W %x %y }
#############################################
