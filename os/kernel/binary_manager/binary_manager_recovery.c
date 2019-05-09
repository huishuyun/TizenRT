/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <debug.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#ifdef CONFIG_BOARD_ASSERT_AUTORESET
#include <sys/boardctl.h>
#endif
#include <tinyara/mm/mm.h>
#include <tinyara/sched.h>
#include <tinyara/init.h>
#include <tinyara/board.h>

#include "binary_manager.h"
#include "task/task.h"

extern bool abort_mode;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
struct binmgr_kill_data_s {
	int binid;
	int faultid;
};
typedef struct binmgr_kill_data_s binmgr_kill_data_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/* Reboot the board */
static void binary_manager_board_reset(void)
{
#ifdef CONFIG_BOARD_ASSERT_AUTORESET
	boardctl(BOARDIOC_RESET, EXIT_SUCCESS);
#else
	(void)irqsave();
	for (;;) {
#ifdef CONFIG_ARCH_LEDS
		//board_autoled_on(LED_PANIC);
		up_mdelay(250);
		//board_autoled_off(LED_PANIC);
		up_mdelay(250);
#endif
	}
#endif
}

static void recovery_kill_each(FAR struct tcb_s *tcb, FAR void *arg)
{
	int ret;
	binmgr_kill_data_t *info;

	info = (binmgr_kill_data_t *)arg;
	if (info == NULL || info->binid < 0 || info->faultid < 0) {
		return;
	}

	if (tcb->group->tg_loadtask == info->binid && tcb->pid != info->binid) {
		bmllvdbg("KILL!! %d\n", tcb->pid);
		ret = task_terminate(tcb->pid, true);
	}
}

/****************************************************************************
 * Name: recovery_kill_binary
 *
 * Description:
 *   This function will kill all the task/thread created by the binary
 *   i.e input pid.
 *   If any fault occurs in the system, recovery thread will call this function to
 *   kill all the thread/task created by the input binary.
 *
 * Input parameters:
 *   pid   -   The pid of the binary, whoes all childs to be killed
 *
 * Returned Value:
 *   Zero (OK) on success; otherwise -1 (ERROR) value is returned.
 *
 ****************************************************************************/
static int recovery_kill_binary(pid_t pid, int binid)
{
	int ret;

	binmgr_kill_data_t info;

	if (pid < 0 || binid < 0) {
		return ERROR;
	}

	/* Set binary id and faulty id */
	info.binid = binid;
	info.faultid = pid;

	/* Search all task/pthreads created by same loaded task */
	sched_foreach(recovery_kill_each, (FAR void *)&info);

	/* Finally, unload binary */
	ret = task_terminate(binid, true);
	if (ret < 0) {
		bmlldbg("Failed to unload binary %d, ret %d, errno %d\n", binid, ret, errno);
		return ERROR;
	}
	bmllvdbg("Unload binary! pid %d\n", binid);

	return OK;
}

/****************************************************************************
 * Name: binary_manager_recovery
 *
 * Description:
 *   This function will receive the faulty pid and check if its binary id is one
 *   of the registered binary with binary manager.
 *   If the binary is registered then it will be restarted after killing its
 *   child processes, and if it is not registered then board will be rebooted.
 *   And then, it creates loading thread to load binary again.
 *
 * Input parameters:
 *   pid   -   The pid of recovery message
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
void binary_manager_recovery(int pid)
{
	int ret;
	int bin_id;
	int bin_idx;
	char type_str[1];
	char data_str[1];
	struct tcb_s *tcb;
	char *loading_data[LOADTHD_ARGC + 1];

	bmllvdbg("Try to recover fault with pid %d\n", pid);

	if (pid > 0) {
		/* Get binary id of fault task and check it is registered in binary manager */
		tcb = sched_gettcb(pid);
		if (tcb == NULL || tcb->group == NULL || tcb->group->tg_loadtask < 0) {
			bmlldbg("Failed to get pid %d binary info\n", pid);
			goto reboot_board;
		}
		bin_id = tcb->group->tg_loadtask;
		bmllvdbg("pid %d, binary id %d\n", pid, bin_id);

		bin_idx = binary_manager_get_index_with_binid(bin_id);
		if (bin_idx < 0) {
			bmlldbg("binary pid %d is not registered to binary manager\n", bin_id);
			goto reboot_board;
		}

		/* Kill its children and restart binary if the binary is registered with the binary manager */
		ret = recovery_kill_binary(pid, bin_id);
		if (ret == OK) {
			BIN_ID(bin_idx) = -1;
			/* load binary and update binid */
			memset(loading_data, 0, sizeof(char *) * (LOADTHD_ARGC + 1));
			loading_data[0] = itoa(LOADCMD_LOAD, type_str, 10);
			loading_data[1] = itoa(bin_idx, data_str, 10);
			ret = binary_manager_loading(loading_data);
			if (ret > 0) {
				abort_mode = false;
				return 0;
			}
		}
	}

reboot_board:
	/* Reboot the board  */
	bmlldbg("RECOVERY FAIL, BOARD RESET!!\n");
	binary_manager_board_reset();
}
