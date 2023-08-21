
lappend auto_path [file dir [info script]]
package require lstring

proc value-isa {var {expected ""}} {
    upvar $var v
    set t [lindex [tcl::unsupported::representation $v] 3]
    if {$expected ne "" && $expected ne $t} {
	set fail " expecting: $expected"
    } else {
	set fail ""
    }
    return "value in $var is a $t$fail"
}
proc value-rep {var {expected ""}} {
    upvar $var v
    set r [tcl::unsupported::representation $v]
    set t [lindex $r 3]
    if {$expected ne "" && $expected ne $t} {
	set fail " expecting: $expected"
    } else {
	set fail ""
    }
    return "value in $var is a $r$fail"
}

set x [catch lstring err]
if {!$x || $err ne {wrong # args: should be "lstring string"}} {
    puts "\"catch lstring err\" failed: x=$x, err=$err"
}

set str "My name is Inigo Montoya. You killed my father. Prepare to die!"
set str2 "Vizzini: HE DIDN'T FALL? INCONCEIVABLE. Inigo Montoya: You keep using that word. I do not think it means what you think it means."

if {0} {
    memory tag lstring
    puts \n[memory info]
    set lstring-time [time {
	set f [lstring $str]
	foreach l $f {
	    incr hash($l)
	}
    } 100]

    puts [memory info]
    memory tag list
    set list-time [time {
	set g [split $str {}]
	foreach l $g {
	    incr lash($l)
	}
    } 100]

    memory tag ""

    puts lstring:${lstring-time}
    puts list:${list-time}
    puts f-isa:[value-isa f]
    puts f=$f
    puts g-isa:[value-isa f]
    puts g=$g

}

puts "\n# lrange not supported by lstring"
set fs [lstring $str]
puts fs-isa:[value-isa fs lstring]
puts slice:[set s [lrange $fs 11 23]]->[join $s {}]
puts fs-isa:[value-isa fs lstring]
puts s-isa:[value-isa s list]

puts "\n# lreverse is supported by lstring"
set fr [lstring $str]
puts fr-isa:[value-isa fr]
set rfr [lreverse $fr]
puts rfr-isa:[value-isa rfr]
puts reverse:$rfr
puts fr-isa:[value-isa fr]

puts "\n# lsearch no-shimmer (Parse out the words from the list of characters.)"
set f [lstring $str]
puts [set il [lsearch -all $f { }]]
puts f:[value-isa f lstring]
lappend il [llength $f]
set words {}
set start 0
foreach i $il {
    set word [join [lrange $f $start $i-1] {}]
    lappend words $word
    set start [expr {$i+1}]
}
puts words=$words


# Test lset
puts "lreplace supported"
set str2 "Vizzini: HE DIDN'T FALL? INCONCEIVABLE. Inigo Montoya: You keep using that word. I do not think it means what you think it means."
set l [lstring $str2]
set expres {V i z z i n i : { } H E { } D I D N ' T { } F a i l ? { } I N C O N C E I V A B L E . { } I n i g o { } M o n t o y a : { } Y o u { } k e e p { } u s i n g { } t h a t { } w o r d . { } I { } d o { } n o t { } t h i n k { } i t { } m e a n s { } w h a t { } y o u { } t h i n k { } i t { } m e a n s .}
puts before-:[value-isa l lstring]
set m [lreplace $l 19 22 F a i l]
puts after-l:[value-isa l lstring]
puts after-m:[value-isa m lstring]

puts l\ [expr {([join $l {}] eq $str2) ? "is good" : "is incorrect"}]
puts m\ [expr {([join $m {}] eq [join $expres {}]) ? "is good" : "is incorrect"}]
if {$expres ne $m} {
    puts "lreplace error: expecting $expres\ngot $m"
}
#puts m=$m
puts "ledit m 9 8 S"
puts [ledit m 9 -1 S]
puts [list linsert {$m} 13 {*}[split "almost " {}]]
puts [linsert $m 13 {*}[split "almost " {}]]
puts [lpop m 17]
puts m=$m
puts after-pop:[value-isa m]
set n [string first not $str2]
set ixl [lseq $n count [string length "not "]]
puts [list lremove [lstring $str2] {*}$ixl]
puts :[set ln [lremove [lstring $str2] {*}$ixl]]
puts ln=$ln
puts [value-isa ln]

proc test2 {l} {
    puts "# Test lsearch + lset"
    puts before:[value-isa l]
    set dotxl [lsearch -all $l "."]
    puts after-1:[value-isa l]
    foreach x $dotxl {
	puts "lset l $x+1 \\n ;# loop"
	lset l $x+1 \n
    }
    puts l=$l
    puts after-2:[value-isa l]
}
proc test2r {l} {
    puts "# Test lsearch + lreplace"
    puts before:[value-isa l]
    set dotxl [lsearch -all $l "."]
    puts after-1:[value-isa l]
    foreach x $dotxl {
	puts "lreplace $l $x+1 $x+1 \\n ;# loop"
	set y [lreplace $l $x+1 $x+1 \n]
    }
    puts y=$y
    puts after-2:[value-isa l]
}
proc test3 {l dotxl} {
    puts "# Test lset"
    puts before3-0:[value-isa l]
    set x [lindex $dotxl 0]
    puts "lset l $x+1 \\n"
    lset l $x+1 \n
    puts after3-0:[value-rep l]
    set x [lindex $dotxl 1]
    puts "lset l $x+1 \\n"
    lset l $x+1 \n
    set x [lindex $dotxl 2]
    puts len=[llength $l]
    puts "lset l $x+1 \\n"
    lset l $x+1 \n
    puts l=[join $l {}]
    puts after3-2:[value-isa l]
    return
}
proc test4 {l} {
    puts "# test foreach lset"
    puts before3:[value-isa l]
    foreach ix [lseq [llength $l]] ch $l {
	if {$ch eq "."} {
	    puts "lset l $ix \\n"
	    lset l $ix \n
	}
    }
    puts l=$l
    puts after4:[value-isa l]
}

proc test5 {} {
    # This test includes a utf-8 character that's actually 4-bytes, so
    # the indexing is not going to work correctly.
    puts "start test5"
    set str "I am 😎"
    set l [lstring "Lets parse $str"]
    puts value-isa:[value-isa l]
    puts length-l:[llength $l]
    set word ""
    foreach c $l {
	append word $c
	if {$c == " "} {
	    puts $word
	    set word ""
	}
    }
    puts $word
    puts l=$l
    puts value-isa:[value-isa l]
    puts "end test5"
}

test2  [lstring $str2]
test2r [lstring $str2]
test3  [lstring $str2] [lsearch -all [lstring $str2] "."]
#puts done!
#exit
test4  [lstring $str2]
if {[catch {test5} rv]} {
    puts "\ntest5 expected error: $rv"
}

puts "# Test join:"
puts joined-l:[join $l {}]
