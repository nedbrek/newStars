proc commify { num {sep ,} } {
   while {[regsub {^([-+]?\d+)(\d\d\d)} $num "\\1$sep\\2" num]} {}
   return $num
}

# return the list common to l1 and l2
proc ljoin {l1 l2} {
   set ret ""
   foreach i $l1 {
      if {[lsearch -exact $l2 $i] != -1} {
         lappend ret $i
      }
   }
   return $ret
}

# return a list containing all of l1 and l2
proc lunion {l1 l2} {
	set ret $l2
   foreach i $l1 {
      if {[lsearch -exact $ret $i] == -1} {
         lappend ret $i
      }
   }
   return $ret
}

