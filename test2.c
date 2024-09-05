#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

void checkError(cl_int ret, const char* operation) {
    if (ret != CL_SUCCESS) {
        printf("Error during operation '%s': %d\n", operation, ret);
        exit(1);
    }
}

int main() {
    cl_int ret;
    cl_uint num_platforms;
    cl_platform_id *platforms;

    // Get the number of platforms
    ret = clGetPlatformIDs(0, NULL, &num_platforms);
    checkError(ret, "clGetPlatformIDs");

    // Allocate memory for platforms
    platforms = (cl_platform_id*)malloc(num_platforms * sizeof(cl_platform_id));

    // Get the platforms
    ret = clGetPlatformIDs(num_platforms, platforms, NULL);
    checkError(ret, "clGetPlatformIDs");

    // Iterate over each platform
    for (cl_uint i = 0; i < num_platforms; i++) {
        cl_uint num_devices;
        cl_device_id *devices;

        // Get the number of devices for the platform
        ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
        checkError(ret, "clGetDeviceIDs");

        // Allocate memory for devices
        devices = (cl_device_id*)malloc(num_devices * sizeof(cl_device_id));

        // Get the devices
        ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
        checkError(ret, "clGetDeviceIDs");

        // Iterate over each device
        for (cl_uint j = 0; j < num_devices; j++) {
            char device_name[128];
            cl_ulong global_mem_size;
            cl_ulong max_mem_alloc_size;
            cl_uint mem_base_addr_align;

            // Get device name
            ret = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
            checkError(ret, "clGetDeviceInfo");

            // Get global memory size
            ret = clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);
            checkError(ret, "clGetDeviceInfo");

            // Get maximum memory allocation size
            ret = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(max_mem_alloc_size), &max_mem_alloc_size, NULL);
            checkError(ret, "clGetDeviceInfo");

            // Get memory base address alignment
            ret = clGetDeviceInfo(devices[j], CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(mem_base_addr_align), &mem_base_addr_align, NULL);
            checkError(ret, "clGetDeviceInfo");

            printf("Platform %u, Device %u:\n", i, j);
            printf("  Device Name: %s\n", device_name);
            printf("  Global Memory Size: %llu MB\n", global_mem_size / (1024 * 1024));
            printf("  Max Memory Allocation Size: %llu MB\n", max_mem_alloc_size / (1024 * 1024));
            printf("  Memory Base Address Alignment: %u bits\n", mem_base_addr_align);
        }

        // Free device memory
        free(devices);
    }

    // Free platform memory
    free(platforms);

    return 0;
}
