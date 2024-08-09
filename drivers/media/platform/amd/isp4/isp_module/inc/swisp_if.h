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

#ifndef _SWISP_IF_H
#define _SWISP_IF_H

#define SWISP_IF_VERSION_1   0x0100

enum swisp_result {
	/* STATUS_SUCCESS */
	SWISP_RESULT_OK                                  = 0,
	/* some unknown error happen */
	SWISP_RESULT_ERROR_GENERIC                       = 1,
	/* input or output parameter error */
	SWISP_RESULT_ERROR_INVALIDPARAMS                 = 2,
	/* service not available yet */
	SWISP_RESULT_ERROR_FUNCTION_NOT_SUPPORT          = 3,
	/* wait operation time out */
	SWISP_RESULT__ERROR_TIMEOUT                      = 4
};

enum irq_source_isp {
	IRQ_SOURCE_ISP_RINGBUFFER_BASE9_CHANGED  = 1,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE10_CHANGED = 2,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE11_CHANGED = 3,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE12_CHANGED = 4,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE13_CHANGED = 5,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE14_CHANGED = 6,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE15_CHANGED = 7,
	IRQ_SOURCE_ISP_RINGBUFFER_BASE16_CHANGED = 8,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT9           = 9,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT10          = 10,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT11          = 11,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT12          = 12,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT13          = 13,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT14          = 14,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT15          = 15,
	IRQ_SOURCE_ISP_RINGBUFFER_WPT16          = 16,
	IRQ_SOURCE_ISP_END_OF_LIST
};

enum isp_gpu_mem_type {
	ISP_GPU_MEMORY_TYPE_VISIBLE_FB,
	ISP_GPU_MEMORY_TYPE_INVISIBLE_FB,
	ISP_GPU_MEMORY_TYPE_GART_CACHEABLE,
	ISP_GPU_MEM_TYPE_NLFB = ISP_GPU_MEMORY_TYPE_GART_CACHEABLE,
	ISP_GPU_MEMORY_TYPE_GART_WRITECOMBINE
};

struct isp_gpu_mem_allocation_flag {
	/* CPU virtual address */
	u32   cpu_virtual_address              : 1;
	/* CPU physical address */
	u32   sys_physical_address             : 1;
	/* Memory needs save/restore due to power loss event */
	u32   save_restore                     : 1;
	/* Reserved bits */
	u32   reserved                         : 29;
};

struct isp_gpu_mem_info {
	u32	mem_domain;
	u64	mem_size;
	u32	mem_align;
	u64	gpu_mc_addr;
	void	*sys_addr;
	void	*mem_handle;
};

struct isphwip_version_info {
	u32 major;
	u32 minor;
	u32 revision;
	u32 variant;
};

struct swisp_isp_info {
	/* HW component version */
	struct isphwip_version_info hc_version;
	/* reserved 4k for possible future extension */
	u32                         resv[1024];
};

struct isp_allocate_gpu_memory_input {
	/* in: indicate the memory type requested */
	enum isp_gpu_mem_type	            memory_type;
	/* required memory size in byte */
	u64                                 mem_size;
	/* required memory allocation alignment in unit byte */
	u32                                 alignment;
	/* gpu memory allocation flag */
	struct isp_gpu_mem_allocation_flag  gpu_memory_allocate_flag;
};

struct isp_allocate_gpu_memory_output {
	/* GPU MC address for the allocate memory block */
	u64   gpu_mc_addr;
	/* CPU virtual address for the allocated memory block */
	void  *mem_block_ptr;
	/* allocated memory size */
	u64   allocated_memsize;
	/* system physical address for the allocate memory block */
	u64   system_physical_addr;
	/* memory block handle of the allocated memory block */
	void  *mem_handle;
};

/* The following definition is for isp_ReleaseGPUMemory */
struct isp_release_gpu_memory_input {
	/* indicate the memory type requested */
	enum isp_gpu_mem_type memory_type;
	/* required memory size in byte */
	u64                   em_size;
	/* GPU virtual address for the allocate memory block */
	u64                   gpu_mc_addr;
	/* CPU virtual address of the allocated memory block */
	void                  *mem_block_ptr;
	/* memory block handle of the allocated memory block */
	void                  *mem_handle;
};

