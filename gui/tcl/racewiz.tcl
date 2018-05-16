namespace eval raceWiz {
   set step     1
   set prerace  0
   set points  25

   set grvCtr 50
   set tmpCtr 50
   set radCtr 50

   set grvRng 10
   set tmpRng 10
   set radRng 10

   set growthRt 15

   set resPop     1000
   set resFact    10
   set costFact   10
   set numFact    10
   set factOneLess 0
   set effMine    10
   set costMine    5
   set numMine    10

   set prmTrait 0
   set lrtAry(0)  0
   set lrtAry(1)  0
   set lrtAry(2)  0
   set lrtAry(3)  0
   set lrtAry(4)  0
   set lrtAry(5)  0
   set lrtAry(6)  0
   set lrtAry(7)  0
   set lrtAry(8)  0
   set lrtAry(9)  0
   set lrtAry(10) 0
   set lrtAry(11) 0
   set lrtAry(12) 0
   set lrtAry(13) 0

   set techCst(0) 1; # energy
   set techCst(1) 1; # weapons
   set techCst(2) 1; # prop
   set techCst(3) 1; # con
   set techCst(4) 1; # elec
   set techCst(5) 1; # bio
   set techStart3 0

   set envColor(0) "#00007F"
   set envColor(1) "#7F0000"
   set envColor(2) "#007F00"
}

proc racePackButtons {} {
   pack .tRaceWiz.fBRow -side bottom -expand 1 -fill x
   pack .tRaceWiz.fBRow.bHelp   -side left -expand 1
   pack .tRaceWiz.fBRow.bCancel -side left -expand 1
   pack .tRaceWiz.fBRow.bBack   -side left -expand 1
   pack .tRaceWiz.fBRow.bNext   -side left -expand 1
   pack .tRaceWiz.fBRow.bFinish -side left -expand 1
}

proc racePage1 {} {
   bind .tRaceWiz <Alt-a> {}

   pack .tRaceWiz.fPoints.lTxt -side left
   pack .tRaceWiz.fPoints.lVal -side right
   pack .tRaceWiz.fPoints -side right -anchor ne

   pack .tRaceWiz.fName.lTxt -side left
   pack .tRaceWiz.fName.eVal -side right
   pack .tRaceWiz.fName -side top

   pack .tRaceWiz.fPlural.lTxt -side left
   pack .tRaceWiz.fPlural.eVal -side right
   pack .tRaceWiz.fPlural -side top

   pack .tRaceWiz.fPassword.lTxt -side left
   pack .tRaceWiz.fPassword.eVal -side right
   pack .tRaceWiz.fPassword -side top

   pack .tRaceWiz.lfPre -side top
   pack .tRaceWiz.lfPre.row1 -side top
   pack .tRaceWiz.lfPre.rHum -side left  -in .tRaceWiz.lfPre.row1
   pack .tRaceWiz.lfPre.rSil -side right -in .tRaceWiz.lfPre.row1
   pack .tRaceWiz.lfPre.row2 -side top
   pack .tRaceWiz.lfPre.rRab -side left  -in .tRaceWiz.lfPre.row2
   pack .tRaceWiz.lfPre.rAnt -side right -in .tRaceWiz.lfPre.row2
   pack .tRaceWiz.lfPre.row3 -side top
   pack .tRaceWiz.lfPre.rIns -side left  -in .tRaceWiz.lfPre.row3
   pack .tRaceWiz.lfPre.rRan -side right -in .tRaceWiz.lfPre.row3
   pack .tRaceWiz.lfPre.row4 -side top
   pack .tRaceWiz.lfPre.rNuc -side left  -in .tRaceWiz.lfPre.row4
   pack .tRaceWiz.lfPre.rCus -side right -in .tRaceWiz.lfPre.row4

   pack .tRaceWiz.lLeft -side top

   .tRaceWiz.fBRow.bBack configure -state disabled

   racePackButtons
}

proc racePage2 {} {
   bind .tRaceWiz <Alt-a> {}

   pack .tRaceWiz.fPoints.lTxt -side left
   pack .tRaceWiz.fPoints.lVal -side right
   pack .tRaceWiz.fPoints -side right -anchor ne

   pack .tRaceWiz.fPage2

   racePackButtons

   .tRaceWiz.fBRow.bBack configure -state normal
}

proc racePage3 {} {
   bind .tRaceWiz <Alt-a> {}

   pack .tRaceWiz.fPoints.lTxt -side left
   pack .tRaceWiz.fPoints.lVal -side right
   pack .tRaceWiz.fPoints -side right -anchor ne

   racePackButtons

   pack .tRaceWiz.fPage3bot -side bottom
   pack .tRaceWiz.fPage3top -side bottom
}

proc racePage4 {} {
   bind .tRaceWiz <Alt-a> {}

   pack .tRaceWiz.fPoints.lTxt -side left
   pack .tRaceWiz.fPoints.lVal -side right
   pack .tRaceWiz.fPoints -side top -anchor ne

   pack .tRaceWiz.fGrav  -side top -anchor n
   pack .tRaceWiz.fGrav2 -side top -anchor n -expand 1 -fill x

   pack .tRaceWiz.fTemp  -side top -anchor n
   pack .tRaceWiz.fTemp2 -side top -anchor n -expand 1 -fill x

   pack .tRaceWiz.fRad  -side top -anchor n
   pack .tRaceWiz.fRad2 -side top -anchor n -expand 1 -fill x

   pack .tRaceWiz.fGrow -side top

   racePackButtons
}

