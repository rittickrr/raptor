/*
 * Radboud Polarized Integrator
 * Copyright 2014-2021 Black Hole Cam (ERC Synergy Grant)
 * Authors: Thomas Bronzwaer, Jordy Davelaar, Monika Moscibrodzka, Ziri Younsi
 */

#include "functions.h"
#include "parameters.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void read_model(char *argv[]) {
    // model to read
    sscanf(argv[1], "%s", inputfile);

    FILE *input;
    input = fopen(inputfile, "r");
    if (input == NULL) {
        printf("Cannot read input file");
        // return 1;
    }

    char temp[100], temp2[100];

    //    read_in_table("symphony_pure_thermal.txt");

    // Model parameters
    fscanf(input, "%s %s %lf", temp, temp2, &MBH);
    fscanf(input, "%s %s %lf", temp, temp2, &M_UNIT);
    fscanf(input, "%s %s %lf", temp, temp2, &R_LOW);
    fscanf(input, "%s %s %lf", temp, temp2, &R_HIGH);
    fscanf(input, "%s %s %lf", temp, temp2, &INCLINATION);

    // Observer parameters
    fscanf(input, "%s %s %d", temp, temp2, &IMG_WIDTH);
    fscanf(input, "%s %s %d", temp, temp2, &IMG_HEIGHT);
    fscanf(input, "%s %s %lf", temp, temp2, &CAM_SIZE_X);
    fscanf(input, "%s %s %lf", temp, temp2, &CAM_SIZE_Y);

    fscanf(input, "%s %s %d", temp, temp2, &FREQS_PER_DEC);
    fscanf(input, "%s %s %lf", temp, temp2, &FREQ_MIN);
    fscanf(input, "%s %s %lf", temp, temp2, &STEPSIZE);
    fscanf(input, "%s %s %d", temp, temp2, &max_level);

    // Second argument: GRMHD file
    sscanf(argv[2], "%s", GRMHD_FILE);
    sscanf(argv[3], "%d", TIME_INIT);

    printf("Model parameters:\n");
    printf("MBH \t\t= %g \n", MBH);
    printf("M_UNIT \t\t= %g \n", M_UNIT);
    printf("R_LOW \t= %g \n", R_LOW);
    printf("R_HIGH \t= %g \n", R_HIGH);
    printf("INCLINATION \t= %g \n", INCLINATION);

    printf("Observer parameters:\n");
    printf("IMG_WIDTH \t= %d \n", IMG_WIDTH);
    printf("IMG_HEIGHT \t= %d \n", IMG_HEIGHT);
    printf("CAM_SIZE_X \t= %g \n", CAM_SIZE_X);
    printf("CAM_SIZE_Y \t= %g \n", CAM_SIZE_Y);
    printf("FREQS_PER_DEC \t= %d \n", FREQS_PER_DEC);
    printf("FREQ_MIN \t= %g \n", FREQ_MIN);

    printf("STEPSIZE \t= %g \n", STEPSIZE);
    fclose(input);
}

void calculate_image_block(struct Camera *intensityfield,
                           double energy_spectrum[num_frequencies],
                           double frequencies[num_frequencies]) {

    for (int pixel = 0; pixel < tot_pixels; pixel++) {
        for (int freq = 0; freq < num_frequencies; freq++) {
            for (int s = 0; s < 4; s++) {
                (*intensityfield).IQUV[pixel][freq][s] = 0;
            }
        }
    }
#pragma omp parallel for shared(energy_spectrum, frequencies, intensityfield,  \
                                p) schedule(static, 1)
    for (int pixel = 0; pixel < tot_pixels; pixel++) {
        int steps = 0;
        // For all pixel rows (distributed over threads)...

        double *lightpath2 = malloc(9 * max_steps * sizeof(double));

        double f_x = 0.;
        double f_y = 0.;
        double p = 0.;

        // INTEGRATE THIS PIXEL'S GEODESIC

        integrate_geodesic((*intensityfield).alpha[pixel],
                           (*intensityfield).beta[pixel], lightpath2, &steps,
                           CUTOFF_INNER);

        // PERFORM RADIATIVE TRANSFER AT DESIRED FREQUENCIES, STORE RESULTS
        for (int f = 0; f < num_frequencies; f++) {
            radiative_transfer_polarized(lightpath2, steps, frequencies[f],
                                         &f_x, &f_y, &p, 0,
                                         (*intensityfield).IQUV[pixel][f]);
        }
        free(lightpath2);
    }
#pragma omp barrier
}

void compute_spec(struct Camera *intensityfield,
                  double energy_spectrum[num_frequencies]) {
    double dA;
    for (int block = 0; block < tot_blocks; block++) {
        dA = (intensityfield)[block].dx[0] * (intensityfield)[block].dx[1];
        for (int pixel = 0; pixel < tot_pixels; pixel++) {
            for (int freq = 0; freq < num_frequencies; freq++) {
                energy_spectrum[freq] +=
                    (intensityfield)[block].IQUV[pixel][freq][0] * dA;
            }
        }
    }
}
