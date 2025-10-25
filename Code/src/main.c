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
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>


#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

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

    lv_obj_t * label1 = lv_label_create(lv_screen_active());
    lv_label_set_long_mode(label1, LV_LABEL_LONG_MODE_WRAP);     /*Break the long lines*/
    lv_label_set_recolor(label1, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_text(label1, "#0000ff Re-color# #ff00ff words# #ff0000 of a# label, align the lines to the center "
                      "and wrap long text automatically.");
    lv_obj_set_width(label1, 150);  /*Set smaller width to make the lines wrap*/
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t * label2 = lv_label_create(lv_screen_active());
    lv_label_set_long_mode(label2, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);     /*Circular scroll*/
    lv_obj_set_width(label2, 150);
    lv_label_set_text(label2, "It is a circularly scrolling text. ");
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, 40);

    // 注意这里的管道文件路径要改为你创建的管道路径，也就说在这之间要先用mkfifo创建一个管道出来
//    sprintf(command_name,"mplayer -slave -quiet -ao alsa:device=hw=0,0 -input file=/mywork/lvgl_3568/lvgl_iPads/pipe /mywork/lvgl_3568/lvgl_iPads/%s",music_name);

    mkfifo("./pipe", 0777);
//    pthread_t tid1, tid2;
//    pthread_create(&tid1, NULL, music_play_task, NULL); // 创建播放线程 
//    pthread_create(&tid2, NULL, print_test, NULL); // 创建播放线程 
    /*Create a Demo*/
//    lv_demo_widgets();
//    lv_demo_widgets_start_slideshow();
    lv_demo_music();
//	system("aplay -v --format=cd --device=plughw:0,0 module/audio/music_zhou.wav");
//    const char * lv_demo_music_get_title(uint32_t track_id);
//    const char * lv_demo_music_get_artist(uint32_t track_id);
//    const char * lv_demo_music_get_genre(uint32_t track_id);
//    uint32_t lv_demo_music_get_track_length(uint32_t track_id);

    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    return 0;
}
