/****************************************************************************
 *
 * Copyright 2023 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * examples/appendtest/appendtest_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <tinyara/timer.h>
#include <sys/statfs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
// #include <redposix.h>

/* File system includes. */

#define ROOT                            "/mnt/"
#define SAMPLE_FILE                     ROOT "sample_file"
#define TEST_DIR_SINGLE                 ROOT "single"
#define TEST_FILE_SINGLE                ROOT "single/test_file"
#define TEST_DIR_MULTIPLE               ROOT "multiple"
#define TEST_FILE_MULTIPLE              ROOT "multiple/test_file_"
#define TEST_FILE_NAME_LEN_MAX          42
#define TEST_ITR                        10
#define TIMER_DEVICE 			"/dev/timer0"
#define FS_PART_SIZE			512

int frt_fd = -1;
/****************************************************************************
 * appendtest_main
 ****************************************************************************/
char buf[241];
int init_sample_file(int buffer_length)
{
	int fd;
        int ret;
        int i;
        char sample_buf[] = "Writing data into sample file";

        fd = open(SAMPLE_FILE, O_CREAT| O_RDWR);
        if (fd < 0) {
                printf("Unable to open sample file %s, fd = %d\n", SAMPLE_FILE, fd);
                return -ENOMEM;
        }

        for (i = 0; i < (buffer_length / 32) + 1; i++) {
                ret = write(fd, sample_buf, 32);
                if (ret != 32) {
                        printf("Unable to write to sample file, fd = %d\n", ret);
                        close(fd);
                        return -1;
                }
        }

        close(fd);
	printf("Sample File Initialization Complete\n\n");
	return OK;
}
int create_PPE(const char *dir_path)
{
        int ret;
        int fd1, fd2, i;
        char *sample_buf;

        ret = mkdir(dir_path, 0777);
        if (ret < 0) {
                printf("Unable to create test directory, ret:  %d\n", ret);
                goto errout;
        }
        sample_buf = (char *)malloc(2049); // buffer size 2 KB
        

        /* Fill up the partition with garbage, Create and Delete files
         * Creating 64 KiB files at a time and deleting them to create garbage
         * However, we do not want the garbage to be collected before the test itself starts
         */
        for (i = 0; i < ((FS_PART_SIZE - 128) / 64); i++) {
                fd1 = open(SAMPLE_FILE, O_RDONLY);
                if (fd1 < 0) {
                        printf("Unable to open sample file ro reading\n");
                        return -EIO;
                }
                fd2 = open(TEST_FILE_SINGLE, O_CREAT | O_WRONLY);
                if (fd2 < 0) {
                        printf("Failed to open garbage file for creation\n");
                        goto errout;
                }
                printf("Writing Garbage file #%d\n", i);
                for( int j = 0; j < 32; j++) {
                        ret = read(fd1, sample_buf, 2048);
                        if (ret != 2048) {
                                printf("Unable to read from Sample File\n");
                                goto errout;
                        }
                        ret = write(fd2, sample_buf, 2048);
                        if (ret != 2048) {
                                printf("Unable to write to garbage file\n");
                                goto errout;
                        }
                }

                close(fd1);
                close(fd2);
                unlink(TEST_FILE_SINGLE);
        }

        free(sample_buf);
        printf("PPE Exits\n");
            return OK;
    errout:
        free(sample_buf);
        printf("RRE Exits\n");
            return -EIO;
}

