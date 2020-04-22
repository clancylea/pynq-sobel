

#include <stdio.h>
#include "sobel.h"
#include <string.h>

#include <stdio.h>
#include "sobel.h"

// RGB to Y Conversion
// Resulting luminance value used in edge detection
unsigned char rgb2y(RGB pix)
{
#pragma HLS INLINE
  unsigned char y;
  y = ((66 * pix.R.to_int() + 129 * pix.G.to_int() + 25 * pix.B.to_int() + 128) >> 8) + 16;

  return y;
}

//Sobel Computation using a 3x3 neighborhood
RGB sobel_operator(WINDOW *window)
{
  short x_weight = 0;
  short y_weight = 0;

  short edge_weight;
  unsigned char edge_val;
  RGB pixel;

  char i;
  char j;


  const char x_op[3][3] = { {-1,0,1},
			    {-2,0,2},
			    {-1,0,1}};

  const char y_op[3][3] = { {1,2,1},
			    {0,0,0},
			    {-1,-2,-1}};

  //Compute approximation of the gradients in the X-Y direction
  for(i=0; i < 3; i++){
    for(j = 0; j < 3; j++){

      // X direction gradient
      x_weight = x_weight + (window->getval(i,j) * x_op[i][j]);

      // Y direction gradient
      y_weight = y_weight + (window->getval(i,j) * y_op[i][j]);
    }
  }

  edge_weight = ABS(x_weight) + ABS(y_weight);

  if(edge_weight > 255)
  {
//	printf("edge_weight = %d,uchar = %d\n",edge_weight,(unsigned char)edge_weight);
	edge_weight = 255;
  }

  edge_val = (255-(unsigned char)(edge_weight));

  //Edge thresholding
  if(edge_val > 200)
    edge_val = 255;
  else if(edge_val < 100)
    edge_val = 0;

  pixel.R = pixel.G = pixel.B = edge_val;

  return pixel;
}

void LoadPixelM2B(PIXEL *inter_pix,PIXEL input_buffer[MAX_WIDTH],int rows,int cols,int row,int row_l2p[1])
{
	static int in_offset;
	static int rows_c;
	static int cols_c;

	row_l2p[0] = row;

	if(row == 0)
	{
		in_offset = 0;
		rows_c = rows;
		cols_c = cols;
	}

	bool enable = row < rows_c;

	if(enable)
	{
		memcpy(input_buffer,(PIXEL *)(inter_pix + in_offset),cols_c*sizeof(PIXEL));
		in_offset += cols_c;
	}
}

void StorePixelB2M(PIXEL *out_pix,PIXEL output_buffer[MAX_WIDTH],int cols,int row_p2s[1])
{
	static int out_offset;
	static int cols_c;

	int row = row_p2s[0];
	if(row == 0){
		out_offset = 0;
		cols_c = cols;
	}
	bool enable = row > 0;
	if(enable)
	{
		memcpy((PIXEL *)(out_pix + out_offset),output_buffer,cols_c*sizeof(PIXEL));
		out_offset += cols_c;
	}
}

