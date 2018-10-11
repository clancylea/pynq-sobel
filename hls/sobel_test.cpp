#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sobel.h"


int main () {
  int           	x,y;
  int			width, height;
  char *tempbuf;
  char *tempbuf1;
  int check_results = 0;

  tempbuf = (char *)malloc(2000 * sizeof(char));
  tempbuf1 = (char *)malloc(2000 * sizeof(char));

  hls_int32 *in_pix, *out_pix;

  //volatile hls_int32 *in_pix;

  in_pix  = (hls_int32 *) malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(hls_int32));
  out_pix = (hls_int32 *) malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(hls_int32));

  // Arrays to store image data
  unsigned char *R;
  unsigned char *G;
  unsigned char *B;
  

  R = (unsigned char *)malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(unsigned char));
  G = (unsigned char *)malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(unsigned char));
  B = (unsigned char *)malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(unsigned char));

  //Get image data from disk
  sprintf(tempbuf, "%s.bmp", INPUT_IMAGE_BASE);
  
  // Fill a frame with data
  int read_tmp = BMP_Read(tempbuf, MAX_HEIGHT, MAX_WIDTH, R, G, B);
  if(read_tmp != 0) {
    printf("%s Loading image failed\n", tempbuf);
    exit (1);
  }

  // Copy Input Image to pixel data structure
  // Hardware accelerator works on video pixel streams
   for(x = 0; x < MAX_HEIGHT; x++){
    for(y = 0; y < MAX_WIDTH; y++){
      RGB pixel;
      pixel.R = R[x*MAX_WIDTH + y];
      pixel.G = G[x*MAX_WIDTH + y];
      pixel.B = B[x*MAX_WIDTH + y];
      in_pix[x*MAX_WIDTH + y] = (pixel.R,pixel.G);
      in_pix[x*MAX_WIDTH + y] = (in_pix[x*MAX_WIDTH + y],pixel.B);
    }
   }
  
  // Hardware Function
   sobel_filter(in_pix, out_pix, 0, 0, 1080, 1920, 1920);

  // Copy Output video pixel stream to Output Image data structure
  for(x =0; x < MAX_HEIGHT; x++){
    for(y = 0; y < MAX_WIDTH; y++){
      RGB pixel;
      pixel.B = out_pix[x*MAX_WIDTH + y].range(7,0);
      pixel.G = out_pix[x*MAX_WIDTH + y].range(15,8);
      pixel.R = out_pix[x*MAX_WIDTH + y].range(23,16);
      R[x*MAX_WIDTH + y] = pixel.R.to_int();
      G[x*MAX_WIDTH + y] = pixel.G.to_int();
      B[x*MAX_WIDTH + y] = pixel.B.to_int();
    }
   }
  
  //Write the image back to disk
  sprintf(tempbuf1, "%s.bmp", OUTPUT_IMAGE_BASE);
  int write_tmp = BMP_Write(tempbuf1, MAX_HEIGHT, MAX_WIDTH, R, G, B);
  if(write_tmp != 0){ 
    printf("WriteBMP %s failed\n", tempbuf);
    exit(1);
  }
  free(R);
  free(G);
  free(B);
  free(tempbuf);
  free(tempbuf1);
  free(in_pix);
  free(out_pix);
  printf("Simulation Complete\n");
  check_results = system("diff result_1080p_golden.bmp result_1080p.bmp");
  if(check_results != 0){
    printf("Output image has mismatches with reference output image!\n");
    check_results = 1;
  }
  else{
    printf("Output image matches the reference output image\n");
  }
  return check_results;
}