proc racePage5 {} {
   bind .tRaceWiz <Alt-a> {}

   pack .tRaceWiz.fPoints.lTxt -side left
   pack .tRaceWiz.fPoints.lVal -side right
   pack .tRaceWiz.fPoints -side right -anchor ne

   pack .tRaceWiz.fPage5 -side top

   racePackButtons

   .tRaceWiz.fBRow.bNext configure -state normal
}

proc racePage6 {} {
   bind .tRaceWiz <Alt-a> { .tRaceWiz.fPage6cb toggle }

   pack .tRaceWiz.fPoints.lTxt -side left
   pack .tRaceWiz.fPoints.lVal -side right
   pack .tRaceWiz.fPoints -side right -anchor ne

   pack .tRaceWiz.fPage6   -side top
   pack .tRaceWiz.fPage6cb -side top

   racePackButtons

   .tRaceWiz.fBRow.bNext configure -state disabled
}

proc raceHelp {} {
}

proc raceBack {} {
   if {$raceWiz::step == 1} { return }

   incr raceWiz::step -1

   eval "pack forget [pack slaves .tRaceWiz]"

   wm title .tRaceWiz "Custom Race Wizard - Step $raceWiz::step of 6"
   eval "racePage$raceWiz::step"
}

proc raceNext {} {
   if {$raceWiz::step == 6} {return}

   incr raceWiz::step

   eval "pack forget [pack slaves .tRaceWiz]"

   wm title .tRaceWiz "Custom Race Wizard - Step $raceWiz::step of 6"
   eval "racePage$raceWiz::step"
}

#   set doc [dom parse $data]
#   set node [$doc documentElement root]
#   set singnode [$node getElementsByTagName SINGULARNAME]
#   .tRaceWiz.fName.eVal get

proc raceDone {} {
   set filename [tk_getSaveFile]
   if {$filename eq ""} { return }

   set data [newStars $::ns_race $::ns_getXML]

   set xmllist [xml2list $data]
   set arg [lindex $xmllist 2]

   set raceName [.tRaceWiz.fName.eVal get]
   set arg [changeXMLbyTag $arg SINGULARNAME $raceName]

   set pluralName [.tRaceWiz.fPlural.eVal get]
   set arg [changeXMLbyTag $arg PLURALNAME $pluralName]
   
   set newFile [open $filename w]
   puts $newFile {<?xml version="1.0"?>}
   puts $newFile $data
   close $newFile

   destroy .tRaceWiz
}

proc findPRTfromVec {vec} {
   for {set i 0} {$i < 10} {incr i} {
      if {[lindex $vec $i] == 1} {
         return $i
      }
   }
   return -1
}

proc makePRTvec {num} {
   set vec ""
   for {set i 0} {$i < 10} {incr i} {
      if {$i == $num} {
         lappend vec 1
      } else {
         lappend vec 0
      }
   }
   return $vec
}

proc updateRaceVars {first} {
   set raceWiz::points [newStars $::ns_race $::ns_getPoints]

   set raceXML [xml2list [newStars $::ns_race $::ns_getXML]]
   set mainXML [lindex $raceXML 2]

   if {$first} {
      set tmpXML [lindex [findXMLbyTag "LRTVECTOR" $mainXML 2] 1]
      for {set i 0} {$i < 14} {incr i} {
         set raceWiz::lrtAry($i) [lindex $tmpXML $i]
      }

      set tmpXML [lindex [findXMLbyTag "TECHCOSTMULTIPLIER" $mainXML 2] 1]
      for {set i 0} {$i < 6} {incr i} {
         set raceWiz::techCst($i) [lindex $tmpXML $i]
      }
   }

   set tmpXML [findXMLbyTag "PRTVECTOR" $mainXML 2]
   set tuple  [lindex $tmpXML 1]
   set raceWiz::prmTrait [findPRTfromVec $tuple]

   set tmpXML [findXMLbyTag "POPULATIONRESOURCEUNIT" $mainXML 2]
   set raceWiz::resPop [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "FACTORYOUTPUT" $mainXML 2]
   set raceWiz::resFact [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "FACTORYRESOURCECOST" $mainXML 2]
   set raceWiz::costFact [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "FACTORYBUILDFACTOR" $mainXML 2]
   set raceWiz::numFact [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "FACTORYMINERALCOST" $mainXML 2]
   set tuple [lindex $tmpXML 1]
   if {[lindex $tuple 2] == 3} {
      set raceWiz::factOneLess 1
   } else {
      set raceWiz::factOneLess 0
   }

   set tmpXML [findXMLbyTag "MINEOUTPUT" $mainXML 2]
   set raceWiz::effMine [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "MINERESOURCECOST" $mainXML 2]
   set raceWiz::costMine [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "MINEBUILDFACTOR" $mainXML 2]
   set raceWiz::numMine [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "GROWTHRATE" $mainXML 2]
   set raceWiz::growthRt [lindex $tmpXML 1]

   set tmpXML [findXMLbyTag "HAB" $mainXML 2]
   set tuple  [lindex $tmpXML 1]

   set raceWiz::grvCtr [lindex $tuple 0]
   set raceWiz::tmpCtr [lindex $tuple 1]
   set raceWiz::radCtr [lindex $tuple 2]

   set tmpXML [findXMLbyTag "TOLERANCE" $mainXML 2]
   set tuple  [lindex $tmpXML 1]

   set raceWiz::grvRng [lindex $tuple 0]
   set raceWiz::tmpRng [lindex $tuple 1]
   set raceWiz::radRng [lindex $tuple 2]

   set tmpXML [findXMLbyTag "IMMUNITY" $mainXML 2]
   set tuple  [lindex $tmpXML 1]

   set raceWiz::grvImm [lindex $tuple 0]
   set raceWiz::tmpImm [lindex $tuple 1]
   set raceWiz::radImm [lindex $tuple 2]
}

