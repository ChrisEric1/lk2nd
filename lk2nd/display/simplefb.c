// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, Nikita Travkin <nikita@trvn.ru> */

#include <stdlib.h>
#include <string.h>
#include <dev/fbcon.h>
#include <boot.h>
#include <libfdt.h>

#include <lk2nd/util/cmdline.h>

/*
 * simplefb.c - Pass lk2nd's framebuffer to the OS.
 *
 * It might be useful to pass already enabled framebuffer to
 * the next OS. This module creates/fills simple-framebffer
 * node if this is requested on the cmdline.
 */

static int lk2nd_simplefb_dt_update(void *dtb, const char *cmdline,
				    enum boot_type boot_type)
{
	struct fbcon_config *fb = fbcon_display();
	int ret, chosen_offset, offset;
	char tmp[32];

	if (boot_type & (BOOT_DOWNSTREAM | BOOT_LK2ND))
		return 0;

	if (!lk2nd_cmdline_scan(cmdline, "lk2nd.pass-simplefb"))
		return 0;

	chosen_offset = fdt_path_offset(dtb, "/chosen");
	if (chosen_offset < 0)
		return 0;

	offset = fdt_node_offset_by_compatible(dtb, -1, "simple-framebuffer");
	if (offset < 0 && offset != -FDT_ERR_NOTFOUND)
		return 0;

	if (offset == -FDT_ERR_NOTFOUND) {
		snprintf(tmp, sizeof(tmp), "framebuffer@%08x", (uint32_t)fb->base);
		offset = fdt_add_subnode(dtb, chosen_offset, tmp);
		if (offset < 0)
			return 0;

		ret = fdt_setprop_string(dtb, offset, "compatible", "simple-framebuffer");
		if (ret < 0)
			return 0;
	}

	ret = fdt_setprop_empty(dtb, offset, "reg");
	if (ret < 0)
		return 0;

	ret = fdt_appendprop_addrrange(dtb, chosen_offset, offset, "reg",
				       (uint32_t)fb->base, (fb->stride * fb->height));
	if (ret < 0)
		return 0;

	ret = fdt_setprop_u32(dtb, offset, "width", fb->width);
	if (ret < 0)
		return 0;

	ret = fdt_setprop_u32(dtb, offset, "height", fb->height);
	if (ret < 0)
		return 0;

	ret = fdt_setprop_u32(dtb, offset, "stride", fb->stride);
	if (ret < 0)
		return 0;

	switch (fb->format) {
		case FB_FORMAT_RGB565:
			ret = fdt_setprop_string(dtb, offset, "format", "r5g6b5");
			break;

		case FB_FORMAT_RGB888:
			ret = fdt_setprop_string(dtb, offset, "format", "r8g8b8");
			break;

		default:
			return 0;
	}
	if (ret < 0)
		return 0;

	ret = fdt_setprop_string(dtb, offset, "status", "okay");
	if (ret < 0)
		return 0;

	return 0;
}
DEV_TREE_UPDATE(lk2nd_simplefb_dt_update);
