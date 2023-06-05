#
# AbstractList example concrete types
#
if {![package vsatisfies [package provide Tcl] 8.0-]} {return}

package ifneeded lfib 1.0.0 [list load [file join $dir fib.so]]
package ifneeded lpoly 1.0.0 [list load [file join $dir poly.so]]
package ifneeded lreadlines 1.0.0 [list load [file join $dir readlines.so]]
package ifneeded lstring 1.0.0 [list load [file join $dir lstring.so]]
