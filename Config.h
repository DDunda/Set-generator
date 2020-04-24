#pragma once

// Minimum time in milliseconds between frames
#define MIN_DELTA 1000.0L/60.0L
// Size of the window
#define WIDTH 1920
#define HEIGHT 1080

#define NVIDEA_ACCELERATION

//#define CAMERA_OFFSET_X -0.5
#define CAMERA_OFFSET_X -0.75L
#define CAMERA_OFFSET_Y 0.0L
#define CAMERA_RADIUS 2.0L

#define SAMPLING
#define SAMPLE_OFFSET_X -0.75L
#define SAMPLE_OFFSET_Y 0.0L
#define SAMPLE_RADIUS_X 1.0L
#define SAMPLE_RADIUS_Y 2L

#define Y_RATIO_CORRECTION (long double)HEIGHT/(long double)WIDTH

#define SAMPLE_MULT 10

#define MAX_PATH_LENGTH 10000

#define MAX_ESCAPES 4

#define NORMAL 0
#define LOGARITHMIC 1
#define SQRT 2
#define SQR 3
#define SCALING_TYPE NORMAL

#define MANDELBROT 0
#define FULL_BUDDHA 1
#define ESCAPING_BUDDHA 2
#define JULIA 3
#define PLOT_TYPE ESCAPING_BUDDHA

#define GREYSCALE 0
#define GREEN 1
#define BANDED 2
#define GOLD 3
#define COLOUR_TYPE GREYSCALE

#define SQR_ESCAPE_RADIUS 9

#define JULIA_X 0.3L
#define JULIA_Y 0.5L