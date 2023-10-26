lappend auto_path [pwd]
package require lweave

proc Task {} {
    set n 100000
    set t 50

    # L1 has now uniquid indicines
    set timeL1 [time {set L1 [Get_List1 $n ]}]
    puts timeL1->$timeL1
    set timeL2 [time {set L2 [Get_List2 $n ]}]
    puts timeL2->$timeL2
    set timeL3 [time {set L3 [Get_List3 $n ]}]
    puts timeL2->$timeL3

    set time8 [time {set Z8 [ZIP_Main8 $L1 $L2]} $t]
    puts time8->$time8
    puts Z8-representation->[tcl::unsupported::representation $Z8]
    puts dict-keys->[time {dict keys $Z8}]
    puts After-dict->[tcl::unsupported::representation $Z8]

    set time0 [time {set Z0 [ZIP_Main0 $L1 $L2]} $t]
    puts time0->$time0
    set time1 [time {ZIP_Main1 $L1 $L2} $t]
    puts time1->$time1
    set time2 [time {ZIP_Main2 $L1 $L2} $t]
    puts time2->$time2
    set time3 [time {ZIP_Main3 $L1 $L2} $t]
    puts time3->$time3
    set time4 [time {ZIP_Main4 $L1 $L2} $t]
    puts time4->$time4
    set time5 [time {ZIP_Main5 $L1 $L2} $t]
    puts time5->$time5
    set time6 [time {ZIP_Main6 $L1 $L2} $t]
    puts time6->$time6
    set time7 [time {ZIP_Main7 $L1 $L2} $t]
    puts time7->$time7
    puts Z0-check->[time {ZipCheck $L1 $L2 $Z0}]
    puts Z8-check->[time {ZipCheck $L1 $L2 $Z8}]
    puts [time {dict keys $Z0}]

    set time9 [time {set z9 [ZIP_Main8 $L1 $L3]}]
    puts time9->$time9
}

proc ZipCheck {0 1 zip} {
    set i 0
    set errors 0
    foreach a $0 b $1 {
	if {$a ne [lindex $zip $i]} {
	    puts "Mismatch($i) $a ne [lindex $zip $i]"
	    incr errors
	}
	incr i
	if {$b ne [lindex $zip $i]} {
	    puts "Mismatch($i) $b ne [lindex $zip $i]"
	    incr errors
	}
	incr i
    }
    puts "Errors=$errors"
}

# std call by value
proc ZIP_Main0 {L1 L2} {
    set LR [ZIP_Action0 $L1 $L2]
}

proc ZIP_Action0 {L1 L2} {
    foreach I1 $L1 I2 $L2 {
	lappend LR $I1 $I2
    }
    return $LR
}

# in args by ref
proc ZIP_Main1 {L1 L2} {
    set LR [ZIP_Action1 L1 L2]
}


proc ZIP_Action1 {cbrL1 cbrL2} {
    upvar $cbrL1 L1
    upvar $cbrL2 L2
    foreach I1 $L1 I2 $L2 {
	lappend LR $I1 $I2
    }
    return $LR
}

# all by ref
proc ZIP_Main2 {L1 L2} {
    set LR [list]
    ZIP_Action2 L1 L2 LR
}

proc ZIP_Action2 {cbrL1 cbrL2 cbrRes} {
    upvar $cbrL1 L1
    upvar $cbrL2 L2
    upvar $cbrRes LR
    foreach I1 $L1 I2 $L2 {
	lappend LR $I1 $I2
    }
}
# result by ref
proc ZIP_Main3 {L1 L2} {
    set LR [list]
    ZIP_Action3 $L1 $L2 LR
}


proc ZIP_Action3 {L1 L2 cbrRes} {
    upvar $cbrRes LR
    foreach I1 $L1 I2 $L2 {
	lappend LR $I1 $I2
    }
}

# # by ref and index
proc ZIP_Main4 {L1 L2} {
    set LR [list]
    ZIP_Action4 L1 L2 LR
}

proc ZIP_Action4 {cbrL1 cbrL2 cbrRes} {
    upvar $cbrL1 L1
    upvar $cbrL2 L2
    upvar $cbrRes LR
    set LR [list]
    set n [llength $L1]
    for {set i 0} {$i < $n} {incr i} {
	lappend LR [lindex $L1 $i] [lindex $L2 $i]
    }
    return $LR
}
# use lmap by val
proc ZIP_Main5 {L1 L2} {
    set LR [ZIP_Action5 $L1 $L2]
}

proc ZIP_Action5 {L1 L2} {
    return [lmap a $L1 b $L2 {list $a $b}]
}

# use lmap by ref
proc ZIP_Main6 {L1 L2} {
    set LR [ZIP_Action6 L1 L2]
}

proc ZIP_Action6 {cbrL1 cbrL2} {
    upvar $cbrL1 L1
    upvar $cbrL2 L2
    return [lmap a $L1 b $L2 {list $a $b}]
}


# use dict with unique ID's in L1
# use lmap by ref
proc ZIP_Main7 {L1 L2} {
    set LR [ZIP_Action7 $L1 $L2]
}

proc ZIP_Action7 {L1 L2} {
    set d [dict create]
    foreach I1 $L1 I2 $L2 {
	dict set d $I1 $I2
    }
    return [set d]
}

# use lweave
proc ZIP_Main8 {L1 L2} {
    set LR [ZIP_Action8 $L1 $L2]
}

proc ZIP_Action8 {L1 L2} {
    lweave $L1 $L2
}

proc Get_List1 {n} {
    set bl [list]
    for {set i 0} {$i < $n} {incr i} {
	lappend bl $i
    }
    return $bl
}

proc Get_List2 {n} {
    return [lrepeat $n {A B C d e F} ]
}

proc Get_List3 {n} {
    return [lrepeat $n [Get_List1 4]]
}

proc reverse_test {} {
    set x [lweave {a b c} {1 2 3}]
    puts x=$x
    set y [lreverse $x]
    puts y=$y
    puts $x\ ->\ \ $y

    puts "index 1 -> [lindex $x 1]"
    puts "rev-index 1 -> [lindex $y 1]"
}

puts Reverse\ Test
reverse_test

puts Start\ Task
Task
