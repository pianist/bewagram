#include <cfg/sensor_config.h>
#include <stdio.h>
#include <cfg/common.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#define SECTION_sensor      1
#define SECTION_mode        2
#define SECTION_mipi        3
#define SECTION_lvds        4
#define SECTION_vb_conf     5
#define SECTION_isp_image   10
#define SECTION_vi_dev      20
#define SECTION_vi_chn      21
#define SECTION_vpss_group  30
#define SECTION_vpss_crop   31
#define SECTION_vpss_chn    32
#define SECTION_venc_comm   99
#define SECTION_venc_0      100
#define SECTION_venc_1      101
#define SECTION_bind        1000


static int __cfg_sensor_current_section = 0;
static char __cfg_sensor_error_key[32];
static char __cfg_sensor_error_value[256];

const char* cfg_sensor_read_error_value()
{
    return __cfg_sensor_error_value;
}

const char* cfg_sensor_read_error_key()
{
    return __cfg_sensor_error_key;
}

#define ADD_SECTION(section) if (strcmp(s, #section) == 0) { __cfg_sensor_current_section = SECTION_##section; return CFG_PROC_OK; }

static int __cfg_sensor_read_section_cb(const char* s)
{
    ADD_SECTION(sensor)
    ADD_SECTION(mode)
    ADD_SECTION(mipi)
    ADD_SECTION(lvds)
    ADD_SECTION(vb_conf)
    ADD_SECTION(isp_image)
    ADD_SECTION(vi_dev)
    ADD_SECTION(vi_chn)
    ADD_SECTION(vpss_group)
    ADD_SECTION(vpss_crop)
    ADD_SECTION(vpss_chn)
    ADD_SECTION(venc_comm)
    ADD_SECTION(venc_0)
    ADD_SECTION(venc_1)
    ADD_SECTION(bind)
    if (strcmp(s, "vpss_corp") == 0)
    {
        //log_info("Wrong section [vpss_corp] in sensor config, igroring...");
        __cfg_sensor_current_section = SECTION_vpss_crop;
        return CFG_PROC_OK;
    }

    snprintf(__cfg_sensor_error_value, 256, "%s", s);
    return CFG_PROC_WRONG_SECTION;
}

struct SensorConfig* __current_sc = 0;
#define KEYVAL_PARAM_COPY_STR(key, dest, MAX) if (strcasecmp(key, k) == 0) { snprintf(dest, MAX, "%s", v); return CFG_PROC_OK; }
#define KEYVAL_PARAM_IGN(key) if (strcasecmp(key, k) == 0) { return CFG_PROC_OK; }

#define KEYVAL_PARAM_SL_dec(key, dest) if (strcasecmp(key, k) == 0) {   \
        char* endptr;                                                   \
        dest = strtol(v, &endptr, 10);                                  \
        if (!*endptr) return CFG_PROC_OK;                               \
        snprintf(__cfg_sensor_error_key, 256, "%s", k);                 \
        snprintf(__cfg_sensor_error_value, 256, "%s", v);               \
        return CFG_PROC_VALUE_BAD;                                      \
    }

#define KEYVAL_PARAM_UL_dec(key, dest) if (strcasecmp(key, k) == 0) {   \
        char* endptr;                                                   \
        dest = strtoul(v, &endptr, 10);                                 \
        if (!*endptr) return CFG_PROC_OK;                               \
        snprintf(__cfg_sensor_error_key, 256, "%s", k);                 \
        snprintf(__cfg_sensor_error_value, 256, "%s", v);               \
        return CFG_PROC_VALUE_BAD;                                      \
    }

#define KEYVAL_PARAM_UL_hex(key, dest) if (strcasecmp(key, k) == 0) {   \
        char* endptr;                                                   \
        if ((v[0] == '0' && v[1] == 'x')) {                             \
            dest = strtoul(&v[2], &endptr, 16);                         \
            if (!*endptr) return CFG_PROC_OK;                           \
        }                                                               \
        if ((v[0] == '0' && !v[1])) { dest = 0; return CFG_PROC_OK; }   \
        snprintf(__cfg_sensor_error_key, 256, "%s", k);                 \
        snprintf(__cfg_sensor_error_value, 256, "%s", v);               \
        return CFG_PROC_VALUE_BAD;                                      \
    }

