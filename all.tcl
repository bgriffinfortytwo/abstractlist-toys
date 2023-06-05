
set fl [glob *.tcl]

set fl [lsearch -all -inline -not -regexp $fl (pkgIndex.tcl|all.tcl)]

puts fl=$fl
foreach f $fl {
    puts ====-$f-====
    source $f
    puts ====-done-====
}
