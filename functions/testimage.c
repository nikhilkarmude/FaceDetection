#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>

#include "type-def.h"
#include "util.h"
#include "imageio.h"

int verbose;
extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char **argv)
{
  char fName[256], faceList[256], backList[256];
  int i, j, k;
  IMAGEINT animage, backimage;
  int k1, k2, k3, k4, k5;
  int i1, i2, j1, j2, face_diff;
  FILE *fp;
  char **face_image_list;
  int  nfaces;
  int nbacks, ntest;
  char **back_image_list;
  IMAGEINT *face_images;
  int matching_method;
  int total_comparisons, total_windows;
  char c;
  int trials;
  int max_trials;
  int seed;
  int occlusion_flag;
  int occlusion_width, occlusion_height;
  struct tms start_time, end_time;
  float ftmp;

  matching_method =  TEMPLATE_SMART_MATCHING; 
  animage.nrow = 0;
  backimage.nrow = 0;
  max_trials = 100;
  seed = 0;
  ntest = 10;
  verbose = 0;
  occlusion_flag = 0;
  
  strcpy(faceList,"orl-faces.lis");
  strcpy(backList, "background.lis");

  while ((c = getopt(argc, argv, "s:m:t:f:b:v:o:")) != EOF) {
    switch(c) {
    case 'm':
      matching_method = atoi(optarg);
      break;
    case 't':
      ntest = atoi(optarg);
      break;
    case 'f':
      strcpy(faceList, optarg);
      break;
    case 'b':
      strcpy(backList, optarg);
      break;
    case 'v':
      verbose = atoi(optarg);
      break;   
    case 'o':
      occlusion_flag = 1;
      sscanf(optarg,"%dx%d", &occlusion_height, &occlusion_width);
      break;
    case 's':
      seed = atoi(optarg);
      break;
    default:
      printf("%c is not valid option.\n", c);
    }
  }
  

  if (optind < argc) {
    strcpy(faceList, argv[optind]);
    optind ++;
  }

  if (optind < argc) {
    strcpy(backList, argv[optind]);
  }
  
  fp = fopen(faceList, "rb");
  if (fp == NULL) {
    printf("Cannot load the face list \"%s\" for reading.\n", faceList);
    printf("Exiting from file \"%s\" (line %d) in function \"%s\".\n",
	   __FILE__, __LINE__, __FUNCTION__);
    exit(-1);
  }
  k = 0;
  while(fscanf(fp,"%s", fName)==1) {
    if (Read_An_Image_Int(fName, &animage) != 0) {
      printf("Cannot read image \"%s\" and it is ignored.\n",
	     fName);
      continue;
    }
    k++;
  }
  nfaces = k;
  rewind(fp);
  face_image_list = (char **)malloc(sizeof(char *) * nfaces);
  k = 0;
  while(fscanf(fp,"%s", fName)==1) {
    if (Read_An_Image_Int(fName, &animage) == 0) {
      face_image_list[k] = (char *)malloc(sizeof(char) * (strlen(fName)+1));
      strcpy(face_image_list[k], fName);
      k++;
    }
  }
  fclose(fp);
  if (k != nfaces) {
    printf("Something is wrong. ");
    printf("These two numbers (%d and %d) must be the same.\n",
	   k, nfaces);
    printf("Exiting from file \"%s\" (line %d) in function \"%s\".\n",
	   __FILE__, __LINE__, __FUNCTION__);
    exit(-1);
  }
    
  printf("%d face images are loaded from the list given by \"%s\".\n", 
	 nfaces, faceList);

  fp = fopen(backList, "rb");
  if (fp == NULL) {
    printf("Cannot load the background list \"%s\" for reading.\n", backList);
    printf("Exiting from file \"%s\" (line %d) in function \"%s\".\n",
	   __FILE__, __LINE__, __FUNCTION__);
    exit(-1);
  }
  k = 0;
  while(fscanf(fp,"%s", fName)==1) {
    if (Read_An_Image_Int(fName, &animage) != 0) {
      printf("Cannot read image \"%s\" and it is ignored.\n",
	     fName);
      continue;
    }
    k++;
  }
  nbacks = k;
  rewind(fp);
  back_image_list = (char **)malloc(sizeof(char *) * nbacks);
  k = 0;

  while(fscanf(fp,"%s", fName)==1) {
    if (Read_An_Image_Int(fName, &animage) == 0) {
      back_image_list[k] = (char *)malloc(sizeof(char) * (strlen(fName)+1));
      strcpy(back_image_list[k], fName);
      k++;
    }
  }
  fclose(fp);
  if (k != nbacks) {
    printf("Something is wrong. ");
    printf("These two numbers (%d and %d) must be the same.\n",
	   k, nbacks);
    printf("Exiting from file \"%s\" (line %d) in function \"%s\".\n",
	   __FILE__, __LINE__, __FUNCTION__);
    exit(-1);
  }
    
  printf("%d back images are loaded from the list given by file \"%s\".\n", 
	 nbacks, backList);

  /* We need to load all the face images */
  
  face_images = (IMAGEINT *)malloc(sizeof(IMAGEINT) * nfaces);

  for (k1 = 0; k1 < nfaces; k1++) {
    face_images[k1].nrow = 0;
    if (Read_An_Image_Int(face_image_list[k1], &(face_images[k1])) !=0) {
      printf("Reading from image \"%s\" failed.\n",
	     face_image_list[k1]);
    }
  }

  printf("Matching method is ");
  switch(matching_method) {
  case TEMPLATE_SMART_MATCHING:
    printf("Fast template matching");
    break;
  case TEMPLATE_INTEGRAL_MATCHING:
    printf("Real-time based on Haar features");
    break;
  default:
    printf("Slow pixel-by-pixel template matching");
  }
  printf(".\n");
  
  k = 0;
  trials = 0;
  srand(seed);
  total_comparisons = 0;
  total_windows = 0;
  times(&start_time);
  while ((k < ntest) && (trials < max_trials) ) {
    do {
      k4 = rand() % nbacks;
      if (Read_An_Image_Int(back_image_list[k4], &backimage) !=0) {
	printf("Reading from image \"%s\" failed.\n",
	       back_image_list[k4]);
	trials ++;
      }
      else {
	printf("Load background image %d (%s) ... Done.\n",
	       k4, back_image_list[k4]);
	break;
      }
    } while( trials < max_trials );

    k3 = rand() % nfaces;
    printf("Loading face image %d (%s) ... Done.\n",
	   k3, face_image_list[k3]);
        
    if ( (backimage.nrow < face_images[k3].nrow) ||
	 (backimage.ncol < face_images[k3].ncol) ) {
      printf("Background image size (%dx%d) is too small).\n",
	     backimage.nrow, backimage.ncol);
      trials ++;
      continue;
    }
    if (occlusion_flag != 0) {
      Create_An_Image_Int(face_images[k3].nrow, face_images[k3].ncol,
			  &animage);
      Init_An_Image_Int(&animage, (int)1);
      if ( (occlusion_height >= face_images[k3].nrow) ||
	   (occlusion_width >= face_images[k3].ncol) ) {
	printf("Occlusion window size (%d x %d) not invalid and ",
	       occlusion_height, occlusion_width);
	printf("it is ignored.\n");
      }
      else {
	i = rand() % (face_images[k3].nrow - occlusion_height + 1);
	j = rand() % (face_images[k3].ncol - occlusion_width + 1);
	
	for (i1=0; i1 < occlusion_height; i1++) {
	  for (j1=0; j1 < occlusion_width; j1++) {
	    animage.data[i+i1][j+j1] = 0;
	  }
	}
	printf("Face is occluded by patch of size %d x %d at %d x %d.\n",
	       occlusion_height, occlusion_width,
	       i, j);
      }
    }

    i = rand() % (backimage.nrow-face_images[k3].nrow +1);
    j = rand() % (backimage.ncol-face_images[k3].ncol +1);
    for (k1=0; k1 < face_images[k3].nrow; k1++) {
      for (k2=0; k2 < face_images[k3].ncol; k2++) {
	if (occlusion_flag == 0) {
	  backimage.data[i+k1][j+k2] = face_images[k3].data[k1][k2];
	}
	else {
	  if (animage.data[k1][k2] != 0) {
	    backimage.data[i+k1][j+k2] = face_images[k3].data[k1][k2];
	  }
	}
      }
    }
    printf("%dth test image generated with true location ", k+1);
    printf("at %d x %d (rows x columns) from face %d and background %d.\n", 
	   i, j, k3, k4);

    switch(matching_method) {
    case TEMPLATE_SMART_MATCHING:
      for (i1 = 0; i1 <= (backimage.nrow-face_images[k3].nrow); i1++) {
        for (j1=0; j1 <= (backimage.ncol-face_images[k3].ncol); j1++) {
          for (k5=0; k5 < nfaces; k5++) {
            face_diff = 0;
            if (face_images[k5].nrow ==0) continue;
            for (i2=0; i2 < face_images[k5].nrow; i2++) {
              for (j2=0; j2 < face_images[k5].ncol; j2++) {
		if (backimage.data[i1+i2][j1+j2] != 
		    face_images[k5].data[i2][j2]) {
		  total_comparisons ++;
		  face_diff += 1;
		  break;
		}
		else {
		  total_comparisons++;
		}
              }
	      if (face_diff > 0) break;
            }
            if (face_diff == 0) {
              printf("Match found with face ");
	      printf("%d at location %d x %d (%d size %d %d).\n",
                     k5, i1, j1, face_diff, 
		     face_images[k5].nrow, face_images[k5].ncol);
            }
            else {
              /* */
              // printf("Difference is %d \n", face_diff);
            }
          }
        }
      }
      total_windows += (backimage.nrow-face_images[k3].nrow+1) *
	(backimage.ncol-face_images[k3].ncol+1);
      break;
    case TEMPLATE_INTEGRAL_MATCHING:
      break;
    default:      
      for (i1 = 0; i1 <= (backimage.nrow-face_images[k3].nrow); i1++) {
	if ( (i1 % (i1/50+1)) ==0) {
	  printf(".");
	  fflush(stdout);
	}
	for (j1=0; j1 <= (backimage.ncol-face_images[k3].ncol); j1++) {
	  for (k5=0; k5 < nfaces; k5++) {
	    face_diff = 0;
	    if (face_images[k5].nrow ==0) continue;
	    for (i2=0; i2 < face_images[k5].nrow; i2++) {
	      for (j2=0; j2 < face_images[k5].ncol; j2++) {
		face_diff += abs(backimage.data[i1+i2][j1+j2] -
				 face_images[k5].data[i2][j2]);
		total_comparisons++;
	      }
	    }
	    if (face_diff == 0) {
	      printf("Match found with face ");
	      printf("%d at location %d x %d (%d size %d %d).\n",
		     k5, i1, j1, face_diff, 
		     face_images[k5].nrow, face_images[k5].ncol);
	    }
	    else {
	      /* */
	      // printf("Difference is %d \n", face_diff);
	    }
	  }
	}
      }
      printf("Done.\n");
      total_windows += (backimage.nrow-face_images[k3].nrow+1) *
	(backimage.ncol-face_images[k3].ncol+1);
    }
    sprintf(fName,"background_%d.pgm", (int)(k+1));
    Write_An_Image_Int(&backimage, (int)1, fName);
    k++;
    trials = 0;
  }
  times(&end_time);
  
  printf("For %d test images, %d windows are compared with %d operations.\n",
	 k, total_windows, total_comparisons);
  if (total_windows > 0) {
    printf("Average comparison per window is %8.6f.\n",
	   (float)( (float)total_comparisons/
		    ((float)(total_windows))));
  }
  ftmp = (float)(end_time.tms_utime - start_time.tms_utime+
		 end_time.tms_stime - start_time.tms_stime)/
    (float)sysconf(_SC_CLK_TCK);
  printf("Running time in seconds: user: %6.2f system: %6.2f total: %6.2f.\n",
         (float)(end_time.tms_utime - start_time.tms_utime)/
	 (float)sysconf(_SC_CLK_TCK),
	 (float)(end_time.tms_stime - start_time.tms_stime)/
	 (float)sysconf(_SC_CLK_TCK),
	 ftmp);
  if (k > 0) {
    printf("For %d test images, it took %8.6f second(s), ",
	   k, ftmp);
    printf("which is %8.6f frame(s) per second.\n",
	   (float)(((float)k)/ftmp));
  }
  Free_Image_Int(&animage);
  for (k=0; k < nfaces; k++) {
    free(face_image_list[k]);
    Free_Image_Int(&(face_images[k]));
  }
  free(face_images);

  free(face_image_list);

  for (k=0; k < nbacks; k++) {
    free(back_image_list[k]);
  }
  free(back_image_list);

  

  return 0;
}













