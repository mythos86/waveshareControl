SET(SOURCES ui_Screen1.c
    ui_menuSCR.c
    ui_switchesSCR.c
    ui_setSCR.c
    ui_brightnessSCR.c
    ui_offTimeSCR.c
    ui_setDateSCR.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_1107282690.c
    ui_img_1970314926.c
    ui_img_1512711676.c
    fonts/ui_font_secfont.c
    fonts/ui_font_small.c
    fonts/ui_font_timeFont.c)

add_library(ui ${SOURCES})
