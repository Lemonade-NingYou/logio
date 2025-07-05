/*
 * Copyright (C) 2025 lemonade_NingYou
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "../include/logio.h"

static int create_directory(const char *dir)
{
	if (mkdir(dir, 0755) == 0)
	{
		printf("The folder' %s' was created successfully! \n", dir);
		return EXIT_SUCCESS;
	}
	else if (errno == EEXIST)
	{ // 目录已存在 
		printf("The folder '%s' already exists\n", dir);
		return EXIT_SUCCESS;
	}
	else
	{
		perror("Failed to create folder.\n");
		return EXIT_FAILURE;
	}
}

LogInfo log_ini(LogInitParams params)
{
	// 创建目录
	if (create_directory(params.FoldName) != EXIT_SUCCESS)
	{
		exit(EXIT_FAILURE);
	}

	// 准备变量
	start = clock(); // 记录程序开始时间

	char FmtFoldName[256];	// 存储格式化后的文件夹名称
	char FmtTime[99];		// 存储格式化后的时间
	char CompletePath[512]; // 存储完整的文件路径

	// 格式化文件夹名称（简单复制）
	snprintf(FmtFoldName, sizeof(FmtFoldName), "%s", params.FoldName);

	// 获取并格式化当前时间
	time_t now = time(NULL);
	struct tm *local_time = localtime(&now);
	strftime(FmtTime, sizeof(FmtTime), params.timeformat, local_time);

	// 构造完整的文件路径
	snprintf(CompletePath, sizeof(CompletePath), "%s/%s_%s.log", FmtFoldName, params.filename, FmtTime);

	// 打开文件
	stream = fopen(CompletePath, "w");
	if (stream == NULL)
	{
		perror("Failed to open file\n");
		exit(-1);
	}

	// 如果 version 未提供，默认设置为 0.0.0.1
	char *version = (params.version == NULL) ? "0.0.0.1": params.version;

	// 初始化并返回 LogInfo 结构体
	LogInfo loginfo;
	loginfo.version = strdup(version); // 使用 strdup 复制字符串
	for (int i = 0; i < params.argc; i++)
	{
		loginfo.argv[i] = strdup(params.argv[i]); // 复制命令行参数字符串
	}
	loginfo.argv[params.argc] = NULL; // 确保数组以 NULL 结尾
	// 将初始化信息存入全局变量
	global_log_info = loginfo;
	return loginfo;
}
