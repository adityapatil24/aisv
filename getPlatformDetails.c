#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

int main() {
    cl_int ret;
    cl_uint num_platforms;

    // Get the number of platforms
    ret = clGetPlatformIDs(0, NULL, &num_platforms);
    if (ret != CL_SUCCESS) {
        printf("Failed to get number of platforms. Error: %d\n", ret);
        return 1;
    }

    // Allocate memory for the platform IDs
    cl_platform_id *platforms = (cl_platform_id *)malloc(num_platforms * sizeof(cl_platform_id));
    if (platforms == NULL) {
        printf("Failed to allocate memory for platform IDs.\n");
        return 1;
    }

    // Get the platform IDs
    ret = clGetPlatformIDs(num_platforms, platforms, NULL);
    if (ret != CL_SUCCESS) {
        printf("Failed to get platform IDs. Error: %d\n", ret);
        free(platforms);
        return 1;
    }

    // Print the platform IDs
    printf("Number of platforms: %u\n", num_platforms);
    for (cl_uint i = 0; i < num_platforms; i++) {
        char platform_name[128];
        char vendor_name[128];
        ret = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        if (ret == CL_SUCCESS) {
            printf("Platform %u: %s\n", i, platform_name);
        } else {
            printf("Failed to get platform name for platform %u. Error: %d\n", i, ret);
        }

        ret = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(vendor_name), vendor_name, NULL);
        if (ret == CL_SUCCESS) {
            printf("Platform %u: %s\n", i, vendor_name);
        } else {
            printf("Failed to get vendor name for platform %u. Error: %d\n", i, ret);
        }
    }

    // Clean up
    free(platforms);
    return 0;
}
