/* arch/arm/mach-msm/include/mach/msm_fb.h
 *
 * Internal shared definitions for various MSM framebuffer parts.
 *
 * Copyright (C) 2007 Google Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MSM_FB_H_
#define _MSM_FB_H_

#include <linux/device.h>

struct mddi_info;

/* output interface format */
#define MSM_MDP_OUT_IF_FMT_RGB565 0
#define MSM_MDP_OUT_IF_FMT_RGB666 1
#define MSM_MDP_OUT_IF_FMT_RGB888 2

/* mdp override operations */
#define MSM_MDP_PANEL_IGNORE_PIXEL_DATA		(1 << 0)
#define MSM_MDP_PANEL_FLIP_UD			(1 << 1)
#define MSM_MDP_PANEL_FLIP_LR			(1 << 2)
#define MSM_MDP4_MDDI_DMA_SWITCH		(1 << 3)

/* mddi type */
#define MSM_MDP_MDDI_TYPE_I	 0
#define MSM_MDP_MDDI_TYPE_II	 1

/* lcdc override operations */
#define MSM_MDP_LCDC_DMA_PACK_ALIGN_LSB		(1 << 0)

struct msm_fb_data {
	int xres;	/* x resolution in pixels */
	int yres;	/* y resolution in pixels */
	int width;	/* disply width in mm */
	int height;	/* display height in mm */
	unsigned output_format;
};

struct msmfb_callback {
	void (*func)(struct msmfb_callback *);
};

enum {
	MSM_MDDI_PMDH_INTERFACE = 0,
	MSM_MDDI_EMDH_INTERFACE,
	MSM_EBI2_INTERFACE,
	MSM_LCDC_INTERFACE,
	MSM_TV_INTERFACE,

	MSM_MDP_NUM_INTERFACES = MSM_TV_INTERFACE + 1
};

#define MSMFB_CAP_PARTIAL_UPDATES	(1 << 0)
#define MSMFB_CAP_CABC			(1 << 1)



struct msm_panel_data {
	/* turns off the fb memory */
	int (*suspend)(struct msm_panel_data *);
	/* turns on the fb memory */
	int (*resume)(struct msm_panel_data *);
	/* turns off the panel */
	int (*blank)(struct msm_panel_data *);
	/* turns on the panel */
	int (*unblank)(struct msm_panel_data *);
	/* for msmfb shutdown() */
	int (*shutdown)(struct msm_panel_data *);
	void (*wait_vsync)(struct msm_panel_data *);
	void (*request_vsync)(struct msm_panel_data *, struct msmfb_callback *);
	void (*clear_vsync)(struct msm_panel_data *);
	void (*dump_vsync)(void);
	/* change timing on the fly */
	//int (*adjust_timing)(struct msm_panel_data *, struct msm_lcdc_timing *,
	//		u32 xres, u32 yres);
	/* from the enum above */
	unsigned interface_type;
	/* data to be passed to the fb driver */
	struct msm_fb_data *fb_data;

	/* capabilities supported by the panel */
	uint32_t caps;
	struct {
		int left;
		int top;
		int eright; /* exclusive */
		int ebottom; /* exclusive */
	} update_info;
};

enum {
	MDP_DMA_P = 0,
	MDP_DMA_S,
};

struct msm_mdp_platform_data {
	/* from the enum above */
	int dma_channel;
	unsigned overrides;
	unsigned color_format;
	int tearing_check;
	unsigned sync_config;
	unsigned sync_thresh;
	unsigned sync_start_pos;
};

struct msm_mddi_client_data {
	void (*suspend)(struct msm_mddi_client_data *);
	void (*resume)(struct msm_mddi_client_data *);
	void (*activate_link)(struct msm_mddi_client_data *);
	void (*remote_write)(struct msm_mddi_client_data *, uint32_t val,
			     uint32_t reg);
	void (*remote_write_vals)(struct msm_mddi_client_data *, uint8_t * val,
			     uint32_t reg, unsigned int nr_bytes);
	uint32_t (*remote_read)(struct msm_mddi_client_data *, uint32_t reg);
	void (*auto_hibernate)(struct msm_mddi_client_data *, int);
	/* custom data that needs to be passed from the board file to a 
	 * particular client */
	void *private_client_data;
	struct resource *fb_resource;
	/* from the list above */
	unsigned interface_type;
};

struct msm_mddi_platform_data {
	unsigned int clk_rate;
	void (*power_client)(struct msm_mddi_client_data *, int on);

	/* fixup the mfr name, product id */
	void (*fixup)(uint16_t *mfr_name, uint16_t *product_id);

	int vsync_irq;

	struct resource *fb_resource; /*optional*/
	/* number of clients in the list that follows */
	int num_clients;
	unsigned type;
	/* array of client information of clients */
	struct {
		unsigned product_id; /* mfr id in top 16 bits, product id
				      * in lower 16 bits
				      */
		char *name;	/* the device name will be the platform
				 * device name registered for the client,
				 * it should match the name of the associated
				 * driver
				 */
		unsigned id;	/* id for mddi client device node, will also
				 * be used as device id of panel devices, if
				 * the client device will have multiple panels
				 * space must be left here for them
				 */
		void *client_data;	/* required private client data */
		unsigned int clk_rate;	/* optional: if the client requires a
					* different mddi clk rate
					*/
	} client_platform_data[];
};

struct msm_lcdc_timing {
	unsigned int clk_rate;		/* dclk freq */
	unsigned int hsync_pulse_width;	/* in dclks */
	unsigned int hsync_back_porch;	/* in dclks */
	unsigned int hsync_front_porch;	/* in dclks */
	unsigned int hsync_skew;	/* in dclks */
	unsigned int vsync_pulse_width;	/* in lines */
	unsigned int vsync_back_porch;	/* in lines */
	unsigned int vsync_front_porch;	/* in lines */

	/* control signal polarity */
	unsigned int vsync_act_low:1;
	unsigned int hsync_act_low:1;
	unsigned int den_act_low:1;
};

struct msm_lcdc_panel_ops {
	int	(*init)(struct msm_lcdc_panel_ops *);
	int	(*uninit)(struct msm_lcdc_panel_ops *);
	int	(*blank)(struct msm_lcdc_panel_ops *);
	int	(*unblank)(struct msm_lcdc_panel_ops *);
};

struct msm_lcdc_platform_data {
	struct msm_lcdc_panel_ops	*panel_ops;
	struct msm_lcdc_timing		*timing;
	int				fb_id;
	struct msm_fb_data		*fb_data;
	struct resource			*fb_resource;
	unsigned			 overrides;
};

struct msm_tvenc_platform_data {
	struct msm_tvenc_panel_ops	*panel_ops;
	int				fb_id;
	struct msm_fb_data		*fb_data;
	struct resource			*fb_resource;
	int (*video_relay)(int on_off);
};

struct mdp_blit_req;
struct fb_info;
struct mdp_device {
	struct device dev;
	void (*dma)(struct mdp_device *mdp, uint32_t addr,
		    uint32_t stride, uint32_t w, uint32_t h, uint32_t x,
		    uint32_t y, struct msmfb_callback *callback, int interface);
	void (*dma_wait)(struct mdp_device *mdp, int interface);
	int (*blit)(struct mdp_device *mdp, struct fb_info *fb,
		    struct mdp_blit_req *req);
#ifdef CONFIG_FB_MSM_OVERLAY
	int (*overlay_get)(struct mdp_device *mdp, struct fb_info *fb,
		    struct mdp_overlay *req);
	int (*overlay_set)(struct mdp_device *mdp, struct fb_info *fb,
		    struct mdp_overlay *req);
	int (*overlay_unset)(struct mdp_device *mdp, struct fb_info *fb,
		    int ndx);
	int (*overlay_play)(struct mdp_device *mdp, struct fb_info *fb,
		    struct msmfb_overlay_data *req, struct file *p_src_file);
#endif
	void (*set_grp_disp)(struct mdp_device *mdp, uint32_t disp_id);
	void (*configure_dma)(struct mdp_device *mdp);
	int (*check_output_format)(struct mdp_device *mdp, int bpp);
	int (*set_output_format)(struct mdp_device *mdp, int bpp);
	unsigned color_format;
	unsigned overrides;
};

struct class_interface;
int register_mdp_client(struct class_interface *class_intf);

/**** private client data structs go below this line ***/

struct msm_mddi_bridge_platform_data {
	/* from board file */
	int (*init)(struct msm_mddi_bridge_platform_data *,
		    struct msm_mddi_client_data *);
	int (*uninit)(struct msm_mddi_bridge_platform_data *,
		      struct msm_mddi_client_data *);
	/* passed to panel for use by the fb driver */
	int (*blank)(struct msm_mddi_bridge_platform_data *,
		     struct msm_mddi_client_data *);
	int (*unblank)(struct msm_mddi_bridge_platform_data *,
		       struct msm_mddi_client_data *);
	int (*shutdown)(struct msm_mddi_bridge_platform_data *,
		       struct msm_mddi_client_data *);
	struct msm_fb_data fb_data;

	/* board file will identify what capabilities the panel supports */
	uint32_t panel_caps;
};

/********** Support for pre-dma callbacks **********/

struct msmfb_update_area {
	int x;
	int y;
	int width;
	int height;
};

typedef void (*msmfb_predma_callback)(struct fb_info *fb, struct msmfb_update_area *area);

/*
 * Registers a pre-dma callback. Callbacks are called just before the driver
 * initiates a dma when a pan_update has been requested. The callback occurs
 * within a spinlock. Also note that the order in which the callbacks occur
 * is not fixed.
 *
 * Returns 0 when successful, -1 when max callbacks has been reached
 */
int msmfb_predma_register_callback(msmfb_predma_callback cb);

/*
 * Unregisters a pre-dma callback.
 *
 * Returns 0 when successful, -1 when not found
 */
int msmfb_predma_unregister_callback(msmfb_predma_callback cb);

/********** END OF Support for pre-dma callbacks **********/

#endif
