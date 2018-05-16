package require tdom

set filedata [read [open "newStarsComponents.xml"]]
set doc [dom parse $filedata]
set root [$doc documentElement root]
set nodes [$root getElementsByTagName "COMPONENT"]

set curIdx 0
trace add variable curIdx write updateCurIdx

proc save {} {
	set f [open "ned.xml" "w"]
	puts $f {<?xml version="1.0" encoding="UTF-8" standalone="yes"?>}
	puts $f [$::doc asXML]
}

proc upIBG {off val} {
   set node [lindex $::nodes $::curIdx]
	set el [[$node getElementsByTagName "MINERALCOSTBASE"] childNode]
   set minerals [$el nodeValue]

	set minerals [lreplace $minerals $off $off $val]

	$el nodeValue $minerals
	drawTech
}

proc upR {val} {
   set node [lindex $::nodes $::curIdx]
	set el [[$node getElementsByTagName "RESOURCECOSTBASE"] childNode]
   $el nodeValue $val
	drawTech
}

proc upM {val} {
   set node [lindex $::nodes $::curIdx]
   set el [[$node getElementsByTagName "MASS"] childNode]
   $el nodeValue $val
	drawTech
}

proc drawTech {} {
   set node [lindex $::nodes $::curIdx]

   set techName [[[$node getElementsByTagName "NAME"] childNode] nodeValue]

   .fInfo.lTitle configure -text $techName

   set minerals [[[$node getElementsByTagName "MINERALCOSTBASE"] \
childNode] nodeValue]

	set i [lindex $minerals 0]
   .fInfo.lIron  configure -text "Ironium   $i kT"
	.fInfo.fI.e delete 0 end
	.fInfo.fI.e insert 0 $i

	set b [lindex $minerals 1]
   .fInfo.lBoron configure -text "Boronium  $b kT"
	.fInfo.fB.e delete 0 end
	.fInfo.fB.e insert 0 $b

	set g [lindex $minerals 2]
   .fInfo.lGerm  configure -text "Germanium $g kT"
	.fInfo.fG.e delete 0 end
	.fInfo.fG.e insert 0 $g

   set resources [[[$node getElementsByTagName "RESOURCECOSTBASE"] childNode] nodeValue]
   .fInfo.lRes   configure -text "Resources $resources"
	.fInfo.fR.e delete 0 end
	.fInfo.fR.e insert 0 $resources

   set mass [[[$node getElementsByTagName "MASS"] childNode] nodeValue]
   .fInfo.lMass  configure -text "Mass: $mass kT"
	.fInfo.fM.e delete 0 end
	.fInfo.fM.e insert 0 $mass
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

pack [frame .fInfo.fI] -side top
pack [label .fInfo.lIron  -text "Ironium   0 kT"] -side left -in .fInfo.fI
pack [entry .fInfo.fI.e] -side right
bind .fInfo.fI.e  <Return> {upIBG 0 [.fInfo.fI.e get]}

pack [frame .fInfo.fB] -side top
pack [label .fInfo.lBoron -text "Boronium  0 kT"] -side left -in .fInfo.fB
pack [entry .fInfo.fB.e] -side right
bind .fInfo.fB.e  <Return> {upIBG 1 [.fInfo.fB.e get]}

pack [frame .fInfo.fG] -side top
pack [label .fInfo.lGerm  -text "Germanium 0 kT"] -side left -in .fInfo.fG
pack [entry .fInfo.fG.e] -side right
bind .fInfo.fG.e  <Return> {upIBG 2 [.fInfo.fG.e get]}

pack [frame .fInfo.fR] -side top
pack [label .fInfo.lRes   -text "Resources 0"] -side left -in .fInfo.fR
pack [entry .fInfo.fR.e] -side right
bind .fInfo.fR.e  <Return> {upR [.fInfo.fR.e get]}

pack [frame .fInfo.fM] -side top
pack [label .fInfo.lMass  -text "Mass: 0 kT"] -side left -in .fInfo.fM
pack [entry .fInfo.fM.e] -side right
bind .fInfo.fM.e  <Return> {upM [.fInfo.fM.e get]}

pack [frame  .fBot] -side bottom -expand 1 -fill x
pack [button .fBot.bClose -text Close -command exit] -side right
pack [button .fBot.bSave -text Save -command save] -side right

drawTech