void Process(PIXEL input_buffer[MAX_WIDTH],PIXEL output_buffer[MAX_WIDTH],int rows,int cols,int row_l2p[1],int row_p2s[1])
{
	int col;
	int in_offset = 0;
	int out_offset = 0;

	Y_BUFFER buff_A;
	RGB_BUFFER buff_B;
	WINDOW buff_C;


	int row = row_l2p[0];
	for(col = 0; col < cols+1; col++){
#pragma HLS LOOP_TRIPCOUNT min=1920 max=1920
#pragma HLS DEPENDENCE variable=buff_A inter false
#pragma HLS PIPELINE II=1

		// Temp values are used to reduce the number of memory reads
		unsigned char temp;
//			RGB tempx;
		unsigned char tmpx_y;
		unsigned char test_val;

		//Line Buffer fill
		if(col < cols){
			buff_A.shift_up(col);
			temp = buff_A.getval(0,col);
		}

		//There is an offset to accomodate the active pixel region
		//There are only MAX_WIDTH and MAX_HEIGHT valid pixels in the image
		if(col < cols & row < rows){
			RGB new_pix;
			PIXEL input_pixel;
//				input_pixel = inter_pix[row][col];
			input_pixel = input_buffer[col];
			new_pix.B = input_pixel.range(7,0);
			new_pix.G = input_pixel.range(15,8);
			new_pix.R = input_pixel.range(23,16);
			tmpx_y = rgb2y(new_pix);
			buff_A.insert_bottom(tmpx_y,col);
		}

		//Shift the processing window to make room for the new column
		buff_C.shift_right();

		//The Sobel processing window only needs to store luminance values
		//rgb2y function computes the luminance from the color pixel
		if(col < cols){
			buff_C.insert(buff_A.getval(2,col),0,2);
			buff_C.insert(temp,1,2);
			buff_C.insert(tmpx_y,2,2);
		}
		RGB edge;

		//The sobel operator only works on the inner part of the image
		//This design assumes there are no edges on the boundary of the image
		if( row <= 1 || col <= 1 || row > (rows-1) || col > (cols-1)){
			edge.R = edge.G = edge.B = 0;
		}
		else{
		//Sobel operation on the inner portion of the image
			edge = sobel_operator(&buff_C);
		}

		//The output image is offset from the input to account for the line buffer
		if(row > 0 && col > 0){
			PIXEL output_pixel;
			ap_uint<8> padding = 0xff;
			output_pixel = (edge.R,edge.G);
			output_pixel = (output_pixel, edge.B);
			output_pixel = (padding,output_pixel);
//				out_pix[row-1][col-1] = output_pixel;
//				out_pix[(row-1)*cols + (col-1)] = output_pixel;
			output_buffer[col-1] = output_pixel;
		}
	}

	row_p2s[0] = row;
}


void sobel_filter(PIXEL *inter_pix,PIXEL *out_pix, int rows, int cols)
{
#pragma HLS INTERFACE m_axi depth=2073600 port=inter_pix offset=slave bundle=DATA_IN
#pragma HLS INTERFACE m_axi depth=2073600 port=out_pix offset=slave bundle=DATA_OUT

#pragma HLS INTERFACE s_axilite register port=return bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite register port=inter_pix bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite register port=out_pix bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite register port=rows bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite register port=cols bundle=CTRL_BUS

	PIXEL input_buffer[MAX_WIDTH];
	PIXEL output_buffer[MAX_WIDTH];

	int row;
	int row_l2p[1];
	int row_p2s[1];
	for(row = 0; row < rows+1; row++){
#pragma HLS LOOP_TRIPCOUNT min=1080 max=1080
#pragma HLS DATAFLOW
		LoadPixelM2B(inter_pix, input_buffer, rows, cols, row, row_l2p);

		Process(input_buffer, output_buffer, rows, cols, row_l2p, row_p2s);

		StorePixelB2M(out_pix, output_buffer, cols, row_p2s);
	}
}

//Main function for Sobel Filtering
//This function includes a line buffer for a streaming implementation
//void sobel_filter(PIXEL *inter_pix,PIXEL *out_pix, int rows, int cols)
//{
//#pragma HLS INTERFACE m_axi depth=2073600 port=inter_pix offset=slave bundle=DATA_IN
//#pragma HLS INTERFACE m_axi depth=2073600 port=out_pix offset=slave bundle=DATA_OUT
//
//#pragma HLS INTERFACE s_axilite register port=return bundle=CTRL_BUS
//#pragma HLS INTERFACE s_axilite register port=inter_pix bundle=CTRL_BUS
//#pragma HLS INTERFACE s_axilite register port=out_pix bundle=CTRL_BUS
//#pragma HLS INTERFACE s_axilite register port=rows bundle=CTRL_BUS
//#pragma HLS INTERFACE s_axilite register port=cols bundle=CTRL_BUS
//
//	PIXEL input_buffer[MAX_WIDTH];
//	PIXEL output_buffer[MAX_WIDTH];
//
//	int row;
////	int col;
////	int in_offset = 0;
////	int out_offset = 0;
//
////	Y_BUFFER buff_A;
////	RGB_BUFFER buff_B;
////	WINDOW buff_C;
//
//	for(row = 0; row < rows+1; row++){
//
//		LoadPixelM2B(inter_pix, input_buffer, rows, cols, row);
//
//		Process( input_buffer, output_buffer, rows, cols, row);
////		if(row < rows)
////		{
////			memcpy(input_buffer,(PIXEL *)(inter_pix + in_offset),cols*sizeof(PIXEL));
////			in_offset += cols;
////		}
//
////		for(col = 0; col < cols+1; col++){
////#pragma HLS LOOP_FLATTEN off
////#pragma HLS DEPENDENCE variable=buff_A inter false
////#pragma HLS PIPELINE II=1
////
////			// Temp values are used to reduce the number of memory reads
////			unsigned char temp;
//////			RGB tempx;
////			unsigned char tmpx_y;
////			unsigned char test_val;
////
////			//Line Buffer fill
////			if(col < cols){
////				buff_A.shift_up(col);
////				temp = buff_A.getval(0,col);
////			}
////
////			//There is an offset to accomodate the active pixel region
////			//There are only MAX_WIDTH and MAX_HEIGHT valid pixels in the image
////			if(col < cols & row < rows){
////				RGB new_pix;
////				PIXEL input_pixel;
//////				input_pixel = inter_pix[row][col];
////				input_pixel = input_buffer[col];
////				new_pix.B = input_pixel.range(7,0);
////				new_pix.G = input_pixel.range(15,8);
////				new_pix.R = input_pixel.range(23,16);
////				tmpx_y = rgb2y(new_pix);
////				buff_A.insert_bottom(tmpx_y,col);
////			}
////
////			//Shift the processing window to make room for the new column
////			buff_C.shift_right();
////
////			//The Sobel processing window only needs to store luminance values
////			//rgb2y function computes the luminance from the color pixel
////			if(col < cols){
////				buff_C.insert(buff_A.getval(2,col),0,2);
////				buff_C.insert(temp,1,2);
////				buff_C.insert(tmpx_y,2,2);
////			}
////			RGB edge;
////
////			//The sobel operator only works on the inner part of the image
////			//This design assumes there are no edges on the boundary of the image
////			if( row <= 1 || col <= 1 || row > (rows-1) || col > (cols-1)){
////				edge.R = edge.G = edge.B = 0;
////			}
////			else{
////			//Sobel operation on the inner portion of the image
////				edge = sobel_operator(&buff_C);
////			}
////
////			//The output image is offset from the input to account for the line buffer
////			if(row > 0 && col > 0){
////				PIXEL output_pixel;
////				ap_uint<8> padding = 0xff;
////				output_pixel = (edge.R,edge.G);
////				output_pixel = (output_pixel, edge.B);
////				output_pixel = (padding,output_pixel);
//////				out_pix[row-1][col-1] = output_pixel;
//////				out_pix[(row-1)*cols + (col-1)] = output_pixel;
////				output_buffer[col-1] = output_pixel;
////			}
////		}
//
//		StorePixelB2M(out_pix, output_buffer, cols, row);
////		if(row > 0)
////		{
////			memcpy((PIXEL *)(out_pix + out_offset),output_buffer,cols*sizeof(PIXEL));
////			out_offset += cols;
////		}
//
//	}
//}

