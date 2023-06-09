# Test TIP 192 - Lazy Lists

lappend ::auto_path .
puts auto_path=$auto_path
package require lgen

set x [lseq 17]
set y [lgen 17 expr * 6]
foreach i $x n $y {
    puts "$i -> $n"
}
puts Index+7:[lgen 15 expr + 7]:--

set phi [expr {(1 + sqrt(5.0)) / 2.0}]

proc fib {n} {
    global phi
    expr {round(pow($phi, $n) / sqrt(5.0))}
}

puts "First 20 fibbinacci:[lgen 20 fib]"
