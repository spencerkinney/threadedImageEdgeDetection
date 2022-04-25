## Multithreaded Edge Detection
This is a program that utilizes OpenMP in order to read an image and output a resultant grayscale image that highlights the edges of the image.

This program uses standard c library functions to takes a BMP image and also produce the result as in the BMP file format. Any standard image format can be converted to the BMP format to use in this program.

### Usage:
***make*** to compile the program

***./edge_detection <input_file> <output_file> <n_threads>*** to run the program

Toggle the ***VERBOSE*** flag to 1 to output execution times to the console else toggle to 0