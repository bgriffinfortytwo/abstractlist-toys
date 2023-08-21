
# memory validate on

set fl {lgen.tcl fib.tcl poly.tcl readlines.tcl lstring.tcl}
puts fl=$fl
set script [lmap f $fl {
    format "puts #\\ ====-%s-====\nsource %s\nputs #\\ ====-done-====\n" $f $f
}]
set x [join $script {}]
puts "# Evaluate:\n$x"

eval $x

puts "End of all.tcl\nSo Long!"
