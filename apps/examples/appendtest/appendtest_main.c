/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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
 * examples/hello/hello_main.c
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
#include<fcntl.h>
#include<unistd.h>
#include<time.h>
#include<stdio.h>
/****************************************************************************
 * hello_main
 ****************************************************************************/
char buf[240];
#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int appendtest_main(int argc, char *argv[])
#endif
{
	time_t start,end;
    for ( int sz = 60; sz < 240; sz *= 2)
    {
        for (int  i = 0; i < sz; i++){
            buf[i] =  'N';
        }
        double total_time = 0;
       
        for (int i=0 ; i < 20; i++ ) 
        {
            start = clock();
            for (int f=0;f < 10;f++)   // for 10 files
            {
                char filename[20];
                snprintf(filename, 20, "/mnt/file_%d.txt", f);
                int fd = open(filename, O_CREAT | O_WRONLY);
                if (fd < 0)
                {
                    printf("can't Open File, filename: %s, ret: %d", filename, fd);
                    continue;
                }
                // printf("%d",fd);
                write(fd, buf, sz);
                close(fd);
                for (int i = 0; i < 199; i++)
                {
                    fd = open(filename,O_WRONLY);
                    if (fd<0)
                    {
                        printf("can't Open File: %d", fd);
                        continue;
                    }
                    // printf("%d\n",fd);
                    lseek(fd, 0, SEEK_END);
                    write(fd, buf, sz);
                    close(fd);
                }
            }
            end = clock();
            // printf("%ld %ld\n",end, start);
            double time = ( (double)end - start )  / CLOCKS_PER_SEC;
            total_time += time;
            printf("Time for iteration %d : %lf\n",i,time);
            for (int f = 0; f < 10; f++) { 
                char filename[20];
                snprintf(filename, 20, "/mnt/file_%d.txt", f);
                unlink(filename);
            }
        }
        printf("Total Time for %d bytes buffer: %lf\n",sz,total_time);
    }
    return 0;
}
