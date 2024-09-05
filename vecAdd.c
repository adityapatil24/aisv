#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

/*
 * compile using GPU: gcc -o vecAdd vecAdd.c -lOpenCL -I"../openCL/src/OpenCL/include/" -L"../openCL/src/OpenCL/lib/"
 * compile using CPU: gcc -o vecAdd vecAdd.c -lOpenCL -I"../openCL/src/OpenCL/include/" -L"../openCL/src/OpenCL/lib/" -DUSE_CPU=1
 * compile using CPU + intel SIMD instructions: gcc -o vecAdd vecAdd.c -lOpenCL -I"../openCL/src/OpenCL/include/" -L"../openCL/src/OpenCL/lib/"  -DUSE_CPU=1 -O3 -ftree-vectorize -march=native
 */
#ifndef USE_CPU
    #define USE_CPU 0
#endif

#if USE_CPU
#include <time.h>
#endif
// OpenCL kernel to perform element-wise addition of two arrays
const char *kernelSource =
"__kernel void vecAdd(__global float *A, __global float *B, __global float *C) {\n"
"    int id = get_global_id(0);\n"
"    C[id] = A[id] + B[id];\n"
"}\n";

void vecAdd(float *A, float *B, float *C, int sizeOfArray) {
    for(int id = 0; id < sizeOfArray; id++)
    {
        C[id] = A[id] + B[id];
    }
}

void checkError(cl_int ret, const char* operation) {
    if (ret != CL_SUCCESS) {
        printf("Error during operation '%s': %d\n", operation, ret);
        exit(1);
    }
}

int main() {
    // Length of arrays
    int n = 100000000;

    // Allocate memory for arrays A, B, and C
    float *A = (float*)malloc(sizeof(float) * n);
    float *B = (float*)malloc(sizeof(float) * n);
    float *C = (float*)malloc(sizeof(float) * n);

    // Initialize arrays A and B with some values
    for (int i = 0; i < n; i++) {
        A[i] = i;
        B[i] = i * 2;
    }

    #if USE_CPU
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        vecAdd(A, B, C, n);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double cpu_time_used = (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) / 1e6;
        printf("CPU execution time: %f milliseconds\n", cpu_time_used);
    #else

        // Get platform and device information
        cl_platform_id platform_id[2] = NULL;
        cl_device_id device_id = NULL;
        cl_uint ret_num_devices;
        cl_uint ret_num_platforms;

        cl_int ret = clGetPlatformIDs(2, platform_id, &ret_num_platforms);
        checkError(ret, "clGetPlatformIDs");
        ret = clGetDeviceIDs(platform_id[1], CL_DEVICE_TYPE_ALL, 1, &device_id, &ret_num_devices);
        checkError(ret, "clGetDeviceIDs");

        char platform_name[128];
        ret = clGetPlatformInfo(platform_id[1], CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        if (ret == CL_SUCCESS) {
            printf("Platform %s\n",platform_name);
        } else {
            printf("Failed to get platform name for platform. Error: %d\n", ret);
        }

        char device_name[128];
        ret = clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
        checkError(ret, "clGetDeviceInfo");

        printf("  Device Name: %s\n", device_name);

        // Create an OpenCL context
        cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
        checkError(ret, "clCreateContext");

        // Create a command queue
        //cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
        cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &ret);
        checkError(ret, "clCreateCommandQueue");

        // Create memory buffers on the device for each vector
        cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, n * sizeof(float), NULL, &ret);
        checkError(ret, "clCreateBuffer A");
        cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, n * sizeof(float), NULL, &ret);
        checkError(ret, "clCreateBuffer B");
        cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, n * sizeof(float), NULL, &ret);
        checkError(ret, "clCreateBuffer C");

        // Copy the lists A and B to their respective memory buffers
        ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0, n * sizeof(float), A, 0, NULL, NULL);
        checkError(ret, "clEnqueueWriteBuffer A");
        ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, n * sizeof(float), B, 0, NULL, NULL);
        checkError(ret, "clEnqueueWriteBuffer B");

        // Create a program from the kernel source
        cl_program program = clCreateProgramWithSource(context, 1, (const char **)&kernelSource, NULL, &ret);
        checkError(ret, "clCreateProgramWithSource");

        // Build the program
        ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
        if (ret != CL_SUCCESS) {
            // Get the build log in case of errors
            size_t log_size;
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
            char *log = (char *)malloc(log_size);
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            printf("Build log:\n%s\n", log);
            free(log);
            checkError(ret, "clBuildProgram");
        }

        // Create the OpenCL kernel
        cl_kernel kernel = clCreateKernel(program, "vecAdd", &ret);
        checkError(ret, "clCreateKernel");

        // Set the arguments of the kernel
        ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
        checkError(ret, "clSetKernelArg 0");
        ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
        checkError(ret, "clSetKernelArg 1");
        ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);
        checkError(ret, "clSetKernelArg 2");

        // Execute the OpenCL kernel on the list
        size_t global_item_size = n; // Process the entire lists
        size_t local_item_size = 64; // Divide work items into groups of 64
        size_t max_work_group_size;
        ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
        checkError(ret, "clGetDeviceInfo");

        printf("Max work group size: %zu\n", max_work_group_size);
            // Ensure local_item_size does not exceed max_work_group_size
        if (local_item_size > max_work_group_size) {
            local_item_size = max_work_group_size;
        }

        // Ensure global_item_size is evenly divisible by local_item_size
        if (global_item_size % local_item_size != 0) {
            global_item_size = (global_item_size / local_item_size + 1) * local_item_size;
        }

        #if 0
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
        checkError(ret, "clEnqueueNDRangeKernel");

        // Read the memory buffer C on the device to the local variable C
        ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, n * sizeof(float), C, 0, NULL, NULL);
        checkError(ret, "clEnqueueReadBuffer");
        #endif

        cl_event event;
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, &event);
        checkError(ret, "clEnqueueNDRangeKernel");

        // Wait for the command queue to get serviced before reading back results
        clWaitForEvents(1, &event);

        // Read the memory buffer C on the device to the local variable C
        ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, n * sizeof(float), C, 0, NULL, NULL);
        checkError(ret, "clEnqueueReadBuffer");

        // Get the profiling information
        cl_ulong time_start, time_end;
        ret = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
        checkError(ret, "clGetEventProfilingInfo START");
        ret = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
        checkError(ret, "clGetEventProfilingInfo END");

        double nanoSeconds = time_end - time_start;
        printf("OpenCL Execution time is: %0.3f milliseconds \n", nanoSeconds / 1000000.0);

        // Clean up
        ret = clFlush(command_queue);
        checkError(ret, "clFlush");
        ret = clFinish(command_queue);
        checkError(ret, "clFinish");
        ret = clReleaseKernel(kernel);
        checkError(ret, "clReleaseKernel");
        ret = clReleaseProgram(program);
        checkError(ret, "clReleaseProgram");
        ret = clReleaseMemObject(a_mem_obj);
        checkError(ret, "clReleaseMemObject A");
        ret = clReleaseMemObject(b_mem_obj);
        checkError(ret, "clReleaseMemObject B");
        ret = clReleaseMemObject(c_mem_obj);
        checkError(ret, "clReleaseMemObject C");
        ret = clReleaseCommandQueue(command_queue);
        checkError(ret, "clReleaseCommandQueue");
        ret = clReleaseContext(context);
        checkError(ret, "clReleaseContext");

    #endif

    // Display the result to the screen
    // for (int i = 0; i < n; i++) {
    //     printf("%f + %f = %f\n", A[i], B[i], C[i]);
    // }

    free(A);
    free(B);
    free(C);
    return 0;
}