/* The following definition is for isp_MapToGartSpace */
struct isp_map_virtual_to_gart_input {
	/* CPU virtual address of the allocated memory block */
	void                  *mem_block_ptr;
	/* in: indicate the memory type requested */
	enum isp_gpu_mem_type memory_type;
	/* required memory size in byte */
	u64                   mem_size;
	/* required gpu_mc_addr alignment in unit byte */
	u32                   alignment;
	/* COS handle of the allocated memory block */
	void                  *cos_mem_handle;
};

struct isp_map_virtual_to_gart_output {
	/* mapped gpu virtual (MC) address */
	u64  gpu_mc_addr;
	/* CGS handle of the mapped memory block */
	void *isp_map_handle;
};

/* The following definition is for CGS_gpu_memory_copy */
struct isp_gpu_mem_copy_input {
	/* mc address of copy source */
	u64                   mc_src;

	/* mc address of copy destination */
	u64                   mc_dest;

	/* copy size in byte */
	u64                   size;

};

/* The following definition is for isp_UnmapFromGartSpace */
struct isp_unmap_virtual_from_gart_input {
	/* mapped gpu virtual (MC) address */
	u64    gpu_mc_addr;
	/* memory block handle of the allocated memory block */
	void   *map_handle;
};

/* The following definition is for isp_pm_request_min_clk */
struct isp_pm_req_min_clk_input {
	u32   sclk : 1;
	u32   iclk : 1;
	u32   xclk : 1;
	u32   reserved : 29;
	/* minimum sclk/soclk, in the unit of 10k */
	u32   min_sclk;
	/* minimum iclk/isp clock, in the unit of 10k */
	u32   min_iclk;
	/* minimum xclk, in the unit of 10k */
	u32   min_xclk;
	/* reserved */
	u32   clk_reserved[4];
};

/* The following definition is for isp_pm_request_power */
enum  isp_pm_req_pwr_id {
	/* ISP power */
	ISP_PM_REQ_PWR__ISP,
	/* VCN power */
	ISP_PM_REQ_PWR__VCN
};

struct isp_pm_req_pwr_input {
	/* power settings to be powered up/down */
	u32 power_up;
	/* power of tile X */
	u32 tile_x : 1;
	/* power of tile M */
	u32 tile_m : 1;
	/* power of tile core */
	u32 tile_core : 1;
	/* power of tile pre */
	u32 tile_pre : 1;
	/* power of tile post */
	u32 tile_post : 1;
	/* disable mmhub power gating */
	u32 disable_mmhub_pg: 1;
	/* power of tile PDP */
	u32 tile_pdp : 1;
	/* power of tile CSTAT */
	u32 tile_cstat : 1;
	/* power of tile LME */
	u32 tile_lme : 1;
	/* power of tile BYRP */
	u32 tile_byrp : 1;
	/* power of tile GRBP */
	u32 tile_grbp : 1;
	/* power of tile MCFP */
	u32 tile_mcfp : 1;
	/* power of tile YUVP */
	u32 tile_yuvp : 1;
	/* power of tile MCSC */
	u32 tile_mcsc : 1;
	/* power of tile GDC */
	u32 tile_gdc : 1;
	/* Reserved bits */
	u32 reserved : 17;
};

