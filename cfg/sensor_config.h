#pragma once

#include "compat.h"
#include "common.h"

#if HISILICON_SDK_GEN >= 2
struct SensorMIPI {
    raw_data_type_e data_type;
    int lane_id[8];
};

struct SensorLVDS {
    int img_size_w;
    int img_size_h;
    wdr_mode_t wdr_mode;
    lvds_sync_mode_t sync_mode;
    data_type_t raw_data_type;
    lvds_bit_endian_t data_endian;
    lvds_bit_endian_t sync_code_endian;
    int lane_id[8];
    int lvds_lane_num;
    int wdr_vc_num;
    int sync_code_num;
    int sync_code[8][16];
};
#endif

struct SensorVIDEV {
    VI_INPUT_MODE_E input_mod;
    VI_WORK_MODE_E work_mod;
    VI_COMBINE_MODE_E combine_mode;
    VI_COMP_MODE_E comp_mode;
    VI_CLK_EDGE_E clock_edge;
    unsigned int mask_num;
    unsigned int mask_0;
    unsigned int mask_1;
    VI_SCAN_MODE_E scan_mode;
    VI_DATA_YUV_SEQ_E data_seq;
    VI_VSYNC_E vsync;
    VI_VSYNC_NEG_E vsync_neg;
    VI_HSYNC_E hsync;
    VI_HSYNC_NEG_E hsync_neg;
    VI_VSYNC_VALID_E vsync_valid;
    VI_VSYNC_VALID_NEG_E vsync_valid_neg;
    unsigned int timing_blank_hsync_hfb;
    unsigned int timing_blank_hsync_act;
    unsigned int timing_blank_hsync_hbb;
    unsigned int timing_blank_vsync_vfb;
    unsigned int timing_blank_vsync_vact;
    unsigned int timing_blank_vsync_vbb;
    unsigned int timing_blank_vsync_vbfb;
    unsigned int timing_blank_vsync_vbact;
    unsigned int timing_blank_vsync_vbbb;

    BT656_FIXCODE_E fix_code;
    BT656_FIELD_POLAR_E field_polar;
    VI_DATA_PATH_E data_path;
    VI_DATA_TYPE_E input_data_type;
    int data_rev;
    int dev_rect_x;
    int dev_rect_y;
    unsigned int dev_rect_w;
    unsigned int dev_rect_h;
};

extern const char *cfg_sensor_vals_bool[];
extern const char *cfg_sensor_vals_videv_input_mod[];
extern const char *cfg_sensor_vals_videv_work_mod[];
extern const char *cfg_sensor_vals_videv_combine_mode[];
extern const char *cfg_sensor_vals_videv_comp_mode[];
extern const char *cfg_sensor_vals_videv_clock_edge[];
extern const char *cfg_sensor_vals_videv_scan_mode[];
extern const char *cfg_sensor_vals_videv_data_seq[];
extern const char *cfg_sensor_vals_videv_vsync[];
extern const char *cfg_sensor_vals_videv_vsync_neg[];
extern const char *cfg_sensor_vals_videv_hsync[];
extern const char *cfg_sensor_vals_videv_hsync_neg[];
extern const char *cfg_sensor_vals_videv_vsync_valid[];
extern const char *cfg_sensor_vals_videv_vsync_valid_neg[];
extern const char *cfg_sensor_vals_videv_fix_code[];
extern const char *cfg_sensor_vals_videv_field_polar[];
extern const char *cfg_sensor_vals_videv_data_path[];
extern const char *cfg_sensor_vals_videv_input_data_type[];

struct SensorISP {
    int isp_x;
    int isp_y;
    unsigned int isp_h;
    unsigned int isp_w;
    unsigned int isp_frame_rate;
    ISP_BAYER_FORMAT_E isp_bayer;
};

extern const char *cfg_sensor_vals_isp_bayer[];

struct SensorVIChn {
    int cap_rect_x;
    int cap_rect_y;
    unsigned int cap_rect_width;
    unsigned int cap_rect_height;
    unsigned int dest_size_width;
    unsigned int dest_size_height;
    VI_CAPSEL_E cap_sel;
    PIXEL_FORMAT_E pix_format;
#if HISILICON_SDK_GEN >= 2
    COMPRESS_MODE_E compress_mode;
#endif
    int src_frame_rate;
    int frame_rate;
};

extern const char *cfg_sensor_vals_vichn_capsel[];
extern const char *cfg_sensor_vals_vichn_pixel_format[];

struct SensorConfig {
    // VB
    unsigned vb_cnt;

    // [sensor]
    char sensor_type[128];
#if HISILICON_SDK_GEN >= 2
    WDR_MODE_E mode;
#endif
    char dll_file[256];

#if HISILICON_SDK_GEN >= 2
    // [mode]
    input_mode_t input_mode;

    // [mipi]
    struct SensorMIPI mipi;

    // [lvds]
    struct SensorLVDS lvds;
#endif

    // [isp_image]
    struct SensorISP isp;

    // [vi_dev]
    struct SensorVIDEV videv;

    // [vi_chn]
    struct SensorVIChn vichn;
};

int cfg_sensor_read(const char* fname, struct SensorConfig* sc);
const char* cfg_sensor_read_error_key();
const char* cfg_sensor_read_error_value();

void cfg_sensor_pretty_print(const struct SensorConfig* sc);