proc updateRace {first} {
   updateRaceVars $first

   if {$first} {
      set raceXML [xml2list [newStars $::ns_race $::ns_getXML]]
      set mainXML [lindex $raceXML 2]

      set tmpXML [findXMLbyTag "SINGULARNAME" $mainXML 2]
      .tRaceWiz.fName.eVal   insert 0 [lindex $tmpXML 1]

      set tmpXML [findXMLbyTag "PLURALNAME" $mainXML 2]
      .tRaceWiz.fPlural.eVal insert 0 [lindex $tmpXML 1]
   }

   .tRaceWiz.fGrav.cGrav delete all
   if {$raceWiz::grvImm == 0} {
      set bot [expr $raceWiz::grvCtr - $raceWiz::grvRng]
      set top [expr $raceWiz::grvCtr + $raceWiz::grvRng]
      .tRaceWiz.fGrav.lRange configure -text "$nsconst::gravLabel($bot)g\n to\n $nsconst::gravLabel($top)g"
      drawHabBar .tRaceWiz.fGrav.cGrav $raceWiz::grvCtr $raceWiz::grvRng "#00007F"
      .tRaceWiz.fGrav2.bWiden configure -state normal
      .tRaceWiz.fGrav2.bNarrow configure -state normal
   } else {
      .tRaceWiz.fGrav.lRange configure -text "\nN/A\n"
      .tRaceWiz.fGrav2.bWiden configure -state disabled
      .tRaceWiz.fGrav2.bNarrow configure -state disabled
   }

   .tRaceWiz.fTemp.cTemp delete all
   if {$raceWiz::tmpImm == 0} {
      set bot [expr ($raceWiz::tmpCtr - $raceWiz::tmpRng - 50) * 4]
      set top [expr ($raceWiz::tmpCtr + $raceWiz::tmpRng - 50) * 4]
      .tRaceWiz.fTemp.lRange configure -text "$bot C\n to\n $top C"
      drawHabBar .tRaceWiz.fTemp.cTemp $raceWiz::tmpCtr $raceWiz::tmpRng "#7F0000"
      .tRaceWiz.fTemp2.bWiden configure -state normal
      .tRaceWiz.fTemp2.bNarrow configure -state normal
   } else {
      .tRaceWiz.fTemp.lRange configure -text "\nN/A\n"
      .tRaceWiz.fTemp2.bWiden configure -state disabled
      .tRaceWiz.fTemp2.bNarrow configure -state disabled
   }

   .tRaceWiz.fRad.cHab delete all
   if {$raceWiz::radImm == 0} {
      set bot [expr ($raceWiz::radCtr - $raceWiz::radRng)]
      set top [expr ($raceWiz::radCtr + $raceWiz::radRng)]
      drawHabBar .tRaceWiz.fRad.cHab $raceWiz::radCtr $raceWiz::radRng "#007F00"
      .tRaceWiz.fRad.lRange  configure -text "$bot mR\n to\n $top mR"
      .tRaceWiz.fRad2.bWiden  configure -state normal
      .tRaceWiz.fRad2.bNarrow configure -state normal
   } else {
      .tRaceWiz.fRad.lRange   configure -text "\nN/A\n"
      .tRaceWiz.fRad2.bWiden  configure -state disabled
      .tRaceWiz.fRad2.bNarrow configure -state disabled
   }
}

proc updateGrowth {} {
   set raceXML [newStars $::ns_race $::ns_getXML]
   set lxml    [lindex [xml2list $raceXML] 2]
   set oldGt   [lindex [findXMLbyTag "GROWTHRATE" $lxml 2] 1]

   regsub "<GROWTHRATE>$oldGt</GROWTHRATE>" $raceXML "<GROWTHRATE>$raceWiz::growthRt</GROWTHRATE>" raceXML

   newStars $::ns_race $::ns_setXML $raceXML

   set raceWiz::points [newStars $::ns_race $::ns_getPoints]
}

proc chgRace {} {
   set raceWiz::prerace 7
   updateRace 0
}

proc chgPrt {} {
   set raceWiz::prerace 7

   set raceXML [newStars $::ns_race $::ns_getXML]
   set lxml    [lindex [xml2list $raceXML] 2]
   set oldVec [lindex [findXMLbyTag "PRTVECTOR" $lxml 2] 1]
   set newVec [makePRTvec $raceWiz::prmTrait]

   regsub "<PRTVECTOR>$oldVec</PRTVECTOR>" $raceXML "<PRTVECTOR>$newVec</PRTVECTOR>" raceXML

   newStars $::ns_race $::ns_setXML $raceXML

   set raceWiz::points [newStars $::ns_race $::ns_getPoints]
}

