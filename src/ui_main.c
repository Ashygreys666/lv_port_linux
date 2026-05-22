#include "ui_main.h"
#include "lvgl.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#define FAST_LIVO_FIFO "/tmp/fast_livo_cmd"

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

/**********************************************************************************

                            用户点击 START
                            ↓
                        LVGL 触发 LV_EVENT_CLICKED
                            ↓
                        调用 btn_cb(e)
                            ↓
                        btn_cb 从 e 里拿到被点击按钮
                            ↓
                        通过 lv_obj_get_user_data(btn) 得到 BTN_START
                            ↓
                        进入 case BTN_START
                            ↓
                        send_fast_livo_cmd("start")
                            ↓
                        如果 FIFO 打开失败：
                                显示 NodeManager not ready
                                返回 false
                                不切换状态
                            ↓
                        如果 FIFO 写入成功：
                                返回 true
                                set_app_state(APP_STATE_RUNNING)
                                    ↓
                                    update_btn_by_app_state(APP_STATE_RUNNING)
                                        START 禁用
                                        PAUSE 启用
                                        RESUME 禁用
                                        STOP 启用
                                    ↓
                                    update_status_label_by_app_state(APP_STATE_RUNNING)
                                        status_label 显示 Running


********************************************************************************************************/


static lv_obj_t *start_btn = NULL;
static lv_obj_t *pause_btn = NULL;
static lv_obj_t *resume_btn = NULL;
static lv_obj_t *stop_btn = NULL;

//状态标签，用来显示当前状态
static lv_obj_t *status_label = NULL;

static void set_label_status(const char *text)
{
    if (status_label) {
        lv_label_set_text(status_label, text);
    }
}


//LVGL 里几乎所有东西都是 lv_obj_t *
//根据程序状态，统一刷新按钮状态,也就是根据系统状态，决定每个按钮是可点还是不可点，不要肤浅的只用可点和不可点

static void btn_set_state(lv_obj_t *btn, bool enable)
{
    if(!btn)return;
    
    if(enable){
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
    }
    else{
        lv_obj_add_state(btn, LV_STATE_DISABLED);
    }
}


static void update_btn_by_app_state(AppState state)
{
    switch(state){
        case APP_STATE_READY://停止和开始的按键使能状态是一样的
        case APP_STATE_STOPPED:
            btn_set_state(start_btn, true);
            btn_set_state(pause_btn, false);
            btn_set_state(resume_btn, false);
            btn_set_state(stop_btn, false);
        break;

        case APP_STATE_RUNNING:
            btn_set_state(start_btn, false);
            btn_set_state(pause_btn, true);
            btn_set_state(resume_btn, false);
            btn_set_state(stop_btn, true);
        break;

        case APP_STATE_PAUSED:
            btn_set_state(start_btn, false);
            btn_set_state(pause_btn, false);
            btn_set_state(resume_btn, true);
            btn_set_state(stop_btn, true);
        break;
    }
}

static void update_status_label_by_app_state(AppState state)
{
    switch (state) {
    case APP_STATE_READY:
        set_label_status("Ready");
        break;

    case APP_STATE_RUNNING:
        set_label_status("Running");
        break;

    case APP_STATE_PAUSED:
        set_label_status("Paused");
        break;

    case APP_STATE_STOPPED:
        set_label_status("Stopped");
        break;

    default:
        set_label_status("Unknown");
        break;
    }
}


static void set_app_state(AppState state)
{
    update_btn_by_app_state(state);
    update_status_label_by_app_state(state);
}



static bool send_fast_livo_cmd(const char *cmd)
{
    //只有先创建了管道文件，才能打开成功         O_NONBLOCK：不管有没有人来读管道
    int fd = open(FAST_LIVO_FIFO, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("open fifo failed: %s\n", strerror(errno));
        set_label_status("NodeManager not ready");
        return false;
    }

    write(fd, cmd, strlen(cmd));
    write(fd, "\n", 1);
    close(fd);

    printf("send fast-livo cmd: %s\n", cmd);

    return true;
}

static void btn_cb(lv_event_t *e)
{
    //获取是哪个按钮
    //在LVGL里面，只要获得按钮的指针，就能通过这个指针获取这个按钮的所有信息
    lv_obj_t *btn = lv_event_get_target(e);
    //uintptr_t 是和指针同宽的无符号整数类型
    //lv_obj_get_user_data(btn) 返回的是 void* 类型（指针)
    BtnType type = (BtnType)(uintptr_t)lv_obj_get_user_data(btn);
    switch(type) {
        case BTN_START:
            if (send_fast_livo_cmd("start")) {
                set_app_state(APP_STATE_RUNNING);
            }
        break;

        case BTN_PAUSE:
            if (send_fast_livo_cmd("pause")) {
                set_app_state(APP_STATE_PAUSED);
            }
        break;

        case BTN_RESUME:
            if (send_fast_livo_cmd("resume")) {
                set_app_state(APP_STATE_RUNNING);
            }
        break;

        case BTN_STOP:
            if (send_fast_livo_cmd("stop")) {
                set_app_state(APP_STATE_STOPPED);
            }
        break;
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
    //lv_obj_set_user_data(btn, (void *)type);   对比一下写法的差异
    lv_obj_set_user_data(btn, (void *)(uintptr_t)type);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}



void ui_main_create(void)
{
    lv_obj_t *scr = lv_screen_active();//获取当前活动的屏幕

    lv_obj_clean(scr);//清除屏幕上的旧控件

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "FAST-LIVO2 Demo Control");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);//align  设置排放位置  LV_ALIGN_TOP_MID  顶部中间
    
    start_btn = create_ctrl_btn(scr, "START",  btn_cb,  80,  120, BTN_START);
    pause_btn = create_ctrl_btn(scr, "PAUSE",  btn_cb,  320, 120, BTN_PAUSE);
    resume_btn = create_ctrl_btn(scr, "RESUME", btn_cb, 80,  240, BTN_RESUME);
    stop_btn = create_ctrl_btn(scr, "STOP",   btn_cb,   320, 240, BTN_STOP);

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -30);

    set_app_state(APP_STATE_READY);
}