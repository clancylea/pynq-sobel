/************************************  Header Section  *********************************************
 --
 -- NAME       : PYNQ z1
 -- Data       : sep, 2018

 -- Filename   : sobel_hls.cpp
 -- Description: Sobel Filter HLS Synthesis Code
 --
*********************************** End Header Section  *********************************************/


#include<stdio.h>
#include"sobel.h"

// RGB to Y Conversion
// Resulting luminance value used in edge detection
//y= R*0.257 + G*0.564 + B*0.098 + 16

unsigned char rgb2y(RGB pix)
{

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
	for(i=0; i < 3; i++)
	{
		for(j = 0; j < 3; j++)
		{
			// X direction gradient
			x_weight = x_weight + (window->getval(i,j) * x_op[i][j]);

			// Y direction gradient
			y_weight = y_weight + (window->getval(i,j) * y_op[i][j]);
		}
	}

	edge_weight = ABS(x_weight) + ABS(y_weight);

	if(edge_weight>255)
	{
		edge_weight =  255;
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



void sobel_filter(hls_int32 *in_pix, hls_int32 *out_pix, unsigned int byte_rdoffset, unsigned int byte_wroffset,int rows, int cols, int stride)

{


	int addr_reserved=0;
//#pragma HLS RESOURCE variable=inout_pix core=AXI4M
	//Data Flow Bus(Read and Write)
#pragma HLS INTERFACE m_axi depth=2048 port=in_pix offset=off bundle=Data_Bus
#pragma HLS INTERFACE m_axi depth=2048 port=out_pix offset=off bundle=Data_Bus
	//Control Bus
#pragma HLS INTERFACE s_axilite register port=return offset=0x00 bundle=Ctrl_Bus
#pragma HLS INTERFACE s_axilite register port=byte_rdoffset offset=0x14 bundle=Ctrl_Bus
#pragma HLS INTERFACE s_axilite register port=byte_wroffset offset=0x1c bundle=Ctrl_Bus
#pragma HLS INTERFACE s_axilite register port=rows offset=0x24 bundle=Ctrl_Bus
#pragma HLS INTERFACE s_axilite register port=cols offset=0x2c bundle=Ctrl_Bus
#pragma HLS INTERFACE s_axilite register port=stride offset=0x34 bundle=Ctrl_Bus
	//Ensure Address Range
#pragma HLS INTERFACE s_axilite register port=addr_reserved offset=0xFF0 bundle=Ctrl_Bus

	hls_int32 i;
	hls_int32 burst_buff_in[BURST_NUM], burst_buff_out[BURST_NUM];

	int row;
	int col;

	Y_BUFFER buff_A;
	WINDOW buff_C;

	for(row=0;row<rows+1;row++)
	{
		//memory coyin
		if(row<rows)
		{

			memcpy(burst_buff_in, (hls_uint32 *)(in_pix+byte_rdoffset/4 + row*stride), cols*sizeof(hls_uint32));
		}



		for(col=0; col<cols+1;col++)
		{
#pragma HLS PIPELINE II=1
			unsigned char temp;
			RGB tempx;
			unsigned char test_val;
			//line buff fill
			if(col<cols)
			{
				buff_A.shift_up(col);
				temp = buff_A.getval(0,col);
			}

			//There is an offset to accodate the active pixel region
			//There are only MAX_WIDTH and MAX_HEIGHT valid pixels in the image

			if(col<cols & row<rows)
			{
				RGB new_pix;
				hls_int32 input_pixel;
				hls_int32 pixel_num;

				input_pixel =burst_buff_in[col];
				new_pix.B = input_pixel.range(7,0);
				new_pix.G = input_pixel.range(15,8);
				new_pix.R = input_pixel.range(23,16);
				tempx= new_pix;
				buff_A.insert_bottom(rgb2y(tempx),col);
			}
		//Shift the processing window to make room for the new column
				buff_C.shift_right();
			//The Sobel processing window only needs to store luminance values
			//rgb2y function computes the luminance from the color pixel
			     if(col < cols){
				buff_C.insert(buff_A.getval(2,col),0,2);
				buff_C.insert(temp,1,2);
				buff_C.insert(rgb2y(tempx),2,2);
//				printf("%d", temp);
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
			    if(row > 0 && col > 0)
			    			{
			    				hls_int32 output_pixel;
			    				ap_uint<8> padding = 0xff;
			    				output_pixel = (edge.R,edge.G);
			    				output_pixel = (output_pixel, edge.B);
			    				output_pixel = (padding,output_pixel);

			    				burst_buff_out[col-1] = output_pixel;
			    			}

		}

		if(row>0)
		{
		memcpy((hls_uint32 *)(out_pix+byte_wroffset/4 + (row-1)*stride), burst_buff_out, cols*sizeof(hls_uint32));

		}

	}

}