proc updateLRT {} {
   set raceXML [newStars $::ns_race $::ns_getXML]
   set lxml    [xml2list $raceXML]
   set mainXML [lindex $lxml 2]
   set oldVec [lindex [findXMLbyTag "LRTVECTOR" $mainXML 2] 1]

   set newVec ""
   for {set i 0} {$i < 14} {incr i} {
      lappend newVec $raceWiz::lrtAry($i)
   }

   regsub "<LRTVECTOR>$oldVec</LRTVECTOR>" $raceXML "<LRTVECTOR>$newVec</LRTVECTOR>" raceXML

   newStars $::ns_race $::ns_setXML $raceXML

   set raceWiz::points [newStars $::ns_race $::ns_getPoints]
}

#Ned
proc updateXMLforVar {tag val} {
   set raceXML [newStars $::ns_race $::ns_getXML]
   set lxml    [xml2list $raceXML]
   set mainXML [lindex $lxml 2]
   set oldVal [lindex [findXMLbyTag $tag $mainXML 2] 1]

   regsub "<$tag>$oldVal</$tag>" $raceXML \
       "<$tag>$val</$tag>" raceXML

   newStars $::ns_race $::ns_setXML $raceXML

   set raceWiz::points [newStars $::ns_race $::ns_getPoints]
}

proc updateResPop {} {
   updateXMLforVar POPULATIONRESOURCEUNIT $raceWiz::resPop
}

proc updateResFact {} {
   updateXMLforVar FACTORYOUTPUT $raceWiz::resFact
}

proc updateCostFact {} {
   updateXMLforVar FACTORYRESOURCECOST $raceWiz::costFact
}

proc updateNumFact {} {
   updateXMLforVar FACTORYBUILDFACTOR $raceWiz::numFact
}

proc updateFactOneLess {} {
   set costVec {0 0 4 0 0}
   if {$raceWiz::factOneLess} {
      set costVec {0 0 3 0 0}
   }

   updateXMLforVar {FACTORYMINERALCOST TYPE="Ironium Boranium Germanium Fuel Colonists" SCALE="KiloTons"} \
       $costVec
}

proc updateEffMine {} {
   updateXMLforVar MINEOUTPUT $raceWiz::effMine
}

proc updateCostMine {} {
   updateXMLforVar MINERESOURCECOST $raceWiz::costMine
}

proc updateNumMine {} {
   updateXMLforVar MINEBUILDFACTOR $raceWiz::numMine
}

proc selectPreRace {} {
   if { $raceWiz::prerace == 7 } { return }

   .tRaceWiz.fName.eVal   delete 0 end
   .tRaceWiz.fPlural.eVal delete 0 end

   if { $raceWiz::prerace == 5 } {
      .tRaceWiz.fName.eVal   insert 0 "Random"
      .tRaceWiz.fPlural.eVal insert 0 "Randoms"

      return
   }

   newStars $::ns_race $::ns_loadPre $raceWiz::prerace

   updateRace 1
}

proc racebutton {nm txt cmd sd} {
   pack [button $nm -text $txt -command $cmd -repeatdelay 400 -repeatinterval 50] -side $sd
}

# name, password, pre-defined, left-over
proc buildPage1 {} {
   frame .tRaceWiz.fName
   label .tRaceWiz.fName.lTxt -text "Race Name:" -width 15
   entry .tRaceWiz.fName.eVal

   frame .tRaceWiz.fPlural
   label .tRaceWiz.fPlural.lTxt -text "Plural Race Name:"
   entry .tRaceWiz.fPlural.eVal

   frame .tRaceWiz.fPassword
   label .tRaceWiz.fPassword.lTxt -text "Password:" -width 15
   entry .tRaceWiz.fPassword.eVal

   labelframe  .tRaceWiz.lfPre -text "Predefined Races" -bd 1 -relief sunken
   frame .tRaceWiz.lfPre.row1
   radiobutton .tRaceWiz.lfPre.rHum -text "Humanoid"   -value 0 -variable raceWiz::prerace -width 20 -command selectPreRace
   radiobutton .tRaceWiz.lfPre.rSil -text "Silicanoid" -value 1 -variable raceWiz::prerace -width 20 -command selectPreRace
   frame .tRaceWiz.lfPre.row2
   radiobutton .tRaceWiz.lfPre.rRab -text "Rabbitoid"  -value 2 -variable raceWiz::prerace -width 20 -command selectPreRace
   radiobutton .tRaceWiz.lfPre.rAnt -text "Antetheral" -value 3 -variable raceWiz::prerace -width 20 -command selectPreRace
   frame .tRaceWiz.lfPre.row3
   radiobutton .tRaceWiz.lfPre.rIns -text "Insectoid"  -value 4 -variable raceWiz::prerace -width 20 -command selectPreRace
   radiobutton .tRaceWiz.lfPre.rRan -text "Random"     -value 5 -variable raceWiz::prerace -width 20 -command selectPreRace
   frame .tRaceWiz.lfPre.row4
   radiobutton .tRaceWiz.lfPre.rNuc -text "Nucleotid"  -value 6 -variable raceWiz::prerace -width 20 -command selectPreRace
   radiobutton .tRaceWiz.lfPre.rCus -text "Custom"     -value 7 -variable raceWiz::prerace -width 20 -command selectPreRace

   label .tRaceWiz.lLeft -text "Spend up to 50 leftover advantage points on:"
}

