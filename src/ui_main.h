#ifndef UI_MAIN_H
#define UI_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    BTN_START,
    BTN_PAUSE,
    BTN_RESUME,
    BTN_STOP
} BtnType;

typedef enum {
    ENABLE,
    DISABLE
} BtnState;

typedef enum {
    APP_STATE_READY,
    APP_STATE_RUNNING,
    APP_STATE_PAUSED,
    APP_STATE_STOPPED
} AppState;

#define CTRL_BTN_W 180
#define CTRL_BTN_H 70

void ui_main_create(void);

#ifdef __cplusplus
}
#endif

#endif