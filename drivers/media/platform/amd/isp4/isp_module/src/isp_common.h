/*
 * Copyright 2024 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ISP_COMMON_H
#define ISP_COMMON_H

#include "os_base_type.h"
#include "os_advance_type.h"
#include "isp_fw_if/isp_hw_reg.h"
#include "isp_fw_if/base_types_pub.h"
#include "isp_fw_if/global_param_types_pub.h"
#include "isp_fw_if/param_types_pub.h"
#include "isp_module_if.h"
#include "buffer_mgr.h"
#include "isp_fw_if/drv_isp_if.h"
#include "swisp_if.h"
#include "isp_queue.h"
#include "isp_pwr.h"
#include "isp_para_capability.h"

#define ISP_LOGRB_SIZE (2 * 1024 * 1024)
#define RB_MAX (25)
#define RESP_CHAN_TO_RB_OFFSET (9)
#define RB_PMBMAP_MEM_SIZE (16 * 1024 * 1024 - 1)
#define RB_PMBMAP_MEM_CHUNK (RB_PMBMAP_MEM_SIZE / (RB_MAX - 1))

#define ISP_ADDR_ALIGN_UP(addr, addr_align) ALIGN((addr), (addr_align))
#define ISP_SIZE_ALIGN_UP(size, size_align) ALIGN((addr), (size_align))

#define ISP_ALIGN_SIZE_1K (0x400)
#define ISP_ALIGN_SIZE_4K (0x1000)
#define ISP_ALIGN_SIZE_32K (0x8000)
#define ISP_BUFF_PADDING_64K (0x10000)

#define ISP_ADDR_ALIGN_UP_1K(addr) (ISP_ADDR_ALIGN_UP(addr, ISP_ALIGN_SIZE_1K))
#define ISP_ADDR_ALIGN_UP_4K(size) (ISP_ADDR_ALIGN_UP(size, ISP_ALIGN_SIZE_4K))
#define ISP_SIZE_ALIGN_UP_32K(size) \
	(ISP_ADDR_ALIGN_UP(size, ISP_ALIGN_SIZE_32K))

/* fw binary, stack, heap, etc */
#define ISP_RESV_FB_SIZE_DEFAULT (2 * 1024 * 1024)

#define ISP_FW_WORK_BUF_SIZE (12 * 1024 * 1024)
#define CMD_RESPONSE_BUF_SIZE (64 * 1024)
#define MAX_CMD_RESPONSE_BUF_SIZE (4 * 1024)
#define MIN_CHANNEL_BUF_CNT_BEFORE_START_STREAM 4

/*
 * command single buffer is to save small data for some indirect commands
 * Max single buffer is 4K for the current commands with single buffer.
 */
#define MAX_SINGLE_BUF_SIZE (4 * 1024)

#define INDIRECT_BUF_SIZE (12 * 1024)
#define INDIRECT_BUF_CNT 100
#define BRSZ_FULL_RAW_TMP_BUF_SIZE \
	(MAX_REAR_SENSOR_WIDTH * MAX_REAR_SENSOR_HEIGHT * 2)
#define BRSZ_FULL_RAW_TMP_BUF_CNT 4

#define META_INFO_BUF_SIZE ISP_SIZE_ALIGN_UP_32K(sizeof(struct meta_info_t))
#define META_INFO_BUF_CNT 4
#define META_DATA_BUF_SIZE (128 * 1024)

/*
 * ISP driver need wait for the in-flight frame to be processed by FW.
 * Do synchronized stop to wait for the process.
 */
#define DO_SYNCHRONIZED_STOP_STREAM

/*
 * The SEND_FW_CMD_TIMEOUT is used in tuning tool when sending FW command.
 * Some FW commands like dump engineer data needs 4 frames and during
 * development phase we sometimes enabled very low fps around 1, so the time
 * is about 4S, it'll be 5S by adding some redundancy
 */
#define SEND_FW_CMD_TIMEOUT (1000 * 5) /* in MS */

#define SKIP_FRAME_COUNT_AT_START 0

#define ISP_MC_ADDR_ALIGN (1024 * 32)
#define ISP_MC_PREFETCH_GAP (1024 * 32)

#define BUF_NUM_BEFORE_START_CMD 2
#define BUFFER_ALIGN_SIZE (0x400)

#define MAX_HOST2FW_SEQ_NUM (16 * 1024)
#define HOST2FW_COMMAND_SIZE (sizeof(struct cmd_t))
#define FW2HOST_RESPONSE_SIZE (sizeof(struct resp_t))

#define MAX_NUM_HOST2FW_COMMAND (40)
#define MAX_NUM_FW2HOST_RESPONSE (1000)

#define ISP_FW_CODE_BUF_SIZE (2 * 1024 * 1024)
#define ISP_FW_STACK_BUF_SIZE (8 * 64 * 1024)
#define ISP_FW_HEAP_BUF_SIZE (11 * 1024 * 1024 / 2)
#define ISP_FW_TRACE_BUF_SIZE ISP_LOGRB_SIZE
#define ISP_FW_CMD_BUF_SIZE (MAX_NUM_HOST2FW_COMMAND * HOST2FW_COMMAND_SIZE)
#define ISP_FW_CMD_BUF_COUNT 4
#define ISP_FW_RESP_BUF_SIZE (MAX_NUM_FW2HOST_RESPONSE * FW2HOST_RESPONSE_SIZE)
#define ISP_FW_RESP_BUF_COUNT 4

#define ISP_FW_CMD_PAY_LOAD_BUF_SIZE                     \
	(ISP_FW_WORK_BUF_SIZE -                          \
	 (ISP_FW_CODE_BUF_SIZE + ISP_FW_STACK_BUF_SIZE + \
	  ISP_FW_HEAP_BUF_SIZE + ISP_FW_TRACE_BUF_SIZE + \
	  ISP_FW_CMD_BUF_SIZE * ISP_FW_CMD_BUF_COUNT +   \
	  ISP_FW_RESP_BUF_SIZE * ISP_FW_RESP_BUF_COUNT))

#define ISP_FW_CMD_PAY_LOAD_BUF_ALIGN 64

#define STREAM_META_BUF_COUNT 6

#define MAX_REAL_FW_RESP_STREAM_NUM 4

/* value of result_t */
#define RET_SUCCESS 0
#define RET_FAILURE 1 /* general failure */
#define RET_NOTSUPP 2 /* feature not supported */
#define RET_BUSY 3 /* there's already something going on... */
#define RET_CANCELED 4 /* operation canceled */
#define RET_OUTOFMEM 5 /* out of memory */
#define RET_OUTOFRANGE 6 /* parameter/value out of range */
#define RET_IDLE 7 /* feature/subsystem is in idle state */
#define RET_WRONG_HANDLE 8 /* handle is wrong */
#define RET_NULL_POINTER 9 /* the parameter is NULL pointer */
#define RET_NOTAVAILABLE 10 /* profile not available */
#define RET_DIVISION_BY_ZERO 11 /* a divisor equals ZERO */
#define RET_WRONG_STATE 12 /* state machine in wrong state */
#define RET_INVALID_PARM 13 /* invalid parameter */
#define RET_PENDING 14 /* command pending */
#define RET_WRONG_CONFIG 15 /* givin configuration is invalid */
#define RET_TIMEOUT 16 /* time out */
#define RET_INVALID_PARAM 17

#define MAX_MODE_TYPE_STR_LEN 16

#define MAX_SLEEP_COUNT (10)
#define MAX_SLEEP_TIME (100)

#define ISP_GET_STATUS(isp) ((isp)->isp_status)
#define ISP_SET_STATUS(isp, state)                                      \
	{                                                               \
		struct isp_context *c = (isp);				\
		enum isp_status s = (state);                            \
									\
		c->isp_status = s;					\
		if (s == ISP_STATUS_FW_RUNNING)				\
			isp_get_cur_time_tick(                          \
				&c->isp_pu_isp.idle_start_time);	\
		else                                                    \
			c->isp_pu_isp.idle_start_time = MAX_ISP_TIME_TICK;\
	}

#define MAX_REG_DUMP_SIZE (64 * 1024) /* 64KB for each subIp register dump */

#define ISP_SEMAPHORE_ID_X86 0x0100
#define ISP_SEMAPHORE_ATTEMPTS 15
#define ISP_SEMAPHORE_DELAY 10 /* ms */

enum fw_cmd_resp_stream_id {
	FW_CMD_RESP_STREAM_ID_GLOBAL = 0,
	FW_CMD_RESP_STREAM_ID_1 = 1,
	FW_CMD_RESP_STREAM_ID_2 = 2,
	FW_CMD_RESP_STREAM_ID_3 = 3,
	FW_CMD_RESP_STREAM_ID_MAX = 4
};

enum fw_cmd_para_type {
	FW_CMD_PARA_TYPE_INDIRECT = 0,
	FW_CMD_PARA_TYPE_DIRECT = 1
};

enum list_type_id {
	LIST_TYPE_ID_FREE = 0,
	LIST_TYPE_ID_IN_FW = 1,
	LIST_TYPE_ID_MAX = 2
};

enum isp_status {
	ISP_STATUS_UNINITED,
	ISP_STATUS_INITED,
	ISP_STATUS_PWR_OFF = ISP_STATUS_INITED,
	ISP_STATUS_PWR_ON,
	ISP_STATUS_FW_RUNNING,
	ISP_FSM_STATUS_MAX
};

enum start_status {
	START_STATUS_NOT_START,
	START_STATUS_STARTING,
	START_STATUS_STARTED,
	START_STATUS_START_FAIL,
	START_STATUS_START_STOPPING
};

enum isp_aspect_ratio {
	ISP_ASPECT_RATIO_16_9, /* 16:9 */
	ISP_ASPECT_RATIO_16_10, /* 16:10 */
	ISP_ASPECT_RATIO_4_3, /* 4:3 */
};

#define STREAM__PREVIEW_OUTPUT_BIT BIT(STREAM_ID_PREVIEW)
#define STREAM__VIDEO_OUTPUT_BIT BIT(STREAM_ID_VIDEO)
#define STREAM__ZSL_OUTPUT_BIT BIT(STREAM_ID_ZSL)

struct isp_mc_addr_node {
	u64 start_addr;
	u64 align_addr;
	u64 end_addr;
	u64 size;
	struct isp_mc_addr_node *next;
	struct isp_mc_addr_node *prev;
};

struct isp_mc_addr_mgr {
	struct isp_mc_addr_node head;
	struct mutex mutex; /* mutex */
	u64 start;
	u64 len;
};

struct sys_to_mc_map_info {
	u64 sys_addr;
	u64 mc_addr;
	u32 len;
};

struct isp_mapped_buf_info {
	struct list_node node;
	u8 camera_port_id;
	u8 stream_id;
	struct sys_img_buf_info *sys_img_buf_hdl;
	u64 multi_map_start_mc;
	struct sys_to_mc_map_info y_map_info;
	struct sys_to_mc_map_info u_map_info;
	struct sys_to_mc_map_info v_map_info;
#ifdef USING_KGD_CGS
	cgs_handle_t cgs_hdl;
#else
	void *map_hdl;
	void *cos_mem_handle;
	void *mdl_for_map;
	struct isp_gpu_mem_info *map_sys_to_fb_gpu_info;
#endif
};

struct isp_stream_info {
	enum pvt_img_fmt format;
	u32 width;
	u32 height;
	u32 fps;
	u32 luma_pitch_set;
	u32 chroma_pitch_set;
	u32 max_fps_numerator;
	u32 max_fps_denominator;
	struct isp_list buf_free;
	struct isp_list buf_in_fw;
	enum start_status start_status;
	u8 running;
	u8 buf_num_sent;
};

struct roi_info {
	u32 h_offset;
	u32 v_offset;
	u32 h_size;
	u32 v_size;
};

struct isp_cos_sys_mem_info {
	u64 mem_size;
	void *sys_addr;
	void *mem_handle;
};

struct isp_sensor_info {
	enum camera_port_id cid;
	enum camera_port_id actual_cid;
	enum fw_cmd_resp_stream_id fw_stream_id;
	u64 meta_mc[STREAM_META_BUF_COUNT];
	enum start_status status;
	struct roi_info ae_roi;
	struct roi_info af_roi[MAX_AF_ROI_NUM];
	struct roi_info awb_region;
	u32 raw_width;
	u32 raw_height;
	struct isp_stream_info str_info[STREAM_ID_NUM + 1];

	u32 zsl_ret_width;
	u32 zsl_ret_height;
	u32 zsl_ret_stride;
	u32 open_flag;
	enum camera_type cam_type;
	enum camera_type cam_type_prev;
	enum fw_cmd_resp_stream_id stream_id;
	u8 zsl_enable;
	u8 resend_zsl_enable;
	char cur_res_fps_id;
	char sensor_opened;
	char hdr_enable;
	char tnr_enable;
	char start_str_cmd_sent;
	char channel_buf_sent_cnt;
	u32 poc;
};

#define I2C_REGADDR_NULL 0xffff

struct isp_cmd_element {
	u32 seq_num;
	u32 cmd_id;
	enum fw_cmd_resp_stream_id stream;
	u64 mc_addr;
	s64 send_time;
	struct isp_event *evt;
	struct isp_gpu_mem_info *gpu_pkg;
	void *resp_payload;
	u32 *resp_payload_len;
	u16 i2c_reg_addr;
	enum camera_port_id cam_id;
	struct isp_cmd_element *next;
};

enum isp_pipe_used_status {
	ISP_PIPE_STATUS_USED_BY_NONE,
	ISP_PIPE_STATUS_USED_BY_CAM_R = (CAMERA_PORT_0 + 1),
	ISP_PIPE_STATUS_USED_BY_CAM_FL = (CAMERA_PORT_1 + 1),
	ISP_PIPE_STATUS_USED_BY_CAM_FR = (CAMERA_PORT_2 + 1)
};

enum isp_config_mode {
	ISP_CONFIG_MODE_INVALID,
	ISP_CONFIG_MODE_PREVIEW,
	ISP_CONFIG_MODE_RAW,
	ISP_CONFIG_MODE_VIDEO2D,
	ISP_CONFIG_MODE_VIDEO3D,
	ISP_CONFIG_MODE_VIDEOSIMU,
	ISP_CONFIG_MODE_DATAT_TRANSFER,
	ISP_CONFIG_MODE_MAX
};

enum isp_bayer_pattern {
	ISP_BAYER_PATTERN_INVALID,
	ISP_BAYER_PATTERN_RGRGGBGB,
	ISP_BAYER_PATTERN_GRGRBGBG,
	ISP_BAYER_PATTERN_GBGBRGRG,
	ISP_BAYER_PATTERN_BGBGGRGR,
	ISP_BAYER_PATTERN_MAX
};

struct isp_config_mode_param {
	enum isp_config_mode mode;
	bool disable_calib;
	union mode_param_t {
		struct _loopback_preview_param_t_unused {
			bool en_continue;
			enum isp_bayer_pattern bayer_pattern;
			char *raw_file;
		} loopback_preview;
	} mode_param;
};

struct isp_config_preview_param {
	u32 preview_width;
	u32 preview_height;
	u32 preview_luma_picth;
	u32 preview_chroma_pitch;
	bool disable_calib;
};

enum isp_gpu_mem_src {
	ISP_GPU_MEM_SRC_FB_FROM_GFX,
	ISP_GPU_MEM_SRC_NFB_FROM_GFX,
	ISP_GPU_MEM_SRC_NFB_FROM_ISPDRV,
	ISP_GPU_MEM_SRC_MAX
};

struct dmft_shared_buf_info {
	void *usr_base;
	void *sys_base;
	u64 ipu_base;
	u32 len;
	u32 align;
	void *mdl; /* use PVOID instead of PMDL to avoid system dependence */
	struct isp_gpu_mem_info *tmp_buf;
};

enum fw_cmd_resp_str_status {
	FW_CMD_RESP_STR_STATUS_IDLE = 0,
	FW_CMD_RESP_STR_STATUS_OCCUPIED,
	FW_CMD_RESP_STR_STATUS_INITIALED,
};

struct fw_cmd_resp_str_info {
	enum fw_cmd_resp_str_status status;
	enum camera_port_id cid_owner;
	struct isp_gpu_mem_info *meta_info_buf[STREAM_META_BUF_COUNT];
	struct isp_gpu_mem_info *meta_data_buf[STREAM_META_BUF_COUNT];
	struct isp_gpu_mem_info *cmd_resp_buf;
};

#define ASYNC_INIT_THREAD_RUNING_TIMEOUT 60 /* milliseconds */
#define ASYNC_STARTSENSOR_THREAD_RUNING_TIMEOUT 400 /* milliseconds */

struct isp_async_cam_init_work_para {
	struct isp_context *isp;
	enum camera_port_id cid;
	u32 res_fps_idx;
	struct isp_event mem_pool_alloc_done;
	struct isp_event start_sensor;
	struct isp_event start_sensor_done;
};

struct isp_fw_cmd_pay_load_buf {
	uint64 sys_addr;
	uint64 mc_addr;
	struct isp_fw_cmd_pay_load_buf *next;
};

struct isp_fw_work_buf_mgr {
	u64 sys_base;
	u64 mc_base;
	u32 pay_load_pkg_size;
	u32 pay_load_num;
	struct mutex mutex; /* mutex */
	struct isp_fw_cmd_pay_load_buf *free_cmd_pl_list;
	struct isp_fw_cmd_pay_load_buf *used_cmd_pl_list;
};

struct isp_context {
	enum isp_status isp_status;
	struct mutex ops_mutex; /* ops_mutex */

	struct isp_pwr_unit isp_pu_isp;
	struct isp_pwr_unit isp_pu_dphy;
	struct isp_pwr_unit isp_pu_cam[CAMERA_PORT_MAX];
	u32 isp_fw_ver;

	struct isp_config_mode_param mode_param;
	struct isp_fw_work_buf_mgr fw_indirect_cmd_pl_buf_mgr;
	struct isp_gpu_mem_info fb_buf;
	struct isp_gpu_mem_info nfb_buf;

	struct fw_cmd_resp_str_info
		fw_cmd_resp_strs_info[FW_CMD_RESP_STREAM_ID_MAX];

	u64 fw_cmd_buf_sys[ISP_FW_CMD_BUF_COUNT];
	u64 fw_cmd_buf_mc[ISP_FW_CMD_BUF_COUNT];
	u32 fw_cmd_buf_size[ISP_FW_CMD_BUF_COUNT];
	u64 fw_resp_buf_sys[ISP_FW_RESP_BUF_COUNT];
	u64 fw_resp_buf_mc[ISP_FW_RESP_BUF_COUNT];
	u32 fw_resp_buf_size[ISP_FW_RESP_BUF_COUNT];
	u64 fw_log_sys;
	u64 fw_log_mc;
	u32 fw_log_size;

	struct isp_cmd_element *cmd_q;
	struct mutex cmd_q_mtx; /* cmd_q_mtx */

	u32 sensor_count;
	struct thread_handler async_init_thread[CAMERA_PORT_MAX];
	struct isp_async_cam_init_work_para async_cam_init_para[CAMERA_PORT_MAX];
	struct thread_handler fw_resp_thread[MAX_REAL_FW_RESP_STREAM_NUM];
	u64 irq_enable_id[MAX_REAL_FW_RESP_STREAM_NUM];

	struct thread_handler work_item_thread;

	struct mutex command_mutex; /* mutex to command */
	struct mutex response_mutex; /* mutex to retrieve response */
	struct mutex isp_semaphore_mutex; /* mutex to access isp semaphore */
	u32 isp_semaphore_acq_cnt; /* how many times the isp semaphore is acquired */

	u32 host2fw_seq_num;

	u32 reg_value;
	u32 fw2host_response_result;
	u32 fw2host_sync_response_payload[40];

	func_isp_module_cb evt_cb[CAMERA_PORT_MAX];
	void *evt_cb_context[CAMERA_PORT_MAX];
	void *fw_data;
	u32 fw_len;
	u32 sclk; /* In MHZ */
	u32 iclk; /* In MHZ */
	u32 xclk; /* In MHZ */
	u32 refclk; /* In MHZ */
	bool fw_ctrl_3a;
	bool clk_info_set_2_fw;
	bool snr_info_set_2_fw[CAMERA_PORT_MAX];
	bool req_fw_load_suc;
	struct mutex map_unmap_mutex; /* mutex for unmap */
	struct isp_sensor_info sensor_info[CAMERA_PORT_MAX];
	struct isphwip_version_info isphw_info;

	/* buffer to include code, stack, heap, bss, dmamem, log info */
	struct isp_gpu_mem_info *fw_running_buf;
	struct isp_gpu_mem_info *fw_cmd_resp_buf;
	struct isp_gpu_mem_info *indirect_cmd_payload_buf;
	u8 *fw_log_buf;
	u32 fw_log_buf_len;
	u32 prev_buf_cnt_sent;
	struct isp_gpu_mem_info *fw_mem_pool[CAMERA_PORT_MAX];
	u64 timestamp_fw_base;
	u64 timestamp_sw_prev;
	s64 timestamp_sw_base;

	void *isp_power_cb_context;

	bool fw_loaded; /* ISP FW is loaded */
	struct amd_cam *amd_cam;
};

struct isp_fw_resp_thread_para {
	u32 idx;
	struct isp_context *isp;
};

struct isp_context *ispm_get_isp_context(void);

void isp_reset_str_info(struct isp_context *isp, enum camera_port_id cid,
			enum stream_id sid);
void isp_reset_camera_info(struct isp_context *isp, enum camera_port_id cid);

s32 ispm_sw_init(struct isp_context *isp_context);
s32 ispm_sw_uninit(struct isp_context *isp_context);

void isp_init_fw_rb_log_buffer(struct isp_context *isp_context,
			       u32 fw_rb_log_base_lo, u32 fw_rb_log_base_hi,
			       u32 fw_rb_log_size);

bool isp_get_str_out_prop(struct isp_sensor_info *sen_info,
			  struct isp_stream_info *str_info,
			  struct image_prop_t *out_prop);

s32 isp_set_stream_data_fmt(struct isp_context *isp_context,
			    enum camera_port_id camera_id,
			    enum stream_id stream_type,
			    enum pvt_img_fmt img_fmt);
s32 isp_set_str_res_fps_pitch(struct isp_context *isp_context,
			      enum camera_port_id camera_id,
			      enum stream_id stream_type,
			      struct pvt_img_res_fps_pitch *value);

bool is_camera_started(struct isp_context *isp_context,
		       enum camera_port_id cam_id);

void isp_take_back_str_buf(struct isp_context *isp, struct isp_stream_info *str,
			   enum camera_port_id cid, enum stream_id sid);

struct isp_mapped_buf_info *isp_map_sys_2_mc(struct isp_context *isp,
					     struct sys_img_buf_info *buf,
					     u32 mc_align, u16 cam_id,
					     u16 stream_id, u32 y_len, u32 u_len,
					     u32 v_len);

void isp_unmap_sys_2_mc(struct isp_context *isp,
			struct isp_mapped_buf_info *buff);

struct isp_mapped_buf_info *
isp_map_sys_2_mc_multi(struct isp_context *isp,
		       struct sys_img_buf_info *sys_img_buf_hdl, u32 mc_align,
		       u16 cam_id, u16 stream_id, u32 y_len, u32 u_len, u32 v_len);
void isp_unmap_sys_2_mc_multi(struct isp_context *isp,
			      struct isp_mapped_buf_info *buff);

s32 isp_start_stream_from_idle(struct isp_context *isp_context,
			       enum camera_port_id cam_id, s32 pipe_id,
			       enum stream_id stream_id);
s32 isp_start_stream_from_busy(struct isp_context *isp_context,
			       enum camera_port_id cam_id, s32 pipe_id,
			       enum stream_id stream_id);
s32 isp_start_stream(struct isp_context *isp_context,
		     enum camera_port_id cam_id, enum stream_id stream_id);

s32 isp_setup_output(struct isp_context *isp, enum camera_port_id cid,
		     enum stream_id stream_id);

s32 isp_init_stream(struct isp_context *isp, enum camera_port_id cam_id,
		    enum fw_cmd_resp_stream_id fw_stream_id);

result_t isp_alloc_fw_drv_shared_buf(struct isp_context *isp,
				     enum camera_port_id cam_id,
				     enum fw_cmd_resp_stream_id fw_stream_id);
void isp_free_fw_drv_shared_buf(struct isp_context *isp,
				enum camera_port_id cam_id,
				enum fw_cmd_resp_stream_id fw_stream_id);
s32 isp_kickoff_stream(struct isp_context *isp, enum camera_port_id cam_id,
		       enum fw_cmd_resp_stream_id fw_stream_id, u32 w, u32 h);
s32 isp_uninit_stream(struct isp_context *isp, enum camera_port_id cam_id,
		      enum fw_cmd_resp_stream_id fw_stream_id);

struct sys_img_buf_info *sys_img_buf_handle_cpy(struct sys_img_buf_info *buf);
void sys_img_buf_handle_free(struct sys_img_buf_info *hdl);

void isp_init_fw_ring_buf(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id idx, u32 cmd);
void isp_get_cmd_buf_regs(enum fw_cmd_resp_stream_id idx, u32 *rreg, u32 *wreg,
			  u32 *baselo_reg, u32 *basehi_reg, u32 *size_reg);
void isp_get_resp_buf_regs(enum fw_cmd_resp_stream_id idx, u32 *rreg, u32 *wreg,
			   u32 *baselo_reg, u32 *basehi_reg, u32 *size_reg);

static inline void isp_split_addr64(u64 addr, u32 *lo, u32 *hi)
{
	if (lo)
		*lo = (u32)(addr & 0xffffffff);
	if (hi)
		*hi = (u32)(addr >> 32);
}

static inline u64 isp_join_addr64(u32 lo, u32 hi)
{
	return (((u64)hi) << 32) | (u64)lo;
}

static inline u32 isp_get_cmd_pl_size(void)
{
	return INDIRECT_BUF_SIZE;
}

static inline bool is_isp_poweron(struct isp_context *isp)
{
	if (isp->isp_pu_isp.pwr_status == ISP_PWR_UNIT_STATUS_ON)
		return true;
	else
		return false;
}

s32 isp_setup_stream(struct isp_context *isp, enum camera_port_id cid,
		     enum fw_cmd_resp_stream_id fw_stream_id);

s32 isp_setup_fw_mem_pool(struct isp_context *isp, enum camera_port_id cam_id,
			  enum fw_cmd_resp_stream_id fw_stream_id);
void isp_free_fw_mem_pool(struct isp_context *isp, enum camera_port_id cam_id);

enum fw_cmd_resp_stream_id isp_get_fw_stream_id(struct isp_context *isp,
						enum camera_port_id cid);

void isp_fw_resp_func(struct isp_context *isp,
		      enum fw_cmd_resp_stream_id fw_stream_id);

void isp_fw_resp_cmd_done(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id fw_stream_id,
			  struct resp_cmd_done *para);

void isp_fw_resp_cmd_done_extra(struct isp_context *isp,
				enum camera_port_id cid, struct resp_cmd_done *para,
				struct isp_cmd_element *ele);

void isp_fw_resp_cmd_skip_extra(struct isp_context *isp,
				enum camera_port_id cid, struct resp_cmd_done *para,
				struct isp_cmd_element *ele);

void isp_fw_resp_frame_done(struct isp_context *isp,
			    enum fw_cmd_resp_stream_id fw_stream_id,
			    struct resp_param_package_t *para);

void isp_clear_cmdq(struct isp_context *isp);
enum sensor_id isp_get_fw_sensor_id(struct isp_context *isp,
				    enum camera_port_id cid);
s32 isp_get_pipeline_id(struct isp_context *isp, enum camera_port_id cid);
result_t isp_set_stream_path(struct isp_context *isp, enum camera_port_id cid,
			     enum fw_cmd_resp_stream_id fw_stream_id);
result_t isp_send_meta_buf(struct isp_context *isp, enum camera_port_id cid,
			   enum fw_cmd_resp_stream_id fw_stream_id);

bool isp_semaphore_acquire(struct isp_context *isp);
void isp_semaphore_release(struct isp_context *isp);
bool isp_semaphore_acquire_one_try(struct isp_context *isp);

#endif
