load ./readlines.so

proc test {f} {
    set x [lreadlines $f]
    set len [llength $x]
    set n [string length $len]
    set lineno 0
    foreach line $x {
	incr lineno
	puts [format "%*d: %s" $n $lineno $line]
    }
    puts llength=[llength $x]
}

test readlines.c