/* The following definition is for isp_pm_request_power_and_min_clk */
struct isp_pm_req_power_and_min_clk_input {
	u32	sclk : 1;
	u32	iclk : 1;
	u32	xclk : 1;
	u32	reserved : 29;
	/* minimum sclk/soclk, in the unit of 10k */
	u32	min_sclk;
	/* minimum iclk/isp clock, in the unit of 10k */
	u32	min_iclk;
	/* minimum xclk, in the unit of 10k */
	u32	min_xclk;
	/* reserved */
	u32	clk_reserved1[4];
	/* power settings to be powered up/down */
	u32	power_up;
	/* power of tile A */
	u32	tile_a : 1;
	/* power of tile B */
	u32	tile_b : 1;
	/* power of tile X */
	u32	tile_x : 1;
	/* power of tile M */
	u32	tile_m : 1;
	/* power of tile core */
	u32	tile_core : 1;
	/* power of tile pre */
	u32	tile_pre : 1;
	/* power of tile post */
	u32	tile_post : 1;
	/* disable mmhub power gating */
	u32	disable_mmhub_pg: 1;
	/* power of tile PDP */
	u32     tile_pdp : 1;
	/* power of tile CSTAT */
	u32     tile_cstat : 1;
	/* power of tile LME */
	u32     tile_lme : 1;
	/* power of tile BYRP */
	u32     tile_byrp : 1;
	/* power of tile GRBP */
	u32     tile_grbp : 1;
	/* power of tile MCFP */
	u32     tile_mcfp : 1;
	/* power of tile YUVP */
	u32     tile_yuvp : 1;
	/* power of tile MCSC */
	u32     tile_mcsc : 1;
	/* power of tile GDC */
	u32     tile_gdc : 1;
	/* Reserved bits */
	u32	reserved2 : 15;
};

/* The following definition is for isp_pm_request_query_actual_clock */
enum  isp_pm_query_actual_clk_id {
	/* Soc clock */
	ISP_PM_REQ_ACTUAL_CLK_SCLK,
	/* ISP clock */
	ISP_PM_REQ_ACTUAL_CLK_ICLK,
	/* X clock */
	ISP_PM_REQ_ACTUAL_CLK_XCLK
};

enum isp_interrupt_callback_priority {
	ISP_INTERRUPT_CALLBACK_PRIORITY_DEFAULT,
	ISP_INTERRUPT_CALLBACK_PRIORITY_LOW_PRIORITY,
	ISP_INTERRUPT_CALLBACK_PRIORITY_MEDIUM_PRIORITY,
	ISP_INTERRUPT_CALLBACK_PRIORITY_HIGH_PRIORITY
};

struct isp_interrupt_callback_flag {
	/* one time callback interrupt */
	u32    one_time_callback : 1;
	/* Reserved bits */
	u32    reserved		 : 31;
};

struct isp_interrupt_callback_info {
	u32  irq_source;
	/* Handle used at callback registration time which identifies the
	 *grouping of irq_source.
	 */
	void *irq_processor_handle;
	u32  irq_data_type;
	u32  irq_data[8];
};

typedef void (*isp_callback_func)(void *context,
				  struct isp_interrupt_callback_info *);

struct isp_register_interrupt_input {
	/* indicating which irq source to access, from enum irq_source_isp */
	u32                                     irq_source;
	/* indicating action to take for interrupt */
	struct isp_interrupt_callback_flag      flag;
	/*
	 * Call back function pointer
	 * actually it is isp_callback_func
	 */
	void                                    *callback_func;
	/* call back context */
	void                                    *callback_context;
	/* optional,there are 4 possible level */
	enum isp_interrupt_callback_priority     callback_priority;
};

struct isp_register_interrupt_output {
	/* return token from CGS for register and the token for unregister */
	u64        irq_enable_id;
};

/* The following definition is for isp_Unregister_Interrupt_Callback */
struct isp_unregister_interrupt_input {
	/* return token from CGS for register and the token for unregister */
	u64        irq_enable_id;
	/* indicating which irq source to access */
	u32        irq_source;
};

/* The following definition is for isp_acpi_method */
enum isp_acpi_datatype {
	ISP_ACPI_DATATYPE__INTEGER = 1,
	ISP_ACPI_DATATYPE__STRING,
	ISP_ACPI_DATATYPE__BUFFER,
	ISP_ACPI_DATATYPE__PACKAGE,
};

enum isp_acpi_target {
	ISP_ACPI_TARGET_DISPLAY_ADAPTER,
	ISP_ACPI_TARGET_CHILD_DEVICE_CAMERA_0,
	ISP_ACPI_TARGET_CHILD_DEVICE_CAMERA_1
};

struct isp_acpi_method_argument {
	/* Data type of the argument */
	enum isp_acpi_datatype		datatype;
	/* data length of input / output data of an ACPI control method */
	u32			        acpi_method_data_length;
	/*data length of input/output data of a function of an ACPI
	 *ctrl method
	 */
	u32			        acpi_function_data_length;
	union {
		/* argument value if the argument is an integer */
		u32	argument;
		/* pointer to the argument buffer if the argument is not
		 * an integer
		 */
		void		*buffer;
	} arg;
};

struct isp_acpi_method_input {
	/* ACPI method name */
	u32			                    method_name;
	/* count for the input argument */
	u32				            input_argument_count;
	/* pointer to the input argument buffer */
	struct isp_acpi_method_argument             *input_argument;
	/* specify ACPI target, It maybe display adapter or child device */
	enum isp_acpi_target                        acpi_target;
};

struct isp_acpi_method_output {
	/* count for the output argument */
	u32				            output_argument_count;
	/* count for reported output argument */
	u32				            valid_outputarg_count;
	/* pointer to the output argument buffer */
	struct isp_acpi_method_argument             *output_argument;
};

struct isp_load_fw_input {
	u8 *img_addr;
	u32 size;
};

struct sw_isp_if {
	/*
	 * the interface size;
	 */
	u16 size;
	/*
	 * the interface version, its value will be (version_high<<16)
	 * version_low, so the current version 1.0 will be (1<<16)|0
	 */
	u16 version;

	/*
	 * the context of function call, it should be the first parameter of all
	 * function call in this interface.
	 */
	void *context;

	/*
	 * dynamic gpu memory allocation is forbidden out of ip initialization
	 * in KMD, so NULL them to find possible violation
	 */
	enum swisp_result (*alloc_gpumem)(void *sw_isp,
					  struct isp_allocate_gpu_memory_input
					  *memory_input,
					  struct isp_allocate_gpu_memory_output
					  *memory_output);
	enum swisp_result (*release_gpumem)(void *sw_isp,
					    struct isp_release_gpu_memory_input
					    *memory_in);
	enum swisp_result (*map_virt_to_gart)(void *sw_isp,
					      struct
					      isp_map_virtual_to_gart_input
					      * map_gart_input,
					      struct
					      isp_map_virtual_to_gart_output
					      * map_gart_output);
	enum swisp_result (*unmap_virt_to_gart)(void *sw_isp,
						struct isp_unmap_virtual_from_gart_input
						*unmap_gart_input);
	enum swisp_result (*gpu_memcpy)(void *sw_isp,
					struct isp_gpu_mem_copy_input
					*gpu_memory_copy_input);
	enum swisp_result (*req_clk)(void    *sw_isp,
				     struct isp_pm_req_min_clk_input
				     *min_clk_input);
	enum swisp_result (*req_pwr)(void *sw_isp,
				     struct isp_pm_req_pwr_input *pwr_input);
	enum swisp_result (*req_pwr_clk)(void *sw_isp,
					 struct
					 isp_pm_req_power_and_min_clk_input
					 * pwr_clk_input);
	enum swisp_result (*query_clk)(void *sw_isp,
				       enum isp_pm_query_actual_clk_id  clk_id,
				       /* actual clock used, in the unit of 10k */
				       u32 *actual_clk);
	enum swisp_result (*reg_intr)(void *sw_isp,
				      struct isp_register_interrupt_input
				      *register_intr_input,
				      struct isp_register_interrupt_output
				      *register_intr_output);
	enum swisp_result (*unreg_intr)(void *sw_isp,
					struct isp_unregister_interrupt_input
					*unregister_intr_input);
	enum swisp_result (*acpi_method)(void *sw_isp,
					 struct isp_acpi_method_input
					 *acpi_method_input,
					 struct isp_acpi_method_output
					 *acpi_method_output);
	enum swisp_result (*load_firmware)(void *sw_isp,
					   struct isp_load_fw_input *fw_input);
	u32 (*read_reg)(void *sw_isp, u32 offset);
	void (*write_reg)(void *sw_isp, u32 offset, u32 value);
	u32 (*indirect_read_reg)(void *sw_isp, u32 offset);
	void (*indirect_write_reg)(void *sw_isp, u32 offset, u32 value);
	enum swisp_result (*get_info)(void *context,
				      struct swisp_isp_info *info);
};

#endif