# PRT
proc buildPage2 {} {
   labelframe .tRaceWiz.fPage2 -text "Primary Racial Trait"
   pack [frame       .tRaceWiz.fPage2.row1] -side top -expand 1 -fill x
   pack [radiobutton .tRaceWiz.fPage2.row1.rbL -text "Hyper-Expansion" -variable \
       raceWiz::prmTrait -value 9 -command chgPrt -width 14 -anchor w] -side left
   pack [radiobutton .tRaceWiz.fPage2.row1.rbR -text "Space Demolition" -variable \
       raceWiz::prmTrait -value 4 -command chgPrt] -side left

   pack [frame       .tRaceWiz.fPage2.row2] -side top -expand 1 -fill x
   pack [radiobutton .tRaceWiz.fPage2.row2.rbL -text "Super-Stealth" -variable \
       raceWiz::prmTrait -value 8 -command chgPrt -width 14 -anchor w] -side left
   pack [radiobutton .tRaceWiz.fPage2.row2.rbR -text "Packet Physics" -variable \
       raceWiz::prmTrait -value 3 -command chgPrt] -side left

   pack [frame       .tRaceWiz.fPage2.row3] -side top -expand 1 -fill x
   pack [radiobutton .tRaceWiz.fPage2.row3.rbL -text "War Monger" -variable \
       raceWiz::prmTrait -value 7 -command chgPrt -width 14 -anchor w] -side left
   pack [radiobutton .tRaceWiz.fPage2.row3.rbR -text "Inter-stellar Traveler" -variable \
       raceWiz::prmTrait -value 2 -command chgPrt] -side left

   pack [frame       .tRaceWiz.fPage2.row4] -side top -expand 1 -fill x
   pack [radiobutton .tRaceWiz.fPage2.row4.rbL -text "Claim Adjuster" -variable \
       raceWiz::prmTrait -value 6 -command chgPrt -width 14 -anchor w] -side left
   pack [radiobutton .tRaceWiz.fPage2.row4.rbR -text "Alternate Reality" -variable \
       raceWiz::prmTrait -value 1 -command chgPrt] -side left

   pack [frame       .tRaceWiz.fPage2.row5] -side top -expand 1 -fill x
   pack [radiobutton .tRaceWiz.fPage2.row5.rbL -text "Inner-Strength" -variable \
       raceWiz::prmTrait -value 5 -command chgPrt -width 14 -anchor w] -side left
   pack [radiobutton .tRaceWiz.fPage2.row5.rbR -text "Jack of All Trades" -variable \
       raceWiz::prmTrait -value 0 -command chgPrt] -side left
}

# LRT
proc buildPage3 {} {
   frame .tRaceWiz.fPage3top
   pack [label .tRaceWiz.fPage3top.lH -text "Lesser Racial Traits"] -side top

   frame .tRaceWiz.fPage3bot
   pack [frame       .tRaceWiz.fPage3bot.row1] -side top -expand 1 -fill x
   pack [checkbutton .tRaceWiz.fPage3bot.row1.cbL -text "Improved Fuel Efficiency" \
       -variable raceWiz::lrtAry(0) -command updateLRT -width 21 -anchor w] -side left
   pack [checkbutton .tRaceWiz.fPage3bot.row1.cbR -text "No Ram Scoop Engines" \
       -variable raceWiz::lrtAry(7) -command updateLRT] -side left

   pack [frame       .tRaceWiz.fPage3bot.row2] -side top -expand 1 -fill x
   pack [checkbutton .tRaceWiz.fPage3bot.row2.cbL -text "Total Terraforming" \
       -variable raceWiz::lrtAry(1) -command updateLRT -width 21 -anchor w] -side left
   pack [checkbutton .tRaceWiz.fPage3bot.row2.cbR -text "Cheap Engines" \
       -variable raceWiz::lrtAry(8) -command updateLRT] -side left

   pack [frame       .tRaceWiz.fPage3bot.row3] -side top -expand 1 -fill x
   pack [checkbutton .tRaceWiz.fPage3bot.row3.cbL -text "Advanced Remote Mining" \
       -variable raceWiz::lrtAry(2) -command updateLRT -width 21 -anchor w] -side left
   pack [checkbutton .tRaceWiz.fPage3bot.row3.cbR -text "Only Basic Remote Mining" \
       -variable raceWiz::lrtAry(9) -command updateLRT] -side left

   pack [frame       .tRaceWiz.fPage3bot.row4] -side top -expand 1 -fill x
   pack [checkbutton .tRaceWiz.fPage3bot.row4.cbL -text "Improved Starbases" \
       -variable raceWiz::lrtAry(3) -command updateLRT -width 21 -anchor w] -side left
   pack [checkbutton .tRaceWiz.fPage3bot.row4.cbR -text "No Advanced Scanners" \
       -variable raceWiz::lrtAry(10) -command updateLRT] -side left

   pack [frame       .tRaceWiz.fPage3bot.row5] -side top -expand 1 -fill x
   pack [checkbutton .tRaceWiz.fPage3bot.row5.cbL -text "Generalized Research" \
       -variable raceWiz::lrtAry(4) -command updateLRT -width 21 -anchor w] -side left
   pack [checkbutton .tRaceWiz.fPage3bot.row5.cbR -text "Low Starting Population" \
       -variable raceWiz::lrtAry(11) -command updateLRT] -side left

   pack [frame       .tRaceWiz.fPage3bot.row6] -side top -expand 1 -fill x
   pack [checkbutton .tRaceWiz.fPage3bot.row6.cbL -text "Ultimate Recycling" \
       -variable raceWiz::lrtAry(5) -command updateLRT -width 21 -anchor w] -side left
   pack [checkbutton .tRaceWiz.fPage3bot.row6.cbR -text "Bleeding Edge Technology" \
       -variable raceWiz::lrtAry(12) -command updateLRT] -side left

   pack [frame       .tRaceWiz.fPage3bot.row7] -side top -expand 1 -fill x
   pack [checkbutton .tRaceWiz.fPage3bot.row7.cbL -text "Mineral Alchemy" \
       -variable raceWiz::lrtAry(6) -command updateLRT -width 21 -anchor w] -side left
   pack [checkbutton .tRaceWiz.fPage3bot.row7.cbR -text "Regenerating Shields" \
       -variable raceWiz::lrtAry(13) -command updateLRT] -side left
}

