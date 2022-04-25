#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BLURFACTOR 90.0
#define VERBOSE 1

typedef struct {
  u_int8_t red;
  u_int8_t green;
  u_int8_t blue;
} Pixel;

// Function to convert the image to grayscale
void convert_grayscale(int width, int height,
                       Pixel image[height][width]);

// Function to aply gaussian blur
void apply_gaussian_blur(int width, int height,
                         Pixel image[height][width]);

void apply_edge_gradient(int width, int height,
                         Pixel image[height][width]);

int main(int argc, char *argv[]) {
  char *in_name, *out_name;
  int num_threads;

  // Make sure correct number of arguments entered
  if (argc != 4) {
    fprintf(stderr, "Incorrect number of command line inputs\n");
    exit(1);
  }

  // Assign command line arguments
  in_name = argv[1];
  out_name = argv[2];
  num_threads = atoi(argv[3]);

  // Set number of threads
  omp_set_num_threads(num_threads);

  // Create/open files
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

  printf("Using %d threads\n", num_threads);  

  // Time 
  double start, elapsed;
  double total = 0.0;
  
  // Convert image to grayscale
  start = omp_get_wtime();
  convert_grayscale(width, height, image);
  elapsed = omp_get_wtime() - start;
  total += elapsed;
  if (VERBOSE) {
    printf("CONVERT_TO_GRAYSCALE EXEC TIME: %fs\n", elapsed);
  }

  // Gaussian Blur
  start = omp_get_wtime();
  apply_gaussian_blur(width, height, image);
  elapsed = omp_get_wtime() - start;
  total += elapsed;
  if (VERBOSE) {
    printf("APPLY_GAUSSIAN_BLUR EXEC TIME: %fs\n", elapsed);
  }

  // Gradient Calculation
  start = omp_get_wtime();
  apply_edge_gradient(width, height, image);
  elapsed = omp_get_wtime() - start;
  total += elapsed;
  if (VERBOSE) {
    printf("APPLY_SOBEL_FILTER EXEC TIME: %fs\n", elapsed);
  }

  printf("TOTAL EXEC TIME: %fs\n", total);

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
  int i, j;
  
  #pragma omp parallel for private(i, j) shared(image)
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
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
  int i, j, k, l;
  
  #pragma omp parallel for private(i, j, k, l, avg) shared(image) reduction(+ : rgb)
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      //#pragma
      for (k = -1; k < 2; k++) {
        for (l = -1; l < 2; l++) {
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

void apply_edge_gradient(int width, int height, Pixel image[height][width]) {

  Pixel updated[height][width];
  int color_coords[6];
  int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
  int Gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};
  int i, j, k, l;
  
  #pragma omp parallel for private(i,j,k,l) shared(image) reduction(+:color_coords)
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      for (k = -1; k < 2; k++) {
        for (l = -1; l < 2; l++) {
          if (i + k >= 0 && j + l >= 0 && i + k < height && j + l < width) {
            color_coords[0] += image[i + k][j + l].red * Gx[k + 1][l + 1]; // Red x
            color_coords[1] += image[i + k][j + l].red * Gy[k + 1][l + 1]; // Red y
            
            color_coords[2] += image[i + k][j + l].green * Gx[k + 1][l + 1]; // Green x
            color_coords[3] += image[i + k][j + l].green * Gy[k + 1][l + 1]; // Green y

            color_coords[4] += image[i + k][j + l].blue * Gx[k + 1][l + 1]; // Blue x
            color_coords[5] += image[i + k][j + l].blue * Gy[k + 1][l + 1]; // Blue y
          }
        }
      }
      
      int red = round(sqrt((pow(color_coords[0], 2) + pow(color_coords[1], 2))));
      red = red > 255 ? 255 : red;
      updated[i][j].red = red;
      
      int green = round(sqrt((pow(color_coords[2], 2) + pow(color_coords[3], 2))));
      green = green > 255 ? 255 : green;
      updated[i][j].green = green;
      
      int blue = round(sqrt((pow(color_coords[4], 2) + pow(color_coords[5], 2))));
      blue = blue > 255 ? 255 : blue;
      updated[i][j].blue = blue;

      for (int idx = 0; idx < 6; idx++) {
        color_coords[idx] = 0;
      }
    }
  }

  // Make image array equal updated array
  for (int a = 0; a < height; a++) {
    for (int b = 0; b < width; b++) {
      image[a][b] = updated[a][b];
    }
  }
}