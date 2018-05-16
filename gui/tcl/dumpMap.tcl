package require tdom
set doc [dom parse -channel [open tiny_sparse.xml]]
set root [$doc documentElement root]

set univP [$root selectNodes //UNIVERSE_PLANET_LIST/PLANET]
set playP [$root selectNodes //PLANET_LIST/PLANET]

foreach p $univP {
   set id [$p selectNodes string(OBJECTID/text())]
   set univMap($id) $p
}

foreach p $playP {
   set id [$p selectNodes string(OBJECTID/text())]
   set up $univMap($id)

   set name [$p selectNodes string(NAME/text())]
   set id   [$p selectNodes string(OBJECTID/text())]
   set x    [$p selectNodes string(X_COORD/text())]
   set y    [$p selectNodes string(Y_COORD/text())]

   set uname [$up selectNodes string(NAME/text())]
   set uid   [$up selectNodes string(OBJECTID/text())]
   set ux    [$up selectNodes string(X_COORD/text())]
   set uy    [$up selectNodes string(Y_COORD/text())]

   if {$name ne $uname} {puts "Name mismatch at id=$id"}
   if {$id   ne $uid  } {puts "Id   mismatch at id=$id"}
   if {$x    ne $ux   } {puts "X    mismatch at id=$id"}
   if {$y    ne $uy   } {puts "Y    mismatch at id=$id"}
}

puts "#\tX\tY\tName"
foreach p $univP {
   set name [$p selectNodes string(NAME/text())]
   set id   [$p selectNodes string(OBJECTID/text())]
   set x    [$p selectNodes string(X_COORD/text())]
   set y    [$p selectNodes string(Y_COORD/text())]

   incr x 1000
   incr y 1000
   puts "$id\t$x\t$y\t$name"
}