# hab ranges and growth rate
proc buildPage4 {} {
   frame .tRaceWiz.fGrav
   pack [label  .tRaceWiz.fGrav.lGrTxt -text "Gravity" -width 10] -side left
   racebutton .tRaceWiz.fGrav.bLeft "<" "newStars $::ns_race $::ns_habCenter 0 -1; chgRace" left
   pack [canvas .tRaceWiz.fGrav.cGrav  -width 300 -height 20 -bg black] -side left
   racebutton .tRaceWiz.fGrav.bRght ">" "newStars $::ns_race $::ns_habCenter 0 1; chgRace" left
   pack [label  .tRaceWiz.fGrav.lRange -text "0.22g to 4.40g"] -side right

   frame .tRaceWiz.fGrav2
   racebutton      .tRaceWiz.fGrav2.bWiden "<<  >>" "newStars $::ns_race $::ns_habWidth 0 1; chgRace" left
   pack [checkbutton .tRaceWiz.fGrav2.bImmune -text "Immune to Gravity" -underline 10 -variable raceWiz::grvImm \
         -command {newStars $::ns_race $::ns_habImmune 0 $raceWiz::grvImm; chgRace}] -side left
   racebutton      .tRaceWiz.fGrav2.bNarrow ">>  <<" "newStars $::ns_race $::ns_habWidth 0 -1; chgRace" right

   frame .tRaceWiz.fTemp
   pack [label  .tRaceWiz.fTemp.lTxt -text "Temperature"] -side left
   racebutton .tRaceWiz.fTemp.bLeft "<" "newStars $::ns_race $::ns_habCenter 1 -1; chgRace" left
   pack [canvas .tRaceWiz.fTemp.cTemp -width 300 -height 20 -bg black] -side left
   racebutton .tRaceWiz.fTemp.bRght ">" "newStars $::ns_race $::ns_habCenter 1  1; chgRace" left
   pack [label  .tRaceWiz.fTemp.lRange -text "-200 C to 200 C"] -side right

   frame .tRaceWiz.fTemp2
   racebutton      .tRaceWiz.fTemp2.bWiden "<<  >>" "newStars $::ns_race $::ns_habWidth 1 1; chgRace" left
   pack [checkbutton .tRaceWiz.fTemp2.bImmune -text "Immune to Temperature" -underline 10 -variable raceWiz::tmpImm \
         -command {newStars $::ns_race $::ns_habImmune 1 $raceWiz::tmpImm; chgRace}] -side left
   racebutton      .tRaceWiz.fTemp2.bNarrow ">>  <<" "newStars $::ns_race $::ns_habWidth 1 -1; chgRace" right

   frame .tRaceWiz.fRad
   pack [label  .tRaceWiz.fRad.lTxt -text "Radiation" -width 10] -side left
   racebutton .tRaceWiz.fRad.bLeft "<" "newStars $::ns_race $::ns_habCenter 2 -1; chgRace" left
   pack [canvas .tRaceWiz.fRad.cHab -width 300 -height 20 -bg black] -side left
   racebutton .tRaceWiz.fRad.bRght ">" "newStars $::ns_race $::ns_habCenter 2  1; chgRace" left
   pack [label  .tRaceWiz.fRad.lRange -text "0 mR to 100 mR"] -side right

   frame .tRaceWiz.fRad2
   racebutton      .tRaceWiz.fRad2.bWiden "<<  >>" "newStars $::ns_race $::ns_habWidth 2 1; chgRace" left
   pack [checkbutton .tRaceWiz.fRad2.bImmune -text "Immune to Radiation" -underline 10 -variable raceWiz::radImm \
         -command {newStars $::ns_race $::ns_habImmune 2 $raceWiz::radImm; chgRace}] -side left
   racebutton      .tRaceWiz.fRad2.bNarrow ">>  <<" "newStars $::ns_race $::ns_habWidth 2 -1; chgRace" right

   frame .tRaceWiz.fGrow
   pack [label   .tRaceWiz.fGrow.lGrow -text "Maximum colonist growth rate per year: "] -side left
   pack [spinbox .tRaceWiz.fGrow.sGrow -from 1 -to 20 -state readonly -width 3 \
       -textvariable raceWiz::growthRt -command updateGrowth -relief flat] -side left
   pack [label   .tRaceWiz.fGrow.lr -text "%"] -side left
}

