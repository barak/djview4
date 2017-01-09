/*
 *  ddjvuRef.h
 *  QuickLookDjVu
 *
 *  Created by Jeff Sickel on 11/27/07.
 *  Copyright 2007 Corpus Callosum Corporation. All rights reserved.
 *
 */

#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>

char *ddjvuPath(CFBundleRef bundle);

void ddjvuLimitSize(CGSize *size);
