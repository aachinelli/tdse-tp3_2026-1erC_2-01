/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "task_display_attribute.h"
#include "task_display_interface.h"
#include "task_test_attribute.h"

/********************** macros and definitions *******************************/
#define DEL_TEST_XX_MIN				0ul
#define DEL_TEST_XX_MED				500ul
#define DEL_TEST_XX_MAX				1000ul

/********************** internal data declaration ****************************/
task_test_dta_t task_test_dta =
	{DEL_TEST_XX_MIN};

/********************** internal functions declaration ***********************/
void task_test_statechart(void);

/********************** internal data definition *****************************/
const char *p_task_test 		= "Task Test (Test Code Integration)";
const char *p_task_test_ 		= "Non-Blocking & Update By Time Code";
const char *p_task_test__ 		= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void task_test_init(void *parameters)
{
	task_test_dta_t *p_task_test_dta;
	uint32_t tick;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", GET_NAME(task_test_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_test), p_task_test);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_test), p_task_test_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_test), p_task_test__);

	/* Update Task Test Configuration & Data Pointer */
	p_task_test_dta = &task_test_dta;

	tick = DEL_TEST_XX_MAX;
	p_task_test_dta->tick = tick;
	p_task_test_dta->counter = 0ul;
	
	LOGGER_INFO("   %s = %lu", GET_NAME(tick), (uint32_t)tick);

	put_event_task_display(0, 0, "LCD Display Test");
	put_event_task_display(0, 1, " Porting C code ");
}

void task_test_update(void *parameters)
{
	/* Run Task Statechart */
    task_test_statechart();
}

void task_test_statechart(void)
{
	task_test_dta_t *p_task_test_dta;
	char test_str[8];

	/* Update Task Test Configuration & Data Pointer */
    p_task_test_dta = &task_test_dta;

    /* Update Task Counter */
	p_task_test_dta->counter++;

    if (DEL_TEST_XX_MIN < p_task_test_dta->tick)
	{
		p_task_test_dta->tick--;
	}
	else
	{
		p_task_test_dta->tick = DEL_TEST_XX_MAX ;

		put_event_task_display(0, 1, "Test Nro: ******");

		snprintf(test_str, sizeof(test_str), "%lu", (p_task_test_dta->counter/DEL_TEST_XX_MAX));
		put_event_task_display(10, 1, test_str);
	}
}

/********************** end of file ******************************************/
