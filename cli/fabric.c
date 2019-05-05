/*
 * Microsemi Switchtec(tm) PCIe Management Command Line Interface
 * Copyright (c) 2017, Microsemi Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "commands.h"
#include "argconfig.h"
#include "common.h"

#include <switchtec/switchtec.h>
#include <switchtec/portable.h>
#include <switchtec/fabric.h>

#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

static int gfms_bind(int argc, char **argv)
{
	const char *desc = "Unbind the EP(function) to the specified host";
	int ret;

	static struct {
		struct switchtec_dev *dev;
		struct switchtec_gfms_bind_req bind_req;
	} cfg ;

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"host_sw_idx", 's', "NUM", CFG_INT, &cfg.bind_req.host_sw_idx,
		 required_argument,"Host switch index", .require_in_usage = 1},
		{"phys_port_id", 'p', "NUM", CFG_INT,
		 &cfg.bind_req.host_phys_port_id,
		 required_argument,"Host physical port id",
		 .require_in_usage = 1},
		{"log_port_id", 'l', "NUM", CFG_INT,
		 &cfg.bind_req.host_log_port_id, required_argument,
		 "Host logical port id", .require_in_usage = 1},
		{"pdfid", 'f', "NUM", CFG_INT, &cfg.bind_req.pdfid,
		 required_argument,"Endpoint function's PDFID",
		 .require_in_usage = 1},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_gfms_bind(cfg.dev, &cfg.bind_req);
	if (ret) {
		switchtec_perror("gfms_bind");
		return ret;
	}

	return 0;
}

static int gfms_unbind(int argc, char **argv)
{
	const char *desc = "Unbind the EP(function) from the specified host";
	int ret;

	static struct {
		struct switchtec_dev *dev;
		struct switchtec_gfms_unbind_req unbind_req;
	} cfg;

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"host_sw_idx", 's', "NUM", CFG_INT,
		 &cfg.unbind_req.host_sw_idx, required_argument,
		 "Host switch index", .require_in_usage = 1,},
		{"phys_port_id", 'p', "NUM", CFG_INT,
		 &cfg.unbind_req.host_phys_port_id, required_argument,
		 .require_in_usage = 1, .help = "Host physical port id"},
		{"log_port_id", 'l', "NUM", CFG_INT,
		 &cfg.unbind_req.host_log_port_id, required_argument,
		 .require_in_usage = 1,.help = "Host logical port id"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_gfms_unbind(cfg.dev, &cfg.unbind_req);
	if (ret) {
		switchtec_perror("gfms_unbind");
		return ret;
	}

	return 0;
}

static int port_control(int argc, char **argv)
{
	const char *desc = "Initiate switchtec port control command";
	int ret;
	struct argconfig_choice control_type_choices[5] = {
		{"DISABLE", 0, "disable port"},
		{"ENABLE", 1, "enable port"},
		{"RETRAIN", 2, "link retrain"},
		{"HOT_RESET", 3, "link hot reset"},
		{}
	};
	struct argconfig_choice hot_reset_flag_choices[3] = {
		{"CLEAR", 0, "hot reset status clear"},
		{"SET", 1, "hot reset status set"},
		{}
	};

	static struct {
		struct switchtec_dev *dev;
		uint8_t control_type;
		uint8_t phys_port_id;
		uint8_t hot_reset_flag;
	} cfg;

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"control_type", 't', "TYPE", CFG_MULT_CHOICES, &cfg.control_type, required_argument,
		.choices=control_type_choices,
		.require_in_usage = 1,
		.help="Port control type"},
		{"phys_port_id", 'p', "NUM", CFG_INT, &cfg.phys_port_id, required_argument,"Physical port ID",
		.require_in_usage = 1,},
		{"hot_reset_flag", 'f', "FLAG", CFG_MULT_CHOICES, &cfg.hot_reset_flag, required_argument,
		.choices=hot_reset_flag_choices,
		.require_in_usage = 1,
		.help="Hot reset flag option"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_port_control(cfg.dev, cfg.control_type, cfg.phys_port_id, cfg.hot_reset_flag);
	if (ret) {
		switchtec_perror("port_control");
		return ret;
	}

	return 0;
}

static const char * const port_type_strs[] = {
	"Unused",
	"Fabric Link",
	"Fabric EP",
	"Fabric Host",
	"Invalid",
};

static const char * const clock_mode_strs[] = {
	"Common clock without SSC",
	"Non-common clock without SSC (SRNS)",
	"Common clock with SSC",
	"Non-common clock with SSC (SRIS)",
	"Invalid",
};

static int portcfg_set(int argc, char **argv)
{
	const char *desc = "Set the port config";
	int ret;

	struct argconfig_choice port_type_choices[4] = {
		{"UNUSED", 0,
		 port_type_strs[SWITCHTEC_FAB_PORT_TYPE_UNUSED]},
		{"FABRIC_EP", 2,
		 port_type_strs[SWITCHTEC_FAB_PORT_TYPE_FABRIC_EP]},
		{"FABRIC_HOST", 3,
		 port_type_strs[SWITCHTEC_FAB_PORT_TYPE_FABRIC_HOST]},
		{}
	};
	struct argconfig_choice clock_mode_choices[5] = {
		{"COMMON", 0,
		 clock_mode_strs[SWITCHTEC_FAB_PORT_CLOCK_COMMON_WO_SSC]},
		{"SRNS", 1,
		 clock_mode_strs[SWITCHTEC_FAB_PORT_CLOCK_NON_COMMON_WO_SSC]},
		{"COMMON_SSC", 2,
		 clock_mode_strs[SWITCHTEC_FAB_PORT_CLOCK_COMMON_W_SSC]},
		{"SRIS", 3,
		 clock_mode_strs[SWITCHTEC_FAB_PORT_CLOCK_NON_COMMON_W_SSC]},
		{}
	};

	static struct {
		struct switchtec_dev *dev;
		uint8_t phys_port_id;
		struct switchtec_fab_port_config port_cfg;
	} cfg;

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"phys_port_id", 'p', "NUM", CFG_INT,
		 &cfg.phys_port_id, required_argument,
		 "physical port id", .require_in_usage = 1},
		{"port_type", 't', "TYPE", CFG_MULT_CHOICES,
		 &cfg.port_cfg.port_type, required_argument,
		.choices=port_type_choices, .require_in_usage = 1,
		.help="Port type"},
		{"clock_source", 'c', "NUM", CFG_INT,
		 &cfg.port_cfg.clock_source, required_argument,
		 "CSU channel index for port clock source",
		 .require_in_usage = 1},
		{"clock_mode", 'm', "TYPE", CFG_MULT_CHOICES,
		 &cfg.port_cfg.clock_mode, required_argument,
		 .choices=clock_mode_choices, .require_in_usage = 1,
		 .help="Clock mode"},
		{"hvd_id", 'd', "NUM", CFG_INT, &cfg.port_cfg.hvd_inst,
		 required_argument, "HVM domain index for USP",
		 .require_in_usage = 1},
		{NULL}
	};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_fab_port_config_set(cfg.dev, cfg.phys_port_id, &cfg.port_cfg);
	if (ret) {
		switchtec_perror("port_config");
		return ret;
	}

	return 0;
}

static int portcfg_show(int argc, char **argv)
{
	const char *desc = "Get the port config info";
	int ret;
	struct switchtec_fab_port_config port_info;
	int port_type, clock_mode;

	static struct {
		struct switchtec_dev *dev;
		int phys_port_id;
	} cfg = {
		.phys_port_id = -1,
	};

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"phys_port_id", 'p', "NUM", CFG_NONNEGATIVE, &cfg.phys_port_id,
		 required_argument,"physical port id", .require_in_usage = 1},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (cfg.phys_port_id == -1) {
		argconfig_print_usage(opts);
		return 1;
	}

	ret = switchtec_fab_port_config_get(cfg.dev, cfg.phys_port_id, &port_info);
	if (ret) {
		switchtec_perror("port_info");
		return ret;
	}

	port_type = port_info.port_type;
	if(port_type >= SWITCHTEC_FAB_PORT_TYPE_INVALID)
		port_type = SWITCHTEC_FAB_PORT_TYPE_INVALID;

	printf("Port Type:    %s \n", port_type_strs[port_type]);
	printf("Clock Source: %d\n", port_info.clock_source);

	clock_mode = port_info.clock_mode;
	if(clock_mode >= SWITCHTEC_FAB_PORT_CLOCK_INVALID)
		clock_mode = SWITCHTEC_FAB_PORT_CLOCK_INVALID;

	printf("Clock Mode:   %s\n", clock_mode_strs[clock_mode]);
	printf("Hvd Instance: %d\n", port_info.hvd_inst);

	return 0;
}

static const struct cmd commands[] = {
	{"gfms_bind", gfms_bind, "Bind the EP(function) to the specified host"},
	{"gfms_unbind", gfms_unbind, "Unbind the EP(function) from the specified host"},
	{"port_control", port_control, "Initiate port control command"},
	{"portcfg_show", portcfg_show, "Get the port config info"},
	{"portcfg_set", portcfg_set, "Set the port config"},
	{}
};

static struct subcommand subcmd = {
	.name = "fabric",
	.cmds = commands,
	.desc = "Switchtec Fabric Management (PAX only)",
	.long_desc = "",
};

REGISTER_SUBCMD(subcmd);