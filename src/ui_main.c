#include "ui_main.h"
#include "lvgl.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define FAST_LIVO_FIFO "/tmp/fast_livo_cmd"

static lv_obj_t *status_label = NULL;

static void set_status(const char *text)
{
    if (status_label) {
        lv_label_set_text(status_label, text);
    }
}

static void send_fast_livo_cmd(const char *cmd)
{
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

static void start_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    send_fast_livo_cmd("start");
}

static void pause_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    send_fast_livo_cmd("pause");
}

static void resume_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    send_fast_livo_cmd("resume");
}

static void stop_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    send_fast_livo_cmd("stop");
}

static lv_obj_t *create_ctrl_btn(lv_obj_t *parent,
                                 const char *text,
                                 lv_event_cb_t cb,
                                 int x,
                                 int y)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 180, 70);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

void ui_main_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    lv_obj_clean(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "FAST-LIVO2 Demo Control");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    create_ctrl_btn(scr, "START",  start_btn_cb,  80,  120);
    create_ctrl_btn(scr, "PAUSE",  pause_btn_cb,  320, 120);
    create_ctrl_btn(scr, "RESUME", resume_btn_cb, 80,  240);
    create_ctrl_btn(scr, "STOP",   stop_btn_cb,   320, 240);

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -30);
}