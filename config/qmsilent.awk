BEGIN { 
    print "vUIC=$(vUIC_$V)"
    print "vUIC_=$(vUIC_0)"
    print "vUIC_0=@echo \"  UIC     \" $@;"
    print "vRCC=$(vRCC_$V)"
    print "vRCC_=$(vRCC_0)"
    print "vRCC_0=@echo \"  RCC     \" $@;"
    print "vMOC=$(vMOC_$V)"
    print "vMOC_=$(vMOC_0)"
    print "vMOC_0=@echo \"  MOC     \" $@;"
    print "vCXX=$(vCXX_$V)"
    print "vCXX_=$(vCXX_0)"
    print "vCXX_0=@echo \"  CXX     \" $@;"
    print "vCC=$(vCC_$V)"
    print "vCC_=$(vCC_0)"
    print "vCC_0=@echo \"  CC      \" $@;"
    print "vLINK=$(vLINK_$V)"
    print "vLINK_=$(vLINK_0)"
    print "vLINK_0=@echo \"  LINK    \" $@;"
    print "vMISC=$(vMISC_$V)"
    print "vMISC_=$(vMISC_0)"
    print "vMISC_0=@"
    mayberule = 0
    escaped = 0
}

/^[^\t]/ {
    if (escaped == 0) { mayberule = 0 }
}

{
    match($0, /^[ \t]*-?/)
    tabs=substr($0,1,RLENGTH)
    line=substr($0,1+RLENGTH)
    if (escaped == 1) {
        print
    } else if (mayberule == 0) {
        print
    } else if ( $1 ~ /.*\/uic/ || $1 == "$(UIC)" || $1 == "${UIC}") {
        printf("%s$(vUIC)%s\n", tabs, line)
    } else if ( $1 ~ /.*\/rcc/ || $1 == "$(RCC)" || $1 == "${RCC}") {
        printf("%s$(vRCC)%s\n", tabs, line)
    } else if ( $1 ~ /.*\/moc/ || $1 == "$(MOC)" || $1 == "${MOC}") {
        printf("%s$(vMOC)%s\n", tabs, line)
    } else if ( $1 == "$(CXX)" || $1 == "${CXX}") {
        printf("%s$(vCXX)%s\n", tabs, line)
    } else if ( $1 == "$(CC)" || $1 == "${CC}") {
        printf("%s$(vCC)%s\n", tabs, line)
    } else if ( $1 == "$(LINK)" || $1 == "${LINK}") {
        printf("%s$(vLINK)%s\n", tabs, line)
    } else if ( $1 == "-$(DEL_FILE)" || $1 == "-${DEL_DIR}") {
        printf("%s$(vMISC)%s\n", tabs, line)
    } else {
        print
    }
}

/^[^:=\t]*:?=/ {
    if (escaped == 0) { mayberule = 0 }
}

/^[^:=\t]*:/ {
    if (escaped == 0) { mayberule = 1 }
}

/\\$/ {
    escaped = 1 ; next
}

{
    escaped = 0
}