proc buildPage5 {} {
   frame .tRaceWiz.fPage5

   pack [frame   .tRaceWiz.fPage5.row1] -side top
   pack [label   .tRaceWiz.fPage5.row1.l -text "One resource is generated each year for every "] -side left
   pack [spinbox .tRaceWiz.fPage5.row1.s -from 700 -to 2500 -increment 100 -state readonly -width 5 \
       -textvariable raceWiz::resPop -command updateResPop -relief flat] -side left
   pack [label   .tRaceWiz.fPage5.row1.l2 -text " colonists."] -side left

   pack [frame   .tRaceWiz.fPage5.row2] -side top
   pack [label   .tRaceWiz.fPage5.row2.ll -text "Every 10 factories produce "] -side left
   pack [spinbox .tRaceWiz.fPage5.row2.s -from 5 -to 15 -state readonly -width 3 \
       -textvariable raceWiz::resFact -command updateResFact -relief flat] -side left
   pack [label   .tRaceWiz.fPage5.row2.lr -text " resources each year."] -side left

   pack [frame   .tRaceWiz.fPage5.row3] -side top
   pack [label   .tRaceWiz.fPage5.row3.ll -text "Factories require "] -side left
   pack [spinbox .tRaceWiz.fPage5.row3.s -from 5 -to 25 -state readonly -width 3 \
       -textvariable raceWiz::costFact -command updateCostFact -relief flat] -side left
   pack [label   .tRaceWiz.fPage5.row3.lr -text " resources to build."] -side left

   pack [frame   .tRaceWiz.fPage5.row4] -side top
   pack [label   .tRaceWiz.fPage5.row4.ll -text "Every 10,000 colonists may operate up to "] -side left
   pack [spinbox .tRaceWiz.fPage5.row4.s -from 5 -to 25 -state readonly -width 3 \
       -textvariable raceWiz::numFact -command updateNumFact -relief flat] -side left
   pack [label   .tRaceWiz.fPage5.row4.lr -text " factories."] -side left

   pack [checkbutton .tRaceWiz.fPage5.cb -text "Factories cost 1kT less of Germanium to build" \
       -variable raceWiz::factOneLess -command updateFactOneLess] -side top

   pack [frame   .tRaceWiz.fPage5.row5] -side top
   pack [label   .tRaceWiz.fPage5.row5.ll -text "Every 10 mines produce up to "] -side left
   pack [spinbox .tRaceWiz.fPage5.row5.s -from 5 -to 25 -state readonly -width 3 \
       -textvariable raceWiz::effMine -command updateEffMine -relief flat] -side left
   pack [label   .tRaceWiz.fPage5.row5.lr -text "kT of mineral every year."] -side left

   pack [frame   .tRaceWiz.fPage5.row6] -side top
   pack [label   .tRaceWiz.fPage5.row6.ll -text "Mines require "] -side left
   pack [spinbox .tRaceWiz.fPage5.row6.s -from 2 -to 15 -state readonly -width 3 \
       -textvariable raceWiz::costMine -command updateCostMine -relief flat] -side left
   pack [label   .tRaceWiz.fPage5.row6.lr -text " resources to build."] -side left

   pack [frame   .tRaceWiz.fPage5.row7] -side top
   pack [label   .tRaceWiz.fPage5.row7.ll -text "Every 10,000 colonists may operate up to "] -side left
   pack [spinbox .tRaceWiz.fPage5.row7.s -from 5 -to 25 -state readonly -width 3 \
       -textvariable raceWiz::numMine -command updateNumMine -relief flat] -side left
   pack [label   .tRaceWiz.fPage5.row7.lr -text " mines."] -side left
}

proc updateTech {} {
   set techAry ""
   for {set i 0} {$i < 6} {incr i} {
      lappend techAry $raceWiz::techCst($i)
   }

   updateXMLforVar TECHCOSTMULTIPLIER $techAry
}

proc updateTech3 {} {
   updateXMLforVar FACTORYOUTPUT $raceWiz::techStart3
}

