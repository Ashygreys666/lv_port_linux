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

/*    

        涉及设置ui  style的函数
        lv_style_init() ：lv_style_init(&style_start_btn) 把这个 style 清空并初始化，准备往里面设置颜色、圆角等属性
        lv_style_set_bg_color()
        lv_style_set_text_color()
        lv_style_set_radius() 设置圆角  lv_style_set_radius(&style_start_btn, 0);这个是直角
        lv_style_set_border_width() 设置边框宽度      lv_style_set_border_width(&style_start_btn, 0); 这个是不要边框
        lv_style_set_border_color(&style_start_btn, lv_color_white());设置边框颜色
        lv_style_set_opa()  lv_style_set_opa(&style_start_btn, LV_OPA_80);设置整体透明度  lv_style_set_opa(&style_disabled, LV_OPA_40); LV_OPA_COVER   // 完全不透明 LV_OPA_TRANSP  // 完全透明
        禁用按钮的时候可以让他变淡

        lv_obj_add_style()  lv_obj_add_style(start_btn, &style_start_btn, LV_PART_MAIN); LV_PART_MAIN 表示控件主体  把 style_start_btn 这个样式，应用到 start_btn 的主体部分

*/
/*
                Flex类的  Flex 就是给父容器设置自动排版规则，子控件只管创建和设置大小，不再自己写 x/y 坐标
            lv_obj_create()              // 创建容器
            lv_obj_set_size()            // 设置容器/按钮大小
            lv_obj_align()               // 设置容器位置
            lv_obj_set_flex_flow()       // 设置 Flex 排列方向
            lv_obj_set_flex_align()      // 设置 Flex 对齐方式
            lv_obj_set_style_pad_row()   // 设置行间距
            lv_obj_set_style_pad_column()// 设置列间距

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

/*按钮指针*/
static lv_obj_t *start_btn = NULL;
static lv_obj_t *pause_btn = NULL;
static lv_obj_t *resume_btn = NULL;
static lv_obj_t *stop_btn = NULL;

/*按钮样式*/

static lv_style_t style_ctrl_btn;//通用按钮样式

/*统一初始化所有按钮的基础样式*/
static void btn_style_init(void)
{
    lv_style_init(&style_ctrl_btn);
    lv_style_set_radius(&style_ctrl_btn, 12);
    lv_style_set_text_color(&style_ctrl_btn, lv_color_white());
    lv_style_set_border_width(&style_ctrl_btn, 0);
}

/*ui 整体风格初始化*/
static void ui_style_init(void)
{
    btn_style_init();//按键的
}

static void apply_btn_color(lv_obj_t *btn, BtnType type)
{
    switch (type) {
    case BTN_START:
        lv_obj_set_style_bg_color(btn,
                                  lv_palette_main(LV_PALETTE_GREEN),
                                  LV_PART_MAIN);
        break;

    case BTN_PAUSE:
        lv_obj_set_style_bg_color(btn,
                                  lv_palette_main(LV_PALETTE_ORANGE),
                                  LV_PART_MAIN);
        break;

    case BTN_RESUME:
        lv_obj_set_style_bg_color(btn,
                                  lv_palette_main(LV_PALETTE_BLUE),
                                  LV_PART_MAIN);
        break;

    case BTN_STOP:
        lv_obj_set_style_bg_color(btn,
                                  lv_palette_main(LV_PALETTE_RED),
                                  LV_PART_MAIN);
        break;

    default:
        break;
    }
}

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

    //先统一添加公共外观
    lv_obj_add_style(btn, &style_ctrl_btn, LV_PART_MAIN);
    //再根据需求添加特定颜色
    apply_btn_color(btn, type);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

/*用flex写的按钮*/
static lv_obj_t *create_ctrl_btn_flex(lv_obj_t *parent,
                                      const char *text,
                                      const int w,
                                      const int h,
                                      BtnType type)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);

    lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(btn, (void *)(uintptr_t)type);

    lv_obj_add_style(btn, &style_ctrl_btn, LV_PART_MAIN);
    apply_btn_color(btn, type);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}



/*
            1. 获取屏幕并清空
            2. 初始化 style
            3. 创建标题                           
            4. 创建 Flex 容器
            5. 设置 Flex 规则
            6. 把按钮创建到容器里
            7. 创建状态 label
            8. 设置初始 AppState

*/
/*
                    一个控件固定放某个位置：
                    用 lv_obj_align()

                    多个控件要自动排队：
                        用 Flex

                    多个控件要放到指定行列：
                        用 Grid

*/
static int32_t main_col_dsc[] = {
    LV_GRID_FR(1),//这一列占满可用宽度
    LV_GRID_TEMPLATE_LAST//这一行/列规则到这里结束了
};

static int32_t main_row_dsc[] = {
    70,//第 0 行：70 像素，用来放标题
    LV_GRID_FR(1),//第 1 行：占剩下的空间，用来放按钮区
    50,//第 2 行：50 像素，用来放状态栏
    LV_GRID_TEMPLATE_LAST//这一行/列规则到这里结束了
};

/*
                    ┌──────────────────────────────┐
                    │ 第 0 行：标题，高 70           │
                    ├──────────────────────────────┤
                    │ 第 1 行：按钮区，占剩余空间     │
                    ├──────────────────────────────┤
                    │ 第 2 行：状态栏，高 50         │
                    └──────────────────────────────┘

*/


void ui_main_create(void)
{                         
    //注意，虽然scr运行完之后会消失，但是他指向的LVGL屏幕对象不会消失                  野指针的定义是指针还在，但是它指向的对象已经被释放/删除了
    lv_obj_t *scr = lv_screen_active();//获取当前活动的屏幕
    lv_obj_clean(scr);//清除屏幕上的旧控件

    ui_style_init();

    lv_obj_t *page = lv_obj_create(scr);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));//LV_PCT（100）把 page 的宽/高度设置成父对象宽度的 100%
    lv_obj_center(page);
                                        //main_col_dsc：主页面列描述  main_row_dsc：主页面行描述
    lv_obj_set_grid_dsc_array(page, main_col_dsc, main_row_dsc);//给 page 设置 Grid 布局 

    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    //LVGL 对 全屏铺满父容器 的 obj，默认是：无阴影、无轮廓、纯背景
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);

    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);//设置四周内边距



    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "FAST-LIVO2 Demo Control");
    // lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);//align  设置排放位置  LV_ALIGN_TOP_MID  顶部中间
    lv_obj_set_grid_cell(title,
                        LV_GRID_ALIGN_CENTER, 0, 1,//title 放在第 0 列，占 1 列  水平居中
                        LV_GRID_ALIGN_CENTER, 0, 1);//title 放在第 0 行，占 1 行    垂直居中


    lv_obj_t *btn_cont = lv_obj_create(page);
    lv_obj_set_size(btn_cont, 520, 200);
    // lv_obj_align(btn_cont, LV_ALIGN_CENTER, 0, 20);//LV_ALIGN_CENTER  居中对齐，整体往下偏一点

    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);//取消这个容器的可滚动属性
    lv_obj_set_scrollbar_mode(btn_cont, LV_SCROLLBAR_MODE_OFF);//关闭滚动条显示

    /* 容器透明 */                                                                                                       //LV_OPA_COVER 完全不透明，等于 100%
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, LV_PART_MAIN);//把容器背景透明度设置为完全透明   LV_OPA_TRANSP  :完全透明 LV_OPA_10   10% 不透明
    lv_obj_set_style_border_width(btn_cont, 0, LV_PART_MAIN);//边框    边框向里面
    lv_obj_set_style_outline_width(btn_cont, 0, LV_PART_MAIN);//外轮廓 边框向外面
    lv_obj_set_style_shadow_width(btn_cont, 0, LV_PART_MAIN);//阴影   边框旁边会加点黑黑的东西


    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW_WRAP);//LV_FLEX_FLOW_ROW → 横向排列（从左到右）WRAP 换行（一行放不下，自动换到下一行）       COLUMN = 列（竖
    lv_obj_set_flex_align(btn_cont,
                        LV_FLEX_ALIGN_CENTER,//管左右的
                        LV_FLEX_ALIGN_CENTER,//管上下的
                        LV_FLEX_ALIGN_CENTER);//管多行之间的  LV_FLEX_ALIGN_CENTER：可以理解为让控件之间间距均匀
                        
    lv_obj_set_style_pad_row(btn_cont, 20, LV_PART_MAIN);//设置按钮之间的行间距
    lv_obj_set_style_pad_column(btn_cont, 30, LV_PART_MAIN);//设置按钮之间的列间距

    lv_obj_set_grid_cell(btn_cont,
                     LV_GRID_ALIGN_CENTER, 0, 1,
                     LV_GRID_ALIGN_CENTER, 1, 1);//btn_cont 放到第 1 行


    start_btn  = create_ctrl_btn_flex(btn_cont, "START",  CTRL_BTN_W, CTRL_BTN_H, BTN_START);
    pause_btn  = create_ctrl_btn_flex(btn_cont, "PAUSE",  CTRL_BTN_W, CTRL_BTN_H, BTN_PAUSE);
    resume_btn = create_ctrl_btn_flex(btn_cont, "RESUME", CTRL_BTN_W, CTRL_BTN_H, BTN_RESUME);
    stop_btn   = create_ctrl_btn_flex(btn_cont, "STOP",   CTRL_BTN_W, CTRL_BTN_H, BTN_STOP);

    status_label = lv_label_create(page);
    lv_label_set_text(status_label, "Ready");
    // lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_obj_set_grid_cell(status_label,
                     LV_GRID_ALIGN_CENTER, 0, 1,
                     LV_GRID_ALIGN_CENTER, 2, 1);//btn_cont 放到第 1 行

    set_app_state(APP_STATE_READY);
}


