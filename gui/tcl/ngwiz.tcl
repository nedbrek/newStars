namespace eval newGameWiz {
   set step 1
   set ok   0

   set gameName   "Tasty Cheesefest"
   set univSzEnum 0
   set univDnEnum 0
   set playPos    0
   set maxmin     0
   set slowtech   0
   set accbbs     0
   set norand     0
   set cpall      0
   set pubscore   0
   set galclump   0
}

proc newGamePackButtons {} {
   pack .tNewGameWiz.fBRow -side bottom
}

proc newGamePage1 {} {
   wm title .tNewGameWiz "Advanced New Game Wizard - Step 1 of 2"

   pack .tNewGameWiz.fGameName -side top
   newGamePackButtons
   pack .tNewGameWiz.fLeft     -side left
   pack .tNewGameWiz.fRight    -side left

}

proc newGamePage2 {} {
   wm title .tNewGameWiz "Advanced New Game Wizard - Step 2 of 2"

   for {set i 1} {$i <= 16} {incr i} {
      pack .tNewGameWiz.fPg2Row$i
   }
   
   newGamePackButtons
}

proc newGameBack {} {
   if {$newGameWiz::step == 1} { return }

   incr newGameWiz::step -1

   eval "pack forget [pack slaves .tNewGameWiz]"

   eval "newGamePage$newGameWiz::step"
}

proc newGameNext {} {
   if {$newGameWiz::step == 2} {return}

   incr newGameWiz::step

   eval "pack forget [pack slaves .tNewGameWiz]"

   eval "newGamePage$newGameWiz::step"
}

# advanced game, parms
proc newGameBuild1 {} {
   frame .tNewGameWiz.fGameName
   pack [label .tNewGameWiz.fGameName.lName -text "Game Name:"] -side left
   pack [entry .tNewGameWiz.fGameName.eName \
    -textvariable newGameWiz::gameName] -side left

   ### left side (and size)
   frame .tNewGameWiz.fLeft
   pack [labelframe  .tNewGameWiz.fLeft.fUS -text "Universe Size"] -side top
   pack [radiobutton .tNewGameWiz.fLeft.fUS.rTiny  -text "Tiny"   -value 0 \
      -variable newGameWiz::univSzEnum] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fLeft.fUS.rSmall -text "Small"  -value 1 \
      -variable newGameWiz::univSzEnum] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fLeft.fUS.rMed   -text "Medium" -value 2 \
      -variable newGameWiz::univSzEnum] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fLeft.fUS.rLarge -text "Large"  -value 3 \
      -variable newGameWiz::univSzEnum] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fLeft.fUS.rHuge  -text "Huge"   -value 4 \
      -variable newGameWiz::univSzEnum] -side top -anchor w

   ### left side, density
   pack [labelframe  .tNewGameWiz.fLeft.fDen -text "Density"] -side top
   pack [radiobutton .tNewGameWiz.fLeft.fDen.rSparse -text "Sparse" -value 0 \
      -variable newGameWiz::univDnEnum] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fLeft.fDen.rNormal -text "Normal" -value 1 \
      -variable newGameWiz::univDnEnum] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fLeft.fDen.rDense -text "Dense" -value 2 \
      -variable newGameWiz::univDnEnum] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fLeft.fDen.rPacked -text "Packed" -value 3 \
      -variable newGameWiz::univDnEnum] -side top -anchor w

   ### right frame
   frame .tNewGameWiz.fRight
   pack [labelframe  .tNewGameWiz.fRight.fPos -text "Player Positions"] -side top
   pack [radiobutton .tNewGameWiz.fRight.fPos.rClose -text "Close" -value 0 \
      -variable newGameWiz::playPos] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fRight.fPos.rMod -text "Moderate" -value 1 \
      -variable newGameWiz::playPos] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fRight.fPos.rFar -text "Farther" -value 2 \
      -variable newGameWiz::playPos] -side top -anchor w
   pack [radiobutton .tNewGameWiz.fRight.fPos.rDist -text "Distant" -value 3 \
      -variable newGameWiz::playPos] -side top -anchor w

   pack [checkbutton .tNewGameWiz.fRight.cMaxMin -text "Beginner: Maximum Minerals" \
      -variable newGameWiz::maxmin] -side top -anchor w
   pack [checkbutton .tNewGameWiz.fRight.cSlowTech -text "Slower Tech Advances" \
      -variable newGameWiz::slowtech] -side top -anchor w
   pack [checkbutton .tNewGameWiz.fRight.cAccBBS -text "Accelerated BBS Play" \
      -variable newGameWiz::accbbs] -side top -anchor w
   pack [checkbutton .tNewGameWiz.fRight.cNoRand -text "No Random Events" \
      -variable newGameWiz::norand] -side top -anchor w
   pack [checkbutton .tNewGameWiz.fRight.cCPall -text "Computer Players Form Alliances" \
      -variable newGameWiz::cpall] -side top -anchor w
   pack [checkbutton .tNewGameWiz.fRight.cPPS -text "Public Player Scores" \
      -variable newGameWiz::pubscore] -side top -anchor w
   pack [checkbutton .tNewGameWiz.fRight.cGC -text "Galaxy Clumping" \
      -variable newGameWiz::galclump] -side top -anchor w
}

