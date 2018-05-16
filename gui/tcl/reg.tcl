source libxml.tcl

proc dumpFleets {} {
   global numFleets fleetMap
   for {set i 0} {$i < $numFleets} {incr i} {
      puts $fleetMap($i)
      puts    [newStars $::ns_planet $fleetMap($i) $::ns_getName]
      puts    [newStars $::ns_planet $fleetMap($i) $::ns_getNumFleets]

      set tmp [newStars $::ns_planet $fleetMap($i) $::ns_getXML]
      set xml  [xml2list $tmp]
      set xml2 [lindex  $xml 2]
      set tmp [llength $xml2]
      for {set j 0} {$j < $tmp} {incr j} {
         puts [lindex  $xml2 $j]
      }

      set tmp [newStars $::ns_planet $fleetMap($i) $::ns_getNumOrders]
      puts $tmp
      for {set j 0} {$j < $tmp} {incr j} {
         puts [newStars $::ns_planet $fleetMap($i) $::ns_getOrderNameFromIndex $j]
      }
   }
}

load ./nstclgui.so
puts "----File 1" 
puts [newStars $::ns_open ../../src/load_unload_test_0]
puts [newStars $::ns_planet 1 $::ns_getName]
puts $numFleets
dumpFleets

puts "----File 2" 
puts [newStars $::ns_open ../../src/load_unload_test_1]
puts $numFleets
dumpFleets

puts "----File 3" 
puts [newStars $::ns_open ../../src/buildTest0.xml]
for {set i 0} {$i < $numPlanets} {incr i} {
   puts [newStars $::ns_planet $planetMap($i) $::ns_getName]

   set tmp [newStars $::ns_planet $planetMap($i) $::ns_getXML]
   set xml [xml2list $tmp]
   set xl2 [lindex $xml 2]
   set tmp [llength $xl2]
   for {set j 0} {$j < $tmp} {incr j} {
      puts [lindex $xl2 $j]
   }

   set tmp [newStars $::ns_planet $planetMap($i) $::ns_getNumOrders]
   puts $tmp
   for {set j 0} {$j < $tmp} {incr j} {
      puts [newStars $::ns_planet $planetMap($i) $::ns_getOrderNameFromIndex $j]
   }
   puts "====="
}

exit
