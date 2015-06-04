/*
 *  ddjvuRef.c
 *  QuickLookDjVu
 *
 *  Created by Jeff Sickel on 11/27/07.
 *  Copyright 2007 Corpus Callosum Corporation. All rights reserved.
 *
 */

#include "ddjvuRef.h"

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>

#include <sys/param.h> /* for MAXPATHLEN */
#include <sys/types.h>
#include <sys/stat.h>

static CFURLRef
CFURLCopyAppendAndCheck(CFURLRef url, CFStringRef path, Boolean isdir)
{
    struct stat fstat;
    char buf[MAXPATHLEN];
    url = CFURLCreateWithFileSystemPathRelativeToBase(NULL, path, kCFURLPOSIXPathStyle, isdir, url);
    if (CFURLGetFileSystemRepresentation(url, true, (UInt8 *)buf, MAXPATHLEN))
        if (stat(buf, &fstat) >= 0)
             return url;
    CFRelease(url);
    return 0;
} 

static char* 
ddjvuPathNoCache(CFBundleRef bundle)
{
    CFURLRef ddjvuRef = NULL;
    CFURLRef djviewRef = NULL;
    CFURLRef qlRef = NULL;

    /* Using bundle */
    if (!ddjvuRef && bundle && !qlRef)
        qlRef = CFBundleCopyExecutableURL(bundle);
    if (!ddjvuRef && qlRef)
	ddjvuRef = CFURLCopyAppendAndCheck(qlRef, CFSTR("../../../../../bin/ddjvu"), false);
    if (!ddjvuRef)
        bundle = CFBundleGetBundleWithIdentifier(CFSTR("org.djvu.DjView"));
    if (!ddjvuRef && bundle)
        ddjvuRef = CFBundleCopyAuxiliaryExecutableURL(bundle, CFSTR("ddjvu"));
    if (!ddjvuRef && bundle)
        ddjvuRef = CFBundleCopyResourceURL(bundle, CFSTR("ddjvu"), NULL, NULL);
    if (!ddjvuRef && !djviewRef && bundle)
        djviewRef = CFBundleCopyExecutableURL(bundle);
    if (!ddjvuRef && djviewRef) 
	ddjvuRef = CFURLCopyAppendAndCheck(djviewRef, CFSTR("../bin/ddjvu"), false);
    if (!ddjvuRef && djviewRef) 
	ddjvuRef = CFURLCopyAppendAndCheck(djviewRef, CFSTR("ddjvu"), false);
    if (djviewRef) 
        CFRelease(djviewRef);
    if (qlRef)
        CFRelease(qlRef);

    char *ans = NULL;
    char buf[MAXPATHLEN];
    if (CFURLGetFileSystemRepresentation(ddjvuRef, true, (UInt8 *)buf, MAXPATHLEN))
        ans = strdup(buf);
    if (ddjvuRef)
        CFRelease(ddjvuRef);
    return ans;
}

char *
ddjvuPath(CFBundleRef bundle)
{
    struct stat fstat;
    static char *ans = NULL;
    if (ans == NULL || stat(ans, &fstat) < 0)
        ans = ddjvuPathNoCache(bundle);
    return ans;
}
