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

void ui_main_create(void);

#ifdef __cplusplus
}
#endif

#endif