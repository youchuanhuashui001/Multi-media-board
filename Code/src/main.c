/*******************************************************************
 *
 * main.c - LVGL simulator for GNU/Linux
 *
 * Based on the original file from the repository
 *
 * @note eventually this file won't contain a main function and will
 * become a library supporting all major operating systems
 *
 * To see how each driver is initialized check the
 * 'src/lib/display_backends' directory
 *
 * - Clean up
 * - Support for multiple backends at once
 *   2025 EDGEMTech Ltd.
 *
 * Author: EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
 *
 ******************************************************************/
#include "common.h"
#include "view_manager.h"

//#include <unistd.h>
//#include <pthread.h>
//
//
//#include <sys/types.h>
//#include <sys/stat.h>
//
//
//#include "lvgl/lvgl.h"
//#include "lvgl/demos/lv_demos.h"
//
//#include "src/lib/driver_backends.h"
//#include "src/lib/simulator_util.h"
//#include "src/lib/simulator_settings.h"

/* Internal functions */
static void configure_simulator(int argc, char **argv);
static void print_lvgl_version(void);
static void print_usage(void);

/* contains the name of the selected backend if user
 * has specified one on the command line */
static char *selected_backend;


static void *music_play_task(void *arg)
{
    char command_name[512]={0};
    char *music_name = "file_example_WAV_2MG.wav";
    sprintf(command_name,"./mplayer -slave -quiet -ao alsa:device=hw=0,0 -input file=./pipe %s",music_name);

    FILE *fp = popen(command_name, "r");
    if(fp == NULL)
    {
        perror("加载命令失败\n");
        return NULL;
    }


    while(1)
    {
        char filecontent[1024] = {0};
        char *ret = fgets(filecontent, 1024, fp);
        if(ret == NULL)
        {
            printf("mp3读取完毕\n");
            break;
        }
        
        // 去除换行符
        filecontent[strcspn(filecontent, "\n")] = 0;
        
        // 解析不同类型的响应
//        parse_mplayer_response(filecontent);
        
        printf("收到: %s\n", filecontent);
        usleep(100000);

    }

    pclose(fp);


    return NULL;
}

static void *print_test(void *arg)
{
    while (1) {
        printf("hello lvgl\n");
    }
    return NULL;
}

/**
 * @brief entry point
 * @description start a demo
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
int main(int argc, char **argv)
{
    driver_backends_register();

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend */
    if (driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    /* Enable for EVDEV support */
#if LV_USE_EVDEV
    if (driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }
#endif

    // 关闭光标
    system("echo -e \"\\033[?25l\" > /dev/tty1");
    // 屏幕不再熄灭
    system("echo -e  \"\\033[9;0]\"  > /dev/tty0");
    // inputdev 环境变量
    system("export LV_LINUX_EVDEV_POINTER_DEVICE=/dev/input/event1");

    view_manager_init();

    // 谁最先注册，谁就最先显示
    view_manager_register(&main_view);
    view_manager_register(&music_view);
    view_manager_register(&book_view);


    // 注意这里的管道文件路径要改为你创建的管道路径，也就说在这之间要先用mkfifo创建一个管道出来
//    sprintf(command_name,"mplayer -slave -quiet -ao alsa:device=hw=0,0 -input file=/mywork/lvgl_3568/lvgl_iPads/pipe /mywork/lvgl_3568/lvgl_iPads/%s",music_name);

//    pthread_t tid1, tid2;
//    pthread_create(&tid1, NULL, music_play_task, NULL); // 创建播放线程 
//    pthread_create(&tid2, NULL, print_test, NULL); // 创建播放线程 
    /*Create a Demo*/
//    lv_demo_widgets();
//    lv_demo_widgets_start_slideshow();
//	system("aplay -v --format=cd --device=plughw:0,0 module/audio/music_zhou.wav");
//    const char * lv_demo_music_get_title(uint32_t track_id);
//    const char * lv_demo_music_get_artist(uint32_t track_id);
//    const char * lv_demo_music_get_genre(uint32_t track_id);
//    uint32_t lv_demo_music_get_track_length(uint32_t track_id);

    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    return 0;
}
