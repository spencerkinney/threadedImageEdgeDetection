#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BLURFACTOR 90.0

typedef struct {
  u_int8_t red;
  u_int8_t green;
  u_int8_t blue;
} Pixel;

void convert_grayscale(int width, int height, Pixel image[height][width]);

void apply_gaussian_blur(int width, int height, Pixel image[height][width]);

void apply_sobel_filters(int width, int height, Pixel image[height][width]);

void apply_non_max_supression(int width, int height,
                              Pixel image[height][width]);

void calculate_edge_correlation(int width, int height,
                                Pixel image[height][width]);

void edge_tracking_hysteresis(int width, int height,
                              Pixel image[height][width]);

int main(int argc, char *argv[]) {
  char *in_name, *out_name;
  int num_threads;

  if (argc != 4) {
    fprintf(stderr, "Incorrect number of command line inputs\n");
    exit(1);
  }

  // Assign command line arguments
  in_name = argv[1];
  out_name = argv[2];
  num_threads = atoi(argv[3]);

  omp_set_num_threads(num_threads);

  // Create files... Should take from command line input eventually
  FILE *in = fopen(in_name, "rb");
  FILE *out = fopen(out_name, "wb");
  if (!in || !out) {
    printf("File error.\n");
    return 0;
  }

  // Get base information from bitmap header
  unsigned char header[54];
  fread(header, sizeof(unsigned char), 54, in);
  fwrite(header, sizeof(unsigned char), 54, out);

  // Assign basic information to variables
  int width = *(int *)&header[18];
  int height = abs(*(int *)&header[22]);
  int stride = (width * 3 + 3) & ~3;
  int padding = stride - width * 3;

  // Create array of pixels
  Pixel(*image)[width] = calloc(height, width * sizeof(Pixel));

  // Populate array of pixels
  for (int i = 0; i < height; i++) {
    fread(image[i], sizeof(Pixel), width, in);
    fseek(in, padding, SEEK_CUR);
  }

  // Convert image to grayscale
  convert_grayscale(width, height, image);

  // Gaussian Blur
  apply_gaussian_blur(width, height, image);

  // Gradient Calculation
  apply_sobel_filters(width, height, image);

  // Edge Detection

  // Write final image to output file
  for (int i = 0; i < height; i++) {
    fwrite(image[i], sizeof(Pixel), width, out);
    for (int j = 0; j < padding; j++) {
      fputc(0x00, out);
    }
  }
  free(image);
  fclose(out);
  fclose(in);

  return 0;
}

void convert_grayscale(int width, int height, Pixel image[height][width]) {
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int x = (image[i][j].red + image[i][j].green + image[i][j].blue) / 3;
      image[i][j].red = round(x);
      image[i][j].green = round(x);
      image[i][j].blue = round(x);
    }
  }
}

void apply_gaussian_blur(int width, int height, Pixel image[height][width]) {
    Pixel temp_image[height][width];
    float avg = 0;
    float rgb[3] = {0, 0, 0};

    for (int i = 0; i < height; i++) {
        for ( int j = 0; j < width; j++) {
            for ( int k = -1; k < 2; k++) {
                for ( int l = -1; l < 2; l++) {
                    if (i + k >= 0 && j + l >= 0 && i + k < height && j + l < width) {
                        rgb[0] += image[i + k][j + l].red;
                        rgb[1] += image[i + k][j + l].green;
                        rgb[2] += image[i + k][j + l].blue;
                        avg++;
                    }
                }
            }

            // Update image values in temp image array
            temp_image[i][j].red = round(rgb[0] / avg);
            temp_image[i][j].blue = round(rgb[2] / avg);
            temp_image[i][j].green = round(rgb[1] / avg);
            
            // Reset values
            rgb[0] = 0; 
            rgb[1] = 0; 
            rgb[2] = 0;
            avg = 0; 
        }
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            image[i][j] = temp_image[i][j];
        }
    }
}


// http://www.cs.ucr.edu/~jtarango/cs122a_project.html
//https://github.com/fzehracetin/sobel-edge-detection-in-c/blob/main/sobel_edge_detection.c
void apply_sobel_filters(int width, int height, Pixel image[height][width]) {
  int kernelx[3][3] = {{-1,0,1}, {-2,0,2}, {-1,0,1}};
  

  for (int i = 1; i < height-1; i++) {
    for (int j = 1; j < width-1; j++) {
      Pixel magX;
      for (int k = 0; k < 3; k++) {
        for (int l = 0; l < 3; l++) {
          magX = image[height - 1 - k][width - 1 + l];
          magX.red *= kernelx[k][l];
          magX.green *= kernelx[k][l];
          magX.blue *= kernelx[k][l];
        }
      }
      image[i][j].red = magX.red;
      image[i][j].green = magX.green;
      image[i][j].blue = magX.blue;
      
    }
  }
}

void apply_non_max_supression(int width, int height, Pixel image[height][width]) {

  int Z[width][height];
}



// def non_max_suppression(img, D):
//     M, N = img.shape
//     Z = np.zeros((M,N), dtype=np.int32)
//     angle = D * 180. / np.pi
//     angle[angle < 0] += 180

    
//     for i in range(1,M-1):
//         for j in range(1,N-1):
//             try:
//                 q = 255
//                 r = 255
                
//                #angle 0
//                 if (0 <= angle[i,j] < 22.5) or (157.5 <= angle[i,j] <= 180):
//                     q = img[i, j+1]
//                     r = img[i, j-1] 
//                 #angle 45
//                 elif (22.5 <= angle[i,j] < 67.5):
//                     q = img[i+1, j-1]
//                     r = img[i-1, j+1]
//                 #angle 90
//                 elif (67.5 <= angle[i,j] < 112.5):
//                     q = img[i+1, j]
//                     r = img[i-1, j]
//                 #angle 135
//                 elif (112.5 <= angle[i,j] < 157.5):
//                     q = img[i-1, j-1]
//                     r = img[i+1, j+1]

//                 if (img[i,j] >= q) and (img[i,j] >= r):
//                     Z[i,j] = img[i,j]
//                 else:
//                     Z[i,j] = 0

//             except IndexError as e:
//                 pass
    
//     return Z