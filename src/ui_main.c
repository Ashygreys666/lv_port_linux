#include "ui_main.h"
#include "lvgl.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define FAST_LIVO_FIFO "/tmp/fast_livo_cmd"

//状态标签，用来显示当前状态
static lv_obj_t *status_label = NULL;

static void set_status(const char *text)
{
    if (status_label) {
        lv_label_set_text(status_label, text);
    }
}

static void send_fast_livo_cmd(const char *cmd)
{
    //只有先创建了管道文件，才能打开成功         O_NONBLOCK：不管有没有人来读管道
    int fd = open(FAST_LIVO_FIFO, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("open fifo failed: %s\n", strerror(errno));
        set_status("NodeManager not ready");
        return;
    }

    write(fd, cmd, strlen(cmd));
    write(fd, "\n", 1);
    close(fd);

    printf("send fast-livo cmd: %s\n", cmd);

    //strcmp：对比两个字符串是不是一模一样
    if (strcmp(cmd, "start") == 0) {
        set_status("Sent: START");
    } else if (strcmp(cmd, "pause") == 0) {
        set_status("Sent: PAUSE");
    } else if (strcmp(cmd, "resume") == 0) {
        set_status("Sent: RESUME");
    } else if (strcmp(cmd, "stop") == 0) {
        set_status("Sent: STOP");
    }
}

/*******************************************************
    用户点击 START 按钮
            ↓
    LVGL 检测到 LV_EVENT_CLICKED
            ↓
    LVGL 调用 start_btn_cb(e)
            ↓
    start_btn_cb 调用 send_fast_livo_cmd("start")
            ↓
    程序往 FIFO 里写入 start 命令
********************************************************/


//cb  事件回调函数      e是事件信息，他里面包含了是谁触发了事件，是什么事件，有没有用户数据，当前状态是什么
//比如你可以从 e 里拿到被点击的按钮：lv_obj_t *btn = lv_event_get_target(e);
//也可以拿到用户数据：void *user_data = lv_event_get_user_data(e);

/*
    e 里面常用能取到这几类东西：
    lv_event_get_target(e);     // 谁触发了这个事件，比如哪个按钮
    lv_event_get_code(e);       // 触发了什么事件，比如点击、按下、释放
    lv_event_get_user_data(e);  // 绑定事件时传进来的自定义数据

    例子：
    static void start_btn_cb(lv_event_t *e)
    {
        lv_obj_t *btn = lv_event_get_target(e);

        send_fast_livo_cmd("start");

        lv_obj_add_state(btn, LV_STATE_DISABLED);
    }

    用户点击 START 按钮
    ↓
    进入 start_btn_cb
    ↓
    从 e 里拿到被点击的按钮 btn
    ↓
    发送 start 命令
    ↓
    把这个按钮禁用

*/


// static void start_btn_cb(lv_event_t *e)
// {
//     LV_UNUSED(e);//不过这个函数里的逻辑暂时用不到 e，所以写了：LV_UNUSED(e);  因为每个按键都对应一种状态，能拿到信息是定死的
//     send_fast_livo_cmd("start");
// }

// static void pause_btn_cb(lv_event_t *e)
// {
//     LV_UNUSED(e);
//     send_fast_livo_cmd("pause");
// }

// static void resume_btn_cb(lv_event_t *e)
// {
//     LV_UNUSED(e);
//     send_fast_livo_cmd("resume");
// }

// static void stop_btn_cb(lv_event_t *e)
// {
//     LV_UNUSED(e);
//     send_fast_livo_cmd("stop");
// }


static void btn_cb(lv_event_t *e)
{
    //获取是哪个按钮
    //在LVGL里面，只要获得按钮的指针，就能通过这个指针获取这个按钮的所有信息
    lv_obj_t *btn = lv_event_get_target(e);
    //uintptr_t 是和指针同宽的无符号整数类型
    //lv_obj_get_user_data(btn) 返回的是 void* 类型（指针)
    BtnType type = (BtnType)(uintptr_t)lv_obj_get_user_data(btn);
    switch(type) {
        case BTN_START:  send_fast_livo_cmd("start"); break;
        case BTN_PAUSE:  send_fast_livo_cmd("pause"); break;
        case BTN_RESUME: send_fast_livo_cmd("resume"); break;
        case BTN_STOP:   send_fast_livo_cmd("stop"); break;
    }
}



static lv_obj_t *create_ctrl_btn(lv_obj_t *parent,
                                 const char *text,
                                 lv_event_cb_t cb,
                                 int x,
                                 int y,
                                 BtnType type)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 180, 70);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn, (void *)type);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

//LVGL 里几乎所有东西都是 lv_obj_t *

void ui_main_create(void)
{
    lv_obj_t *scr = lv_screen_active();//获取当前活动的屏幕

    lv_obj_clean(scr);//清除屏幕上的旧控件

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "FAST-LIVO2 Demo Control");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);//align  设置排放位置  LV_ALIGN_TOP_MID  顶部中间
    
    create_ctrl_btn(scr, "START",  btn_cb,  80,  120, BTN_START);
    create_ctrl_btn(scr, "PAUSE",  btn_cb,  320, 120, BTN_PAUSE);
    create_ctrl_btn(scr, "RESUME", btn_cb, 80,  240, BTN_RESUME);
    create_ctrl_btn(scr, "STOP",   btn_cb,   320, 240, BTN_STOP);

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -30);
}