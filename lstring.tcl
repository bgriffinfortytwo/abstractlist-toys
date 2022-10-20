lappend auto_path [file dir [info script]]
package require lstring

if {0} {
catch lstring err
puts \nusage=$err
set str "My name is Inigo Montoya. You killed my father. Prepare to die!"
set str2 "Vizzini: HE DIDN'T FALL? INCONCEIVABLE. Inigo Montoya: You keep using that word. I do not think it means what you think it means."

memory tag lstring
puts \n[memory info]
set lstring-time [time {
    set f [lstring $str]
    foreach l $f {
	incr hash($l)
    }
} 100]]

puts [memory info]
memory tag list
set list-time [time {
    set g [split $str {}]
    foreach l $g {
	incr lash($l)
    }
} 100]]

memory tag ""

puts lstring:${lstring-time}
puts list:${list-time}
puts f-isa:[tcl::unsupported::representation $f]
puts f=$f
puts g-isa:[tcl::unsupported::representation $f]
puts g=$g

puts "\n# lrange not supported by lstring"
set fs [lstring $str]
puts slice:[set s [lrange $fs 11 23]]->[join $s {}]
puts fs-isa:[tcl::unsupported::representation $fs]

#puts "\n# lreverse not supported by lstring"
set fr [lstring $str]
puts reverse:[lreverse $fr]
puts fr-isa:[tcl::unsupported::representation $fr]

puts "\n# Parse out the words from the list of characters."
puts [set il [lsearch -all $f { }]]
lappend il [llength $f]
set words {}
set start 0
foreach i $il {
    set word [join [lrange $f $start $i-1] {}]
    lappend words $word
    set start [expr {$i+1}]
}
puts words=$words
}

# Test lset
set str2 "Vizzini: HE DIDN'T FALL? INCONCEIVABLE. Inigo Montoya: You keep using that word. I do not think it means what you think it means."
set l [lstring $str2]

proc test2 {l} {
    puts "# Test lsearch + lset"
    puts before:[tcl::unsupported::representation $l]
    set dotxl [lsearch -all $l "."]
    foreach x $dotxl {
	puts "lset l $x+1 \\n ;# loop"
	lset l $x+1 \n
    }
    puts l=$l
    puts after:[tcl::unsupported::representation $l]
}
proc test3 {l dotxl} {
    puts "# Test lset"
    puts before:[tcl::unsupported::representation $l]
    set x [lindex $dotxl 0]
    puts "lset l $x+1 \\n"
    lset l $x+1 \n
    puts after0:[tcl::unsupported::representation $l]
    set x [lindex $dotxl 1]
    puts "lset l $x+1 \\n"
    lset l $x+1 \n
    set x [lindex $dotxl 2]
    puts "lset l $x+1 \\n"
    lset l $x+1 \n
    puts l=$l
    puts after:[tcl::unsupported::representation $l]
}
proc test4 {l} {
    puts "# test foreach lset"
    puts before:[tcl::unsupported::representation $l]
    foreach ix [lseq [llength $l]] ch $l {
	if {$ch eq "."} {
	    puts "lset l $ix \\n"
	    lset l $ix \n
	}
    }
    puts l=$l
    puts after:[tcl::unsupported::representation $l]
}

test2 [lstring $str2]
test3 [lstring $str2] [lsearch -all [lstring $str2] "."]
test4 [lstring $str2]

puts "# Test join:"
puts joined-l:[join $l {}]