int create_max_file(char *file_name, long int size)
{
	int ret, fd;
        char sample_buf[] = "Writing data to the sample file";

        fd = open(file_name, O_CREAT | O_WRONLY);
        if (fd < 0) {
                printf("Unable to open sample file: %s, fd = %d\n", file_name, fd);
                return -ENOMEM;
        }

        while (size >= 31) {
                ret = write(fd, sample_buf, 31);
                if (ret != 31) {
                        printf("Unable to write to file %s, fd = %d\n", file_name, ret);
                        close(fd);
                        return -ENOMEM;
                }
                size -= 31;
        }
        if (size) {
                ret = write(fd, sample_buf, size);
                if (ret != size) {
                        printf("Unable to write to file %s, fd = %d\n", file_name, ret);
                        close(fd);
                        return -ENOMEM;
                }
                size = 0;
        }

        close(fd);
        return OK;
}
int readBackVerify(char *multi_file_name, char *sample_buf, int sz)
{
        // Readback and verification
        #ifdef CONFIG_EXAMPLES_APPENDTEST_READBACKVERIFY
        // printf("Performing Readbackverify...");
        int fd, ret;
        char *read_buf = (char *) malloc(sz + 1);
        fd = open(multi_file_name, O_RDONLY);
        if(fd < 0) {
                printf("can't Open File, filename: %s, ret: %d\n", multi_file_name, fd);
                goto errout;
        }
        ret = read(fd, read_buf, sz);
        if (ret != sz) {
                printf("Unable to read to test file %s\n", multi_file_name);
                goto errout;
        }
        if( strncmp(sample_buf, read_buf, sz) != 0) {
                printf("Readback verification Failed, filename %s, ret: %d\n", multi_file_name, fd);
                goto errout;
        }
        close(fd); 
        free(read_buf); 
        printf("=======================================\n");
        printf("Buffer compared to written data: OK\n");  
        printf("=======================================\n");
        return OK;
        errout: 
                free(read_buf);
                printf("Test Failed, error: %d\n", errno);
                return errno;
        #endif
}
int verifymanually(char *multi_file_name, int sz)
{
        #ifdef CONFIG_EXAMPLES_APPENDTEST_READBACKVERIFY
        // printing the file to verify content manually (buffer written 200 times or not)
        int fd, ret, l;
        char *read_buf = (char *)malloc(sz * 199 + 1);
        printf("=======================================\n");
        printf("verifing Manually\n");
        printf("=======================================\n");

        fd = open(multi_file_name, O_RDONLY);
        if(fd < 0) {
                printf("can't Open File, filename: %s, ret: %d\n", multi_file_name, fd);
                goto errout;
        }
        lseek(fd, sz, SEEK_SET);
        ret = read(fd, read_buf, sz * 199);
        if (ret != (sz * 199)) {
                printf("Unable to read to test file %s\n", multi_file_name);
                goto errout;
        }
        for ( l = 0; l < (sz * 199); l++) {
                printf("%c", read_buf[l]);
                if (((l + 1) % 180) == 0) {
                        printf("\n");
                }
        }
        printf("=======================================\n");
        free(read_buf);
        close(fd);
        return OK;
        errout: 
                free(read_buf);
                printf("Test Failed, error: %d\n", errno);
                return errno;
        #endif
}
int appendtest(void)
{   
        int fd, ret, i, sz,f, itr;
        char *sample_buf;
        time_t start, end;

        ret = create_PPE(TEST_DIR_SINGLE);
        // char *tmp_buf = "This is data in the temporary buffer. This will be written to the file for testing Filesystems with TizenRT RTOS.\n";
        char multi_file_name[TEST_FILE_NAME_LEN_MAX];
        if (ret != OK) {
                printf("Unable to create PPE\n");
                goto errout;
        }

	printf("PPE Complete\n");

        fd = open(SAMPLE_FILE, O_RDOK);
        if (fd < 0) {
                printf("Failed to open sample file for reading\n");
                return - EIO;
        }
	printf("Sample File opened\n");
        sample_buf = (char *)malloc(1025);

        ret = read(fd, sample_buf, 1024);
        if (ret != 1024) {
                printf("Unable to read from sample file\n");
                goto errout;
        }
	printf("Read 1 buf from sample file\n");

        close(fd);

       for (i = 0; i < 4; i++) {
               snprintf(multi_file_name, TEST_FILE_NAME_LEN_MAX, "%s%s%d", TEST_FILE_SINGLE, "_", i);
               ret = create_max_file(multi_file_name, 64 * 1024);
               if (ret != OK) {
                       printf("Unable to create dummy file %d\n", i);
                       return -1;
               }
       }

	ret = mkdir(TEST_DIR_MULTIPLE, 0777);
        if (ret < 0) {
                printf("Unable to create main test directory\n");
                goto errout;
        }
        
        for (sz = 60; sz <= 240; sz *= 2)
        {
                for ( i = 0; i < sz; i++){
                        buf[i] =  'N';
                }
                double total_time = 0;
        
                for (i = 0 ; i < 20; i++ ) 
                {
                        start = clock();
                        int n = 10;               //-> For file size < 200
                        if (sz == 240) {
                                n = 8;
                        }
                        for (f = 0; f < n; f++)   // for n files
                        {
                                snprintf(multi_file_name, TEST_FILE_NAME_LEN_MAX, "%s%s%d", TEST_FILE_MULTIPLE, "_", f);
                                fd = open(multi_file_name, O_CREAT | O_WRONLY);
                                if (fd < 0)
                                {
                                        printf("can't Open File, filename: %s, ret: %d\n", multi_file_name, fd);
                                        perror("error in 199 times:");
                                        goto errout;
                                }
                                ret = write(fd, sample_buf, sz);
                                if (ret != sz) {
                                        printf("Unable to write to test file %s\n", multi_file_name);
                                        goto errout;
                                }
                                close(fd);

                                ret = readBackVerify(multi_file_name, sample_buf, sz);
                                if(ret < 0) {
                                        printf("Read back verification Failed!, ret: %d\n", ret);
                                        goto errout;
                                }

                                for ( itr = 0; itr < 199; itr++)
                                {
                                        fd = open(multi_file_name, O_WRONLY);
                                        if (fd < 0)
                                        {
                                                printf("can't Open File, filename: %s, ret: %d\n", multi_file_name, fd);
                                                perror("error in 199 times:");
                                                goto errout;

                                        }
                                        lseek(fd, 0, SEEK_END);
                                        ret = write(fd, sample_buf, sz);
                                                
                                        if (ret != sz) {
                                                printf("Unable to write to test file %s, ret: %d\n", multi_file_name, fd);
                                                goto errout;
                                        }
                                        close(fd);
                                }
                                ret = verifymanually(multi_file_name, sz);
                                if(ret < 0) {
                                        printf("Read back verification Failed!, ret: %d\n", ret);
                                        goto errout;
                                }
                        }
                        end = clock();
                        double time = ( (double)end - start )  / CLOCKS_PER_SEC;
                        total_time += time;
                        printf("Time for iteration %d : %lf\n", i, time);
                        for (f = 0; f < n; f++) { 
                                snprintf(multi_file_name, TEST_FILE_NAME_LEN_MAX, "%s%s%d", TEST_FILE_MULTIPLE, "_", f);
                                ret = unlink(multi_file_name);
                                if(ret < 0)
                                {
                                        printf("Can't able to unlink file: %s, ret: %d\n", multi_file_name, ret);
                                        goto errout;
                                }
                        }
                }
                printf("Total Time for %d bytes buffer: %lf\n",sz,total_time);
        }
        free(sample_buf);
        return OK;
        errout:
                free(sample_buf);
                return -EIO;

}


#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int appendtest_main(int argc, char *argv[])
#endif
{
        int ret;
        ret = init_sample_file(65536);
        if (ret != OK) {
                printf("Unable to init sample test file, ret = %d\n", ret);
                goto errout;
        }
        ret = appendtest();
        if (ret != OK) { 
                printf("Append Test failed, ret = %d\n", ret);
                goto errout;
        }
        printf("FS Performance Test Completed successfully\n");
        ret = OK;

        errout:
        printf("Test Failed");
        return 0;
}   
