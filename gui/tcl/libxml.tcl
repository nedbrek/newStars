proc xml2list xml {
   regsub -all {>\s*<} [string trim $xml " \n\t<>"] "\} \{" xml
   set xml [string map {> "\} \{#text \{" < "\}\} \{"}  $xml]

   set res ""   ;# string to collect the result
   set stack {} ;# track open tags
   set rest {}
   foreach item "{$xml}" {
      switch -regexp -- $item {
         ^# {append res "{[lrange $item 0 end]} " ; #text item}

         ^/ {
            regexp {/(.+)} $item -> tagname ;# end tag
            set expected [lindex $stack end]
            if {$tagname!=$expected} {error "$item != $expected"}
            set stack [lrange $stack 0 end-1]
            append res "\}\} "
         }

         /$ { # singleton - start and end in one <> group
            regexp {([^ ]+)( (.+))?/$} $item -> tagname - rest
            set rest [lrange [string map {= " "} $rest] 0 end]
            append res "{$tagname [list $rest] {}} "
         }

         default {
            set tagname [lindex $item 0] ;# start tag
            set rest [lrange [string map {= " "} $item] 1 end]
            lappend stack $tagname
            append res "\{$tagname [list $rest] \{"
         }
      }

      if {[llength $rest]%2} {error "att's not paired: $rest"}
   }

   if [llength $stack] {error "unresolved: $stack"}
   string map {"\} \}" "\}\}"} [lindex $res 0]
}

proc list2xml list {
   switch -- [llength $list] {
      2 {lindex $list 1}

      3 {
         foreach {tag attributes children} $list break
         set res <$tag
         foreach {name value} $attributes {
            append res " $name=\"$value\""
         }
         if [llength $children] {
            append res >
            foreach child $children {
               append res [list2xml $child]
            }
            append res </$tag>
         } else {append res />}
      }

      default {error "could not parse $list"}
   }
}

proc findXMLbyTag {tag xml idx} {
   set ll [llength $xml]
   for {set i 0} {$i < $ll} {incr i} {
      set xl2 [lindex $xml $i]
      if {[lindex $xl2 0] eq $tag} {
         return [lindex [lindex $xl2 $idx] 0]
      }
   }
   return ""
}

# recursive version
proc findXMLbyTagR {tag xml idx} {
   set cur [lindex $xml 0]
   if {[lindex $cur 0] eq $tag} {
      return [lindex $cur $idx]
   }
   return [findXMLbyTagR $tag [lrange $xml 1 [llength $xml]] $idx]
}

# internal recusive version
proc changeXMLbyTagR {pre xml tag newVal} {
   set cur [lindex $xml 0]
   set nxt [lrange $xml 1 [llength $xml]]
   if {[lindex $cur 0] eq $tag} {
      lappend pre [lreplace $cur 2 2 [list [list #text $newVal]]]
      return "$pre $nxt"
   }
   lappend pre $cur
   return [changeXMLbyTagR $pre $nxt $tag $newVal]
}

# change the xml value for tag to newVal
proc changeXMLbyTag {xml tag newVal} {
   return [changeXMLbyTagR "" $xml $tag $newVal]
}
