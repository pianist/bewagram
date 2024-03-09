#include <cfg/common.h>
#include <cfg/sensor_config.h>
#include <errno.h>
#include <string.h>


void cfg_sensor_pretty_print(const struct SensorConfig* sc)
{
    printf("[sensor]\n");
    printf("Sensor_type = %s\n", sc->sensor_type);
    printf("DllFile     = %s\n", sc->dll_file);

    printf("\n[isp_image]\n");
    printf("Isp_x = %u\n", sc->isp.isp_x);
    printf("Isp_y = %u\n", sc->isp.isp_y);
    printf("Isp_W = %u\n", sc->isp.isp_w);
    printf("Isp_H = %u\n", sc->isp.isp_h);
    printf("Isp_FrameRate = %u\n", sc->isp.isp_frame_rate);

#define COMMENT_VALUES(key, arr, val) { printf(key"%s\n", arr[val]); for (int i = 0; arr[i]; ++i) \
        printf("\t\t\t; %c %s = %d\n", i == val ? '*' : ' ', arr[i], i); }

    COMMENT_VALUES("Isp_bayer     = ", cfg_sensor_vals_isp_bayer, sc->isp.isp_bayer);

    printf("\n[vi_dev]\n");
    COMMENT_VALUES("Input_mod    = ", cfg_sensor_vals_videv_input_mod, sc->videv.input_mod);
    COMMENT_VALUES("Work_mod     = ", cfg_sensor_vals_videv_work_mod, sc->videv.work_mod);
    COMMENT_VALUES("Combine_mode = ", cfg_sensor_vals_videv_combine_mode, sc->videv.combine_mode);
    COMMENT_VALUES("Comp_mode    = ", cfg_sensor_vals_videv_comp_mode, sc->videv.comp_mode);
    COMMENT_VALUES("Clock_edge   = ", cfg_sensor_vals_videv_clock_edge, sc->videv.clock_edge);

    printf("Mask_num = %u\n", sc->videv.mask_num);
    printf("Mask_0   = 0x%X\n", sc->videv.mask_0);
    printf("Mask_1   = 0x%X\n", sc->videv.mask_1);

    printf("Timingblank_HsyncHfb    = %u\t; Horizontal front blanking width\n", sc->videv.timing_blank_hsync_hfb);
    printf("Timingblank_HsyncAct    = %u\t; Horizontal effetive width\n", sc->videv.timing_blank_hsync_act);
    printf("Timingblank_HsyncHbb    = %u\t; Horizontal back blanking width\n", sc->videv.timing_blank_hsync_hbb);
    printf("Timingblank_VsyncVfb    = %u\t; Vertical front blanking height\n", sc->videv.timing_blank_vsync_vfb);
    printf("Timingblank_VsyncVact   = %u\t; Vertical effetive width\n", sc->videv.timing_blank_vsync_vact);
    printf("Timingblank_VsyncVbb    = %u\t; Vertical back blanking height\n", sc->videv.timing_blank_vsync_vbb);
    printf("Timingblank_VsyncVbfb   = %u\t; Even-field vertical front blanking height(interlace, invalid progressive)\n", sc->videv.timing_blank_vsync_vbfb);
    printf("Timingblank_VsyncVbact  = %u\t; Even-field vertical effetive width(interlace, invalid progressive)\n", sc->videv.timing_blank_vsync_vbact);
    printf("Timingblank_VsyncVbbb   = %u\t; Even-field vertical back blanking height(interlace, invalid progressive)\n", sc->videv.timing_blank_vsync_vbbb);

}

