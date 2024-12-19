#include <stdio.h>
#include <stdlib.h>

void extractH264FromMP4(const char * inputFile, const char * outputFile)
{
    char command[512];

    // 创建 FFmpeg 命令来提取视频流
    snprintf(command, sizeof(command), "ffmpeg -i %s -c:v copy -an -f h264 -y %s", inputFile, outputFile);

    // 执行命令
    int result = system(command);
    if (result == 0)
    {
        printf("H.264 extraction successful: %s\n", outputFile);
    }
    else
    {
        printf("Error extracting H.264 data.\n");
    }
}

int main()
{
    const char * inputFile  = "./vedio/test4.mp4";
    const char * outputFile = "./vedio/test4.264";

    // 提取 H.264 流
    extractH264FromMP4(inputFile, outputFile);

    return 0;
}
