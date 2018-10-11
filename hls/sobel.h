#ifndef _IMAGE_DEMO_H_
#define _IMAGE_DEMO_H_

#include "ap_bmp.h"
#include "ap_video.h"

#define BURST_NUM 1920


#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080


// I/O Image Settings
#define INPUT_IMAGE_BASE "test_1080p"
#define OUTPUT_IMAGE_BASE "result_1080p"

#define ABSDIFF(x,y)	((x>y)? x - y : y - x)
#define ABS(x)          ((x>0)? x : -x)


typedef ap_int<32> hls_int32;
typedef ap_uint<32> hls_uint32;

typedef ap_rgb<8,8,8> RGB;
typedef ap_window<unsigned char,3,3> WINDOW;
typedef ap_linebuffer<unsigned char, 3, BURST_NUM> Y_BUFFER;
typedef ap_linebuffer<RGB,2,BURST_NUM> RGB_BUFFER;


void sobel_filter(hls_int32 *in_pix, hls_int32 *out_pix, unsigned int byte_rdoffset, unsigned int byte_wroffset, int rows, int cols, int stride);


#endif /*SOBEL_H*/