proc newGameClearRace {i} {
   .tNewGameWiz.fPg2Row$i.lFName configure -text "<None>"
}

proc newGameBrowseRace {i} {
   set ofile [tk_getOpenFile -initialdir $nsGui::ofileDir]
   if {$ofile eq ""} { return }
   set nsGui::ofileDir [file dirname $ofile]

   .tNewGameWiz.fPg2Row$i.lFName configure -text $ofile
}

# advanced game race list
proc newGameBuild2 {} {
   for {set i 1} {$i <= 16} {incr i} {
      set f [frame .tNewGameWiz.fPg2Row$i]
      pack [label  $f.lId -text "#$i"] -side left
      pack [button $f.bClear  -text "Clear"  -command "newGameClearRace  $i"] -side left
      pack [button $f.bBrowse -text "Browse" -command "newGameBrowseRace $i"] -side left
      pack [label  $f.lFName  -text "<None>"] -side left
   }
}

proc newGameHelp {} {
}

proc newGameDone {} {
   set filename [tk_getSaveFile]
   if {$filename eq ""} { return }

   set gameBaseName [file rootname [file tail $filename]]

   set sampleFile [open [file join $starkit::topdir sampleNewGame.xml]]
   set data [read $sampleFile]
   close $sampleFile

   if {[lindex $data 0] eq {<?xml}} {
      set data [lrange $data 2 [llength $data]]
   }
   set xmllist [xml2list $data]

   set arg [lindex $xmllist 2]

   set boolVal(0) false
   set boolVal(1) true

   set denVal(0) "SPARSE"
   set denVal(1) "NORMAL"
   set denVal(2) "DENSE"
   set denVal(3) "PACKED"

   set posVal(0) "CLOSE"
   set posVal(1) "MODERATE"
   set posVal(2) "FARTHER"
   set posVal(3) "DISTANT"

   set sizVal(0) "TINY"
   set sizVal(1) "SMALL"
   set sizVal(2) "MEDIUM"
   set sizVal(3) "LARGE"
   set sizVal(4) "HUGE"

   set arg [changeXMLbyTag $arg GAME_NAME        $newGameWiz::gameName]
   set arg [changeXMLbyTag $arg BASENAME         $gameBaseName]
   set arg [changeXMLbyTag $arg DENSITY          $denVal($newGameWiz::univDnEnum)]
   set arg [changeXMLbyTag $arg PLAYER_POSITIONS $posVal($newGameWiz::playPos)]
   set arg [changeXMLbyTag $arg SIZE             $sizVal($newGameWiz::univSzEnum)]
   set arg [changeXMLbyTag $arg MAX_MINERALS     $boolVal($newGameWiz::maxmin)]
   set arg [changeXMLbyTag $arg SLOW_TECH        $boolVal($newGameWiz::slowtech)]
   set arg [changeXMLbyTag $arg ACCEL_START      $boolVal($newGameWiz::accbbs)]
   set arg [changeXMLbyTag $arg RANDOM_EVENTS    $boolVal($newGameWiz::norand)]
   set arg [changeXMLbyTag $arg CP_ALLIANCE      $boolVal($newGameWiz::cpall)]
   set arg [changeXMLbyTag $arg PUB_SCORES       $boolVal($newGameWiz::pubscore)]
   set arg [changeXMLbyTag $arg STAR_CLUMPS      $boolVal($newGameWiz::galclump)]

   set xmllist [list NSTARS_NEW {} $arg]

   set data [list2xml $xmllist]

   set doc [dom parse $data]
   set node [$doc documentElement root]
   set rlnode [$node getElementsByTagName RACELIST]

   for {set i 1} {$i <= 16} {incr i} {
      set rfname [.tNewGameWiz.fPg2Row$i.lFName cget -text]
      if {$rfname ne "<None>"} {
         set racefile [open $rfname]
         set racedata [read $racefile]
         $rlnode appendXML $racedata
      }
   }

   set data [$doc asXML]
   $doc delete

   set newFile [open $filename w]
   puts $newFile {<?xml version="1.0"?>}
   puts $newFile $data
   close $newFile
   newStars $::ns_generate $filename 1

   destroy  .tNewGameWiz
}

proc newGame {} {
   destroy  .tNewGameWiz
   toplevel .tNewGameWiz

   newGameBuild1
   newGameBuild2

   frame .tNewGameWiz.fBRow
   pack [button .tNewGameWiz.fBRow.bHelp   -text "Help"   -command newGameHelp -underline 0] -side left
   pack [button .tNewGameWiz.fBRow.bCancel -text "Cancel" -command "destroy .tNewGameWiz"] -side left
   pack [button .tNewGameWiz.fBRow.bBack   -text "Back"   -command newGameBack -underline 0] -side left
   pack [button .tNewGameWiz.fBRow.bNext   -text "Next"   -command newGameNext -underline 0] -side left
   pack [button .tNewGameWiz.fBRow.bFinish -text "Finish" -command newGameDone -underline 0] -side left

   set newGameWiz::step 1
   newGamePage1

   focus .tNewGameWiz
}

