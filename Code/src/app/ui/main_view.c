#include "src/display/lv_display.h"
#include "view_manager.h"

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    printf("code:%d\n", code);

    if(code == LV_EVENT_CLICKED) {
        printf("Clicked");
        lv_demo_music();
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        printf("Toggled");
    }
}

// 主界面初始化
void main_view_init(void) {

    lv_obj_t * img;

    img = lv_image_create(lv_screen_active());
    /* Assuming a File system is attached to letter 'A'
     * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
    lv_image_set_src(img, "A:./resources/images/background.png");
    lv_obj_align(img, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_move_background(img);
    lv_obj_move_to_index(img, 0);

    /*Create an image button*/
    lv_obj_t * imagebutton1 = lv_imagebutton_create(lv_screen_active());
    lv_imagebutton_set_src(imagebutton1, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "A:./resources/images/music_icon.png",
                           NULL);

    lv_obj_align(imagebutton1, LV_ALIGN_OUT_TOP_LEFT, 300, 0);
    lv_obj_add_event_cb(imagebutton1, event_handler, LV_EVENT_CLICKED, NULL);

    /*Create a label on the image button*/
    lv_obj_t * label = lv_label_create(imagebutton1);
    lv_label_set_text(label, "Music");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);  // 将文字放在图标下方居中位置

    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);  // 设置白色文字
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);   // 设置字体大小

    mkfifo("./pipe", 0777); // 创建命名管道文件
}





view_t main_view = {
    .name = "main_view",
    .init = main_view_init,
    .destroy = NULL,
    .event_cb = NULL,
};