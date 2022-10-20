load ./fib.so

catch lfib err
puts err=$err
set f [lfib 20 25]
puts f-isa:[tcl::unsupported::representation $f]
puts f=$f
puts slice:[lrange $f 3 5]
puts f-isa:[tcl::unsupported::representation $f]
puts reverse:[lreverse $f]
puts f-isa:[tcl::unsupported::representation $f]
