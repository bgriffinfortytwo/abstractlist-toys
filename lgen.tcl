# Test TIP 192 - Lazy Lists

lappend ::auto_path .
package require lgen

set x [lseq 17]
set y [lgen 17 expr 6*]

foreach i $x n $y {
    puts "$i -> $n"
}

# This line exposed a hole in TEBC, now fixed:
puts Index+7:[lgen 15 expr 7+ ]:--

proc fib {n} {
    set phi [expr {(1 + sqrt(5.0)) / 2.0}]
    set d [expr {round(pow($phi, $n) / sqrt(5.0))}]
    return $d
}
puts fib:[lmap n [lseq 5] {fib $n}]

puts "First 20 fibbinacci:[lgen 20 fib]"
