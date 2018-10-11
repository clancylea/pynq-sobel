# pynq-sobel
A demo for accelerating sobel in xilinx's fpga pynq
# 1.Clone or Download this repo
# 2.change the bit and picture path via your path

#the bitstream path (The name of .tcl must be same as the name of .bit)
overlay = Overlay("/home/xilinx/jupyter_notebooks/sobel/sobel.bit")

#image path
orig_img_path = "/home/xilinx/jupyter_notebooks/sobel/test_1080p.bmp"

# 3.run all and check the result
