
BEGIN {
    printf "qmake_sources=\n";
    printf "qmake_ts=\n";
    printf "qmake_qm=\n"
    srcs["SOURCES"]=1;
    srcs["HEADERS"]=1;
    srcs["FORMS"]=1;
    srcs["RESOURCES"]=1;
    srcs["QMAKE_INFO_PLIST"]=1;
    srcs["REZ_FILES"]=1;
    srcs["RC_FILE"]=1;
    srcs["DEF_FILE"]=1;
}

/^[ \t]*[A-Za-z_]+[ \t]*[+*]?=/ { 
    gsub(/[+*]?=/," += "); 
    if ($1 in srcs) {
        $1="qmake_sources"; 
        print
    } 
    if ($1 == "TRANSLATIONS") {
        gsub(/[+*]?=/," += "); 
        $1="qmake_ts"; 
        print;  
        $1="qmake_qm"; 
        for (i=3; i<=NF; i++) gsub(/[.]ts/,".qm", $i) ; 
        print 
    }
}
