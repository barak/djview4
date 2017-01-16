
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "libdjvu/ddjvuapi.h"


/* handle ddjvu messages */

static void
handle(ddjvu_context_t *ctx, int wait)
{
  const ddjvu_message_t *msg;
  if (!ctx)
    return;
  if (wait)
    msg = ddjvu_message_wait(ctx);
  while ((msg = ddjvu_message_peek(ctx)))
    {
      switch(msg->m_any.tag)
        {
	case DDJVU_ERROR:
	  NSLog(@"%s", msg->m_error.message);
	  if (msg->m_error.filename)
	    NSLog(@"'%s:%d'",
		  msg->m_error.filename, msg->m_error.lineno);
	default:
	  break;
        }
      ddjvu_message_pop(ctx);
    }
}

/* -----------------------------------------------------------------------------
   Generate a preview for file

   This function's job is to create preview for designated file
   ----------------------------------------------------------------------------- */

OSStatus
GeneratePreviewForURL(void *thisInterface,
                      QLPreviewRequestRef preview,
                      CFURLRef url,
                      CFStringRef contentTypeUTI,
                      CFDictionaryRef cfOptions)
{
  ddjvu_context_t *ctx = 0;
  ddjvu_document_t *doc = 0;
  ddjvu_format_t *fmt = 0;
  ddjvu_page_t *pag = 0;
  CGContextRef *cg = 0;
  
  @autoreleasepool {
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSDictionary *domain = [defaults persistentDomainForName:@"org.djvu.qlgenerator"];
    NSString *path = [(NSString *)CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle) autorelease];
    int width, height;
    int npages, page, maxpages = 5;
    
    /* Defaults */
    if (domain && [domain objectForKey:@"previewpages"])
      maxpages = [[domain objectForKey:@"previewpages"] intValue];
    if (maxpages <= 0)
      goto pop;
    
    /* Create context and document */
    if (! (ctx = ddjvu_context_create([[[NSProcessInfo processInfo] processName]
					cStringUsingEncoding:NSASCIIStringEncoding]))) {
      NSLog(@"Cannot create djvu context for '%@'.", path);
      goto pop;
    }
    if (! (doc = ddjvu_document_create_by_filename(ctx, [path fileSystemRepresentation], TRUE))) {
      NSLog(@"Cannot open djvu document '%@'.", path);
      goto pop;
    }
    while (! ddjvu_document_decoding_done(doc))
      handle(ctx, TRUE);
    if (ddjvu_document_decoding_error(doc)) {
      NSLog(@"Djvu document decoding error '%@'.", path);
      goto pop;
    }
    
    /* Loop on pages */
    fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, NULL);
    ddjvu_format_set_row_order(fmt, TRUE);
    npages = ddjvu_document_get_pagenum(doc);
    for (page=0; page<npages && page<maxpages; page++)
      {
        int pw, ph, dpi;
        ddjvu_rect_t rrect;
        NSBitmapImageRep *bitmap;
        NSDictionary *dict;
        NSData *data;
        CGRect cgrect;
        CGImageRef cgimg;

        @autoreleasepool {
          
          /* Decode page */
          pag = ddjvu_page_create_by_pageno(doc, page);
          while (! ddjvu_page_decoding_done(pag))
            handle(ctx, TRUE);
          if (ddjvu_page_decoding_error(pag)) {
            NSLog(@"Djvu page decoding error '%@' (page %d).", path, page);
            goto pop;
          }
          
          /* Obtain page size at 100 dpi */
          pw = ddjvu_page_get_width(pag);
          ph = ddjvu_page_get_height(pag);
          dpi = ddjvu_page_get_resolution(pag);
          if (pw <= 0 || ph <= 0 || dpi <= 0) {
            NSLog(@"Djvu page decoding error '%@' (page %d).", path, page);
            goto pop;
          }
          rrect.x = rrect.y = 0;
          rrect.w = pw * 150 / dpi;
          rrect.h = ph * 150 / dpi;
          
          /* Render page */
	  bitmap = [[NSBitmapImageRep alloc] autorelease];
	  bitmap = [bitmap initWithBitmapDataPlanes:NULL
					 pixelsWide:rrect.w
					 pixelsHigh:rrect.h
				      bitsPerSample:8
				    samplesPerPixel:3
					   hasAlpha:FALSE
					   isPlanar:NO
				     colorSpaceName:NSCalibratedRGBColorSpace
					bytesPerRow:rrect.w * 3
				       bitsPerPixel:24 ];
	if (! ddjvu_page_render(pag, DDJVU_RENDER_COLOR, &rrect, &rrect,
                                  fmt, rrect.w * 3, [bitmap bitmapData]) ) {
            NSLog(@"Djvu page rendering error '%@' (page %d).", path, page);
            goto pop;
          }
	  
          /* Draw bitmap into CG */
          cgrect.origin.x = cgrect.origin.y = 0;
          cgrect.size.width = rrect.w;
          cgrect.size.height = rrect.h;
          [bitmap setSize:cgrect.size];
          if (! cg)
            cg = QLPreviewRequestCreatePDFContext(preview, &cgrect, NULL, NULL);
          data = [NSData dataWithBytes:&cgrect length:sizeof(cgrect)];
          dict = [NSDictionary dictionaryWithObject:data forKey:kCGPDFContextMediaBox];
          CGPDFContextBeginPage(cg, (CFDictionaryRef)dict);
          cgimg = [bitmap CGImage];
          CGContextDrawImage(cg, cgrect, cgimg);

          /* Show when there are more pages (old qlgenerator) */
          if (page+1 == maxpages && page+1 < npages) {
            CGAffineTransform m;
            CFBundleRef bundle = QLPreviewRequestGetGeneratorBundle(preview);
            CFURLRef more = CFBundleCopyResourceURL(bundle,CFSTR("more_pages"),CFSTR("pdf"),NULL);
            CGPDFDocumentRef doc = CGPDFDocumentCreateWithURL(more);
            CGPDFPageRef pdf = CGPDFDocumentGetPage(doc, 1);
            CGContextSaveGState(cg);
            m = CGPDFPageGetDrawingTransform(pdf, kCGPDFMediaBox, cgrect, 0, true);
            CGContextConcatCTM(cg, m);
            CGContextDrawPDFPage(cg, pdf);
            CGContextRestoreGState(cg);
            CFRelease(doc);
          }

          /* Cleanup */
          CGPDFContextEndPage(cg);
          ddjvu_page_release(pag);
          pag = 0;
        }  /* @autoreleasepool */
      }
    /* Flush pdf context */
    if (cg)
      {
        CGPDFContextClose(cg);
	QLPreviewRequestFlushContext(preview, cg);	
      }
  } /* @autoreleasepool */
  /* Cleanup */
 pop:
  if (cg)
    CFRelease(cg);
  if (pag)
    ddjvu_page_release(pag);
  if (fmt)
    ddjvu_format_release(fmt);
  if (doc)
    ddjvu_document_release(doc);
  if (ctx)
    ddjvu_context_release(ctx);
  return noErr;
}


void CancelPreviewGeneration(void* thisInterface, QLPreviewRequestRef preview)
{
    // implement only if supported
}


