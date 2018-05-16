package require tdom

set filedata [read [open "newStarsComponents.xml"]]
set doc [dom parse $filedata]
set root [$doc documentElement root]
set nodes [$root getElementsByTagName "COMPONENT"]

set curIdx 0
trace add variable curIdx write updateCurIdx

proc drawTech {} {
   set node [lindex $::nodes $::curIdx]

   set techName [[[$node getElementsByTagName "NAME"] childNode] nodeValue]

   .fInfo.lTitle configure -text $techName

   set minerals [[[$node getElementsByTagName "MINERALCOSTBASE"] \
childNode] nodeValue]

   .fInfo.lIron  configure -text "Ironium   [lindex $minerals 0] kT"
   .fInfo.lBoron configure -text "Boronium  [lindex $minerals 1] kT"
   .fInfo.lGerm  configure -text "Germanium [lindex $minerals 2] kT"

   set resources [[[$node getElementsByTagName "RESOURCECOSTBASE"] childNode] nodeValue]
   .fInfo.lRes   configure -text "Resources $resources"

   set mass [[[$node getElementsByTagName "MASS"] childNode] nodeValue]
   .fInfo.lMass  configure -text "Mass: $mass kT"
}

proc updateCurIdx {n1 n2 o} {
   drawTech
}

wm title . "Technology Browser"

pack [frame  .fNav] -side top
pack [button .fNav.bPrev -text "<- Prev" -command {incr curIdx -1}] -side left
pack [button .fNav.bNext -text "Next ->" -command {incr curIdx}] -side left

pack [frame .fInfo] -side top
pack [label .fInfo.lTitle -text "Tech"] -side top
pack [label .fInfo.lIron  -text "Ironium   0 kT"] -side top
pack [label .fInfo.lBoron -text "Boronium  0 kT"] -side top
pack [label .fInfo.lGerm  -text "Germanium 0 kT"] -side top
pack [label .fInfo.lRes   -text "Resources 0"] -side top
pack [label .fInfo.lMass  -text "Mass: 0 kT"] -side top

pack [frame  .fBot] -side bottom -expand 1 -fill x
pack [button .fBot.bClose -text Close -command exit] -side right

drawTech