#define KEYVAL_PARAM_ENUM(key, dest, pvs) if (strcasecmp(key, k) == 0) {        \
        unsigned n = 0;                                                         \
        while (pvs[n]) {                                                        \
            if (strcmp(pvs[n], v) == 0) {                                       \
                dest = n; return CFG_PROC_OK;                                   \
            }                                                                   \
            n++;                                                                \
        }                                                                       \
        char* endptr;                                                           \
        unsigned x = strtoul(v, &endptr, 10);                                   \
        if (!*endptr && (n > x)) {                                              \
            dest = x; return CFG_PROC_OK;                                       \
        }                                                                       \
        snprintf(__cfg_sensor_error_key, 256, "%s", k);                         \
        snprintf(__cfg_sensor_error_value, 256, "%s", v);                       \
        return CFG_PROC_VALUE_BAD;                                              \
    }

static int __cfg_sensor_read_keyval_cb_sensor(const char* k, const char* v)
{
    KEYVAL_PARAM_COPY_STR("sensor_type", __current_sc->sensor_type, 128);
    KEYVAL_PARAM_COPY_STR("dllfile", __current_sc->dll_file, 256);
    KEYVAL_PARAM_IGN("mode");

    snprintf(__cfg_sensor_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

static int __cfg_sensor_read_keyval_cb_mode(const char* k, const char* v)
{
    KEYVAL_PARAM_IGN("input_mode");
    KEYVAL_PARAM_IGN("dev_attr");

    snprintf(__cfg_sensor_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

const char *cfg_sensor_vals_isp_bayer[] = { "BAYER_RGGB", "BAYER_GRBG", "BAYER_GBRG", "BAYER_BGGR", 0 };

static int __cfg_sensor_read_keyval_cb_isp_image(const char* k, const char* v)
{
    KEYVAL_PARAM_UL_dec("isp_x", __current_sc->isp.isp_x);
    KEYVAL_PARAM_UL_dec("isp_y", __current_sc->isp.isp_y);
    KEYVAL_PARAM_UL_dec("isp_w", __current_sc->isp.isp_w);
    KEYVAL_PARAM_UL_dec("isp_h", __current_sc->isp.isp_h);
    KEYVAL_PARAM_UL_dec("isp_framerate", __current_sc->isp.isp_frame_rate);
    KEYVAL_PARAM_ENUM("isp_bayer", __current_sc->isp.isp_bayer, cfg_sensor_vals_isp_bayer);

    snprintf(__cfg_sensor_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

const char *cfg_sensor_vals_bool[] = { "FALSE", "TRUE", 0 };
const char *cfg_sensor_vals_videv_input_mod[] = { "VI_INPUT_MODE_BT656", "VI_INPUT_MODE_BT601", "VI_INPUT_MODE_DIGITAL_CAMERA", "VI_INPUT_MODE_INTERLEAVED", "VI_INPUT_MODE_MIPI", "VI_INPUT_MODE_LVDS", "VI_INPUT_MODE_HISPI", 0 };
const char *cfg_sensor_vals_videv_work_mod[] = { "VI_WORK_MODE_1Multiplex", "VI_WORK_MODE_2Multiplex", "VI_WORK_MODE_4Multiplex", 0};
const char *cfg_sensor_vals_videv_combine_mode[] = { "VI_COMBINE_COMPOSITE", "VI_COMBINE_SEPARATE", 0};
const char *cfg_sensor_vals_videv_comp_mode[] = { "VI_COMP_MODE_SINGLE", "VI_COMP_MODE_DOUBLE", 0};
const char *cfg_sensor_vals_videv_clock_edge[] = { "VI_CLK_EDGE_SINGLE_UP", "VI_CLK_EDGE_SINGLE_DOWN", "VI_CLK_EDGE_DOUBLE", 0 };
const char *cfg_sensor_vals_videv_scan_mode[] = { "VI_SCAN_INTERLACED", "VI_SCAN_PROGRESSIVE", 0 };
const char *cfg_sensor_vals_videv_data_seq[] = { "VI_INPUT_DATA_UYVY", "VI_INPUT_DATA_VYUY", "VI_INPUT_DATA_YUYV", "VI_INPUT_DATA_YVYU", 0 };
const char *cfg_sensor_vals_videv_vsync[] = { "VI_VSYNC_FIELD", "VI_VSYNC_PULSE", 0 };
const char *cfg_sensor_vals_videv_vsync_neg[] = { "VI_VSYNC_NEG_HIGH", "VI_VSYNC_NEG_LOW", 0 };
const char *cfg_sensor_vals_videv_hsync[] = { "VI_HSYNC_VALID_SINGNAL", "VI_HSYNC_PULSE", 0 };
const char *cfg_sensor_vals_videv_hsync_neg[] = { "VI_HSYNC_VALID_SINGNAL", "VI_HSYNC_PULSE", 0 };
const char *cfg_sensor_vals_videv_vsync_valid[] = { "VI_VSYNC_NORM_PULSE", "VI_VSYNC_VALID_SINGAL", 0 } ;
const char *cfg_sensor_vals_videv_vsync_valid_neg[] = { "VI_VSYNC_VALID_NEG_HIGH", "VI_VSYNC_VALID_NEG_LOW", 0 } ;
const char *cfg_sensor_vals_videv_fix_code[] = { "BT656_FIXCODE_1", "BT656_FIXCODE_0", 0 } ;
const char *cfg_sensor_vals_videv_field_polar[] = { "BT656_FIELD_POLAR_STD", "BT656_FIELD_POLAR_NSTD", 0 } ;
const char *cfg_sensor_vals_videv_data_path[] = { "VI_PATH_BYPASS", "VI_PATH_ISP", "VI_PATH_RAW", 0 } ;
const char *cfg_sensor_vals_videv_input_data_type[] = { "VI_DATA_TYPE_YUV", "VI_DATA_TYPE_RGB", 0 } ;

static int __cfg_sensor_read_keyval_cb_vi_dev(const char* k, const char* v)
{
    KEYVAL_PARAM_ENUM("input_mod", __current_sc->videv.input_mod, cfg_sensor_vals_videv_input_mod);
    KEYVAL_PARAM_ENUM("work_mod", __current_sc->videv.work_mod, cfg_sensor_vals_videv_work_mod);
    KEYVAL_PARAM_ENUM("combine_mode", __current_sc->videv.combine_mode, cfg_sensor_vals_videv_combine_mode);
    KEYVAL_PARAM_ENUM("comp_mode", __current_sc->videv.comp_mode, cfg_sensor_vals_videv_comp_mode);
    KEYVAL_PARAM_ENUM("clock_edge", __current_sc->videv.clock_edge, cfg_sensor_vals_videv_clock_edge);
    KEYVAL_PARAM_UL_dec("mask_num", __current_sc->videv.mask_num);
    KEYVAL_PARAM_UL_hex("mask_0", __current_sc->videv.mask_0);
    KEYVAL_PARAM_UL_hex("mask_1", __current_sc->videv.mask_1);
    KEYVAL_PARAM_ENUM("scan_mode", __current_sc->videv.scan_mode, cfg_sensor_vals_videv_scan_mode);
    KEYVAL_PARAM_ENUM("data_seq", __current_sc->videv.data_seq, cfg_sensor_vals_videv_data_seq);
    KEYVAL_PARAM_ENUM("vsync", __current_sc->videv.vsync, cfg_sensor_vals_videv_vsync);
    KEYVAL_PARAM_ENUM("vsyncneg", __current_sc->videv.vsync_neg, cfg_sensor_vals_videv_vsync_neg);
    KEYVAL_PARAM_ENUM("hsync", __current_sc->videv.hsync, cfg_sensor_vals_videv_hsync);
    KEYVAL_PARAM_ENUM("hsyncneg", __current_sc->videv.hsync_neg, cfg_sensor_vals_videv_hsync_neg);
    KEYVAL_PARAM_ENUM("vsyncvalid", __current_sc->videv.vsync_valid, cfg_sensor_vals_videv_vsync_valid);
    KEYVAL_PARAM_ENUM("vsyncvalidneg", __current_sc->videv.vsync_valid_neg, cfg_sensor_vals_videv_vsync_valid_neg);
    KEYVAL_PARAM_UL_dec("timingblank_hsynchfb", __current_sc->videv.timing_blank_hsync_hfb);
    KEYVAL_PARAM_UL_dec("timingblank_hsyncact", __current_sc->videv.timing_blank_hsync_act);
    KEYVAL_PARAM_UL_dec("timingblank_hsynchbb", __current_sc->videv.timing_blank_hsync_hbb);
    KEYVAL_PARAM_UL_dec("timingblank_vsyncvfb", __current_sc->videv.timing_blank_vsync_vfb);
    KEYVAL_PARAM_UL_dec("timingblank_vsyncvact", __current_sc->videv.timing_blank_vsync_vact);
    KEYVAL_PARAM_UL_dec("timingblank_vsyncvbb", __current_sc->videv.timing_blank_vsync_vbb);
    KEYVAL_PARAM_UL_dec("timingblank_vsyncvbfb", __current_sc->videv.timing_blank_vsync_vbfb);
    KEYVAL_PARAM_UL_dec("timingblank_vsyncvbact", __current_sc->videv.timing_blank_vsync_vbact);
    KEYVAL_PARAM_UL_dec("timingblank_vsyncvbbb", __current_sc->videv.timing_blank_vsync_vbbb);
    KEYVAL_PARAM_ENUM("fixcode", __current_sc->videv.fix_code, cfg_sensor_vals_videv_fix_code);
    KEYVAL_PARAM_ENUM("fieldpolar", __current_sc->videv.field_polar, cfg_sensor_vals_videv_field_polar);
    KEYVAL_PARAM_ENUM("datapath", __current_sc->videv.data_path, cfg_sensor_vals_videv_data_path);
    KEYVAL_PARAM_ENUM("inputdatatype", __current_sc->videv.input_data_type, cfg_sensor_vals_videv_input_data_type);
    KEYVAL_PARAM_ENUM("datarev", __current_sc->videv.data_rev, cfg_sensor_vals_bool);
    KEYVAL_PARAM_UL_dec("devrect_x", __current_sc->videv.dev_rect_x);
    KEYVAL_PARAM_UL_dec("devrect_y", __current_sc->videv.dev_rect_y);
    KEYVAL_PARAM_UL_dec("devrect_w", __current_sc->videv.dev_rect_w);
    KEYVAL_PARAM_UL_dec("devrect_h", __current_sc->videv.dev_rect_h);

    snprintf(__cfg_sensor_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

static int __cfg_sensor_read_keyval_cb_vb_conf(const char* k, const char* v)
{
    KEYVAL_PARAM_UL_dec("vbcnt", __current_sc->vb_cnt);
    KEYVAL_PARAM_IGN("vbtimes");

    snprintf(__cfg_sensor_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

const char *cfg_sensor_vals_vichn_capsel[] = { "VI_CAPSEL_TOP", "VI_CAPSEL_BOTTOM", "VI_CAPSEL_BOTH", 0 } ;
const char *cfg_sensor_vals_vichn_pixel_format[] = { 
            "PIXEL_FORMAT_RGB_1BPP",
            "PIXEL_FORMAT_RGB_2BPP",
            "PIXEL_FORMAT_RGB_4BPP",
            "PIXEL_FORMAT_RGB_8BPP",
            "PIXEL_FORMAT_RGB_444",
            "PIXEL_FORMAT_RGB_4444",
            "PIXEL_FORMAT_RGB_555",
            "PIXEL_FORMAT_RGB_565",
            "PIXEL_FORMAT_RGB_1555",
            "PIXEL_FORMAT_RGB_888",
            "PIXEL_FORMAT_RGB_8888",
            "PIXEL_FORMAT_RGB_PLANAR_888",
            "PIXEL_FORMAT_RGB_BAYER_8BPP",
            "PIXEL_FORMAT_RGB_BAYER_10BPP",
            "PIXEL_FORMAT_RGB_BAYER_12BPP",
            "PIXEL_FORMAT_RGB_BAYER_14BPP",
            "PIXEL_FORMAT_RGB_BAYER",
            "PIXEL_FORMAT_YUV_A422",
            "PIXEL_FORMAT_YUV_A444",
            "PIXEL_FORMAT_YUV_PLANAR_422",
            "PIXEL_FORMAT_YUV_PLANAR_420",
            "PIXEL_FORMAT_YUV_PLANAR_444",
            "PIXEL_FORMAT_YUV_SEMIPLANAR_422",
            "PIXEL_FORMAT_YUV_SEMIPLANAR_420",
            "PIXEL_FORMAT_YUV_SEMIPLANAR_444",
            "PIXEL_FORMAT_UYVY_PACKAGE_422",
            "PIXEL_FORMAT_YUYV_PACKAGE_422",
            "PIXEL_FORMAT_VYUY_PACKAGE_422",
            "PIXEL_FORMAT_YCbCr_PLANAR",
            "PIXEL_FORMAT_YUV_400",
        0 } ;


static int __cfg_sensor_read_keyval_cb_vi_chn(const char* k, const char* v)
{
    KEYVAL_PARAM_UL_dec("caprect_x", __current_sc->vichn.cap_rect_x);
    KEYVAL_PARAM_UL_dec("caprect_y", __current_sc->vichn.cap_rect_y);
    KEYVAL_PARAM_UL_dec("caprect_width", __current_sc->vichn.cap_rect_width);
    KEYVAL_PARAM_UL_dec("caprect_height", __current_sc->vichn.cap_rect_height);
    KEYVAL_PARAM_UL_dec("destsize_width", __current_sc->vichn.dest_size_width);
    KEYVAL_PARAM_UL_dec("destsize_height", __current_sc->vichn.dest_size_height);
    KEYVAL_PARAM_ENUM("capsel", __current_sc->vichn.cap_sel, cfg_sensor_vals_vichn_capsel);
    KEYVAL_PARAM_ENUM("pixformat",__current_sc->vichn.pix_format, cfg_sensor_vals_vichn_pixel_format);
    KEYVAL_PARAM_IGN("compressmode");
    KEYVAL_PARAM_UL_dec("srcframerate", __current_sc->vichn.src_frame_rate);
    KEYVAL_PARAM_UL_dec("framerate", __current_sc->vichn.frame_rate);

    snprintf(__cfg_sensor_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

#define KEYVAL_CASE(section) case SECTION_##section: { return __cfg_sensor_read_keyval_cb_##section(k, v); }

static int __cfg_sensor_read_keyval_cb(const char* k, const char* v)
{
    switch (__cfg_sensor_current_section)
    {
        KEYVAL_CASE(vb_conf)
        KEYVAL_CASE(sensor)
        KEYVAL_CASE(mode)
        KEYVAL_CASE(isp_image)
        KEYVAL_CASE(vi_dev)
        KEYVAL_CASE(vi_chn)
    }
//    printf("%s=%s\n", k, v);
    return CFG_PROC_OK;
}



int cfg_sensor_read(const char* fname, struct SensorConfig* sc)
{
    __cfg_sensor_current_section = 0;
    __current_sc = sc;

    memset(&sc, 0, sizeof(struct SensorConfig));

    return cfg_proc_read(fname, __cfg_sensor_read_section_cb, __cfg_sensor_read_keyval_cb);
}