proc buildPage6 {} {
   frame       .tRaceWiz.fPage6
   checkbutton .tRaceWiz.fPage6cb -text "All 'Costs 75% extra' research fields start at Tech 3" \
       -variable raceWiz::techStart3 -underline 0 -command updateTech3

   pack [frame       .tRaceWiz.fPage6.row1] -side top
   pack [labelframe  .tRaceWiz.fPage6.row1.lfL -text "Energy"] -side left
   pack [radiobutton .tRaceWiz.fPage6.row1.lfL.rbEx -text "Costs 75% extra" -width 18 -anchor w \
       -variable raceWiz::techCst(0) -value 2 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row1.lfL.rbNorm -text "Costs standard amount" \
       -variable raceWiz::techCst(0) -value 1 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row1.lfL.rbCheap -text "Costs 50% less" -width 18 -anchor w \
       -variable raceWiz::techCst(0) -value 0 -command updateTech] -side top

   pack [labelframe  .tRaceWiz.fPage6.row1.lfR    -text "Construction"] -side right
   pack [radiobutton .tRaceWiz.fPage6.row1.lfR.rbEx -text "Costs 75% extra" -width 18 -anchor w \
       -variable raceWiz::techCst(3) -value 2 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row1.lfR.rbNorm -text "Costs standard amount" \
       -variable raceWiz::techCst(3) -value 1 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row1.lfR.rbCheap -text "Costs 50% less" -width 18 -anchor w \
       -variable raceWiz::techCst(3) -value 0 -command updateTech] -side top


   pack [frame       .tRaceWiz.fPage6.row2] -side top
   pack [labelframe  .tRaceWiz.fPage6.row2.lfL -text "Weapons"] -side left
   pack [radiobutton .tRaceWiz.fPage6.row2.lfL.rbEx -text "Costs 75% extra" -width 18 -anchor w \
       -variable raceWiz::techCst(1) -value 2 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row2.lfL.rbNorm -text "Costs standard amount" \
       -variable raceWiz::techCst(1) -value 1 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row2.lfL.rbCheap -text "Costs 50% less" -width 18 -anchor w \
       -variable raceWiz::techCst(1) -value 0 -command updateTech] -side top

   pack [labelframe  .tRaceWiz.fPage6.row2.lfR    -text "Electronics"] -side right
   pack [radiobutton .tRaceWiz.fPage6.row2.lfR.rbEx -text "Costs 75% extra" -width 18 -anchor w \
       -variable raceWiz::techCst(4) -value 2 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row2.lfR.rbNorm -text "Costs standard amount" \
       -variable raceWiz::techCst(4) -value 1 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row2.lfR.rbCheap -text "Costs 50% less" -width 18 -anchor w \
       -variable raceWiz::techCst(4) -value 0 -command updateTech] -side top


   pack [frame       .tRaceWiz.fPage6.row3] -side top
   pack [labelframe  .tRaceWiz.fPage6.row3.lfL -text "Propulsion"] -side left
   pack [radiobutton .tRaceWiz.fPage6.row3.lfL.rbEx -text "Costs 75% extra" -width 18 -anchor w \
       -variable raceWiz::techCst(2) -value 2 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row3.lfL.rbNorm -text "Costs standard amount" \
       -variable raceWiz::techCst(2) -value 1 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row3.lfL.rbCheap -text "Costs 50% less" -width 18 -anchor w \
       -variable raceWiz::techCst(2) -value 0 -command updateTech] -side top

   pack [labelframe  .tRaceWiz.fPage6.row3.lfR   -text "Biotechnology"] -side right
   pack [radiobutton .tRaceWiz.fPage6.row3.lfR.rbEx -text "Costs 75% extra" -width 18 -anchor w \
       -variable raceWiz::techCst(5) -value 2 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row3.lfR.rbNorm -text "Costs standard amount" \
       -variable raceWiz::techCst(5) -value 1 -command updateTech] -side top
   pack [radiobutton .tRaceWiz.fPage6.row3.lfR.rbCheap -text "Costs 50% less" -width 18 -anchor w \
       -variable raceWiz::techCst(5) -value 0 -command updateTech] -side top
}

# construct the race wizard gui elements
proc doRace {{ro 0}} {
   destroy  .tRaceWiz
   toplevel .tRaceWiz
   wm title .tRaceWiz "Race Wizard"

   frame    .tRaceWiz.secretFrame
   bind .tRaceWiz.secretFrame <Destroy> {newStars $::ns_race $::ns_loadPre -1}

   frame .tRaceWiz.fPoints -relief solid -bd 4
   label .tRaceWiz.fPoints.lTxt -text "Advantage\nPoints Left"
   label .tRaceWiz.fPoints.lVal -textvariable raceWiz::points

   frame  .tRaceWiz.fBRow
   button .tRaceWiz.fBRow.bHelp   -text "Help"   -command raceHelp -underline 0
   button .tRaceWiz.fBRow.bCancel -text "Cancel" -command "destroy .tRaceWiz"
   button .tRaceWiz.fBRow.bBack   -text "Back"   -command raceBack -underline 0
   button .tRaceWiz.fBRow.bNext   -text "Next"   -command raceNext -underline 0
   button .tRaceWiz.fBRow.bFinish -text "Finish" -command raceDone -underline 0

   bind .tRaceWiz <Alt-n> raceNext
   bind .tRaceWiz <Alt-b> raceBack
   bind .tRaceWiz <Alt-f> raceDone
   bind .tRaceWiz <Alt-h> raceHelp
   bind .tRaceWiz <Escape> "destroy .tRaceWiz"

   buildPage1
   buildPage2
   buildPage3
   buildPage4
   buildPage5
   buildPage6

   set raceWiz::step 1

   if {$ro == 1} {
      set raceWiz::prerace 7
      updateRace 1
   } else {
      selectPreRace
   }

   racePage1
   focus .tRaceWiz
}

