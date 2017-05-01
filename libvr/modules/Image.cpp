//
//  Image.cpp
//  vrplayer
//
//  Created by 单强 on 16/9/26.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#include "Image.hpp"
#include "Util.h"
#include "jpeglib.h"
#include <setjmp.h>

#define MAX_READLINE 8

Image::Image()
:_width(0)
,_height(0) {
    
}

struct MyErrorMgr
{
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct MyErrorMgr * MyErrorPtr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
myErrorExit(j_common_ptr cinfo)
{
    /* cinfo->err really points to a MyErrorMgr struct, so coerce pointer */
    MyErrorPtr myerr = (MyErrorPtr) cinfo->err;
    
    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    /* internal message function cann't show error message in some platforms, so we rewrite it here.
     * edit it if has version confilict.
     */
    //(*cinfo->err->output_message) (cinfo);
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    LOGD("jpeg error: %s", buffer);
    
    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

int Image::getWidth() {
    return _width;
}

int Image::getHeight() {
    return _height;
}

string Image::decompressJpg(const string &compressedData) {
    /* these are standard libjpeg structures for reading(decompression) */
    struct jpeg_decompress_struct cinfo;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct MyErrorMgr jerr;
    /* libjpeg data structure for storing one row, that is, scanline of an image */
    JSAMPROW row_pointer[1] = {0};
    unsigned long location = 0;
    
    string ret;
    do
    {
        /* We set up the normal JPEG error routines, then override error_exit. */
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = myErrorExit;
        /* Establish the setjmp return context for MyErrorExit to use. */
        if (setjmp(jerr.setjmp_buffer))
        {
            /* If we get here, the JPEG code has signaled an error.
             * We need to clean up the JPEG object, close the input file, and return.
             */
            jpeg_destroy_decompress(&cinfo);
            break;
        }
        
        /* setup decompression process and source, then read JPEG header */
        jpeg_create_decompress( &cinfo );
        
        jpeg_mem_src(&cinfo,
                     reinterpret_cast<unsigned char*>(const_cast<char *>(compressedData.data())),                               compressedData.size());
        
        /* reading the image header which contains image information */
        jpeg_read_header(&cinfo, TRUE);
        
        cinfo.out_color_space = JCS_RGB;
        
        /* Start decompression jpeg here */
        jpeg_start_decompress( &cinfo );
        
        /* init image info */
        _width  = cinfo.output_width;
        _height = cinfo.output_height;
        
        ret.resize(cinfo.output_width*cinfo.output_height*cinfo.output_components);
        
        /* now actually read the jpeg into the raw buffer */
        /* read one scan line at a time */
        while (cinfo.output_scanline < cinfo.output_height)
        {
            row_pointer[0] = reinterpret_cast<unsigned char *>(&ret[0]) + location;
            location += cinfo.output_width*cinfo.output_components;
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }
        
        /* When read image file with broken data, jpeg_finish_decompress() may cause error.
         * Besides, jpeg_destroy_decompress() shall deallocate and release all memory associated
         * with the decompression object.
         * So it doesn't need to call jpeg_finish_decompress().
         */
        //jpeg_finish_decompress( &cinfo );
        jpeg_destroy_decompress( &cinfo );
        /* wrap up decompression, destroy objects, free pointers and close open files */        
    } while (0);
    
    return ret;
}