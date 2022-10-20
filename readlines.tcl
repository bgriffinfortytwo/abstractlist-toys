load ./readlines.so

proc test {f} {
    foreach line [lreadlines $f] {
	puts line=$line
    }
}

test readlines.c