//// RGB to Y Conversion
//// Resulting luminance value used in edge detection
//unsigned char rgb2y(RGB pix)
//{
//  unsigned char y;
//
////  y = ((66 * pix.R.to_int() + 129 * pix.G.to_int() + 25 * pix.B.to_int() + 128) >> 8) + 16;
//  ap_uint<16> R_u16 = 66 * pix.R;
//  ap_uint<16> G_u16 = 129 * pix.G;
//  ap_uint<16> B_u16 = 25 * pix.B;
//  y = ((R_u16 + G_u16 + B_u16 + 128) >> 8) + 16;
//
//  return y;
//}
//
////Sobel Computation using a 3x3 neighborhood
//RGB sobel_operator(WINDOW *window)
//{
//  short x_weight = 0;
//  short y_weight = 0;
//
//  short edge_weight;
//  unsigned char edge_val;
//  RGB pixel;
//
//  char i;
//  char j;
//
//
//  const char x_op[3][3] = { {-1,0,1},
//			    {-2,0,2},
//			    {-1,0,1}};
//
//  const char y_op[3][3] = { {1,2,1},
//			    {0,0,0},
//			    {-1,-2,-1}};
//
//  //Compute approximation of the gradients in the X-Y direction
//  for(i=0; i < 3; i++){
//    for(j = 0; j < 3; j++){
//
//      // X direction gradient
//      x_weight = x_weight + (window->getval(i,j) * x_op[i][j]);
//
//      // Y direction gradient
//      y_weight = y_weight + (window->getval(i,j) * y_op[i][j]);
//    }
//  }
//
//  edge_weight = ABS(x_weight) + ABS(y_weight);
//
//  edge_val = (255-(unsigned char)(edge_weight));
//
//  //Edge thresholding
//  if(edge_val > 200)
//    edge_val = 255;
//  else if(edge_val < 100)
//    edge_val = 0;
//
//  pixel.R = pixel.G = pixel.B = edge_val;
//
//  return pixel;
//}
//
////Main function for Sobel Filtering
////This function includes a line buffer for a streaming implementation
//void sobel_filter(AXI_PIXEL inter_pix[MAX_HEIGHT][MAX_WIDTH],AXI_PIXEL out_pix[MAX_HEIGHT][MAX_WIDTH], int rows, int cols)
//{
//#pragma HLS INTERFACE axis register both port=inter_pix
//#pragma HLS INTERFACE axis register both port=out_pix
//#pragma HLS INTERFACE s_axilite register port=return bundle=CTRL_BUS
//#pragma HLS INTERFACE s_axilite register port=rows bundle=CTRL_BUS
//#pragma HLS INTERFACE s_axilite register port=cols bundle=CTRL_BUS
//
//  int row;
//  int col;
//
//  Y_BUFFER buff_A;
//  RGB_BUFFER buff_B;
//  WINDOW buff_C;
//
//  for(row = 0; row < rows+1; row++){
//    for(col = 0; col < cols+1; col++){
//#pragma HLS LOOP_FLATTEN off
//#pragma HLS DEPENDENCE variable=buff_A inter false
//#pragma HLS PIPELINE II=1
//
//      // Temp values are used to reduce the number of memory reads
//      unsigned char temp;
//      RGB tempx;
//      unsigned char test_val;
//
//      //Line Buffer fill
//      if(col < cols){
//	buff_A.shift_up(col);
//	temp = buff_A.getval(0,col);
//      }
//
//      //There is an offset to accomodate the active pixel region
//      //There are only MAX_WIDTH and MAX_HEIGHT valid pixels in the image
//      if(col < cols & row < rows){
//	RGB new_pix;
//	AXI_PIXEL input_pixel;
//	input_pixel = inter_pix[row][col];
//	new_pix.B = input_pixel.data.range(7,0);
//	new_pix.G = input_pixel.data.range(15,8);
//	new_pix.R = input_pixel.data.range(23,16);
//	tempx = new_pix;;
//	buff_A.insert_bottom(rgb2y(tempx),col);
//      }
//
//      //Shift the processing window to make room for the new column
//      buff_C.shift_right();
//
//      //The Sobel processing window only needs to store luminance values
//      //rgb2y function computes the luminance from the color pixel
//      if(col < cols){
//	buff_C.insert(buff_A.getval(2,col),0,2);
//	buff_C.insert(temp,1,2);
//	buff_C.insert(rgb2y(tempx),2,2);
//      }
//      RGB edge;
//
//      //The sobel operator only works on the inner part of the image
//      //This design assumes there are no edges on the boundary of the image
//      if( row <= 1 || col <= 1 || row > (rows-1) || col > (cols-1)){
//	edge.R = edge.G = edge.B = 0;
//      }
//      else{
//
//	//Sobel operation on the inner portion of the image
//	edge = sobel_operator(&buff_C);
//      }
//
//      //The output image is offset from the input to account for the line buffer
//      if(row > 0 && col > 0){
//	AXI_PIXEL output_pixel;
//	ap_uint<8> padding = 0xff;
//	output_pixel.data = (edge.R,edge.G);
//	output_pixel.data = (output_pixel.data, edge.B);
//	output_pixel.data = (padding,output_pixel.data);
//	output_pixel.strb = 15;
//	output_pixel.user = 1;
//	output_pixel.tdest = 1;
//	if((col == cols))
//	  output_pixel.last = 1;
//	else
//	  output_pixel.last = 0;
//	out_pix[row-1][col-1] = output_pixel;
//      }
//    }
//  }
//}
