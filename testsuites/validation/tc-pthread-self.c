/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup CPthreadValSelf
 */

/*
 * Copyright (C) 2026 OHB System AG
 * Copyright (C) 2026 embedded brains GmbH & Co. KG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pthread.h>
#include <rtems.h>

#include "tx-support.h"

#include <rtems/test.h>

/**
 * @defgroup CPthreadValSelf spec:/c/pthread/val/self
 *
 * @ingroup TestsuitesValidationNoClock1
 *
 * @brief Tests the pthread_self() function.
 *
 * The test takes the expected identifiers from the creation of the worker task
 * and from the identification of the running task, not from pthread_self().
 *
 * This test case performs the following actions:
 *
 * - Call pthread_self() in the runner task.
 *
 *   - Check that the returned value is equal to the identifier of the runner
 *     task.
 *
 * - Let the worker task call pthread_self().
 *
 *   - Check that the returned value is equal to the identifier of the worker
 *     task.
 *
 * @{
 */

/**
 * @brief Test context for spec:/c/pthread/val/self test case.
 */
typedef struct {
  /**
   * @brief This member contains the identifier of the runner task.
   */
  rtems_id runner_id;

  /**
   * @brief This member contains the identifier of the worker task.
   */
  rtems_id worker_id;

  /**
   * @brief This member contains the return value of the pthread_self() call in
   *   the worker task.
   */
  pthread_t worker_retval;
} CPthreadValSelf_Context;

static CPthreadValSelf_Context CPthreadValSelf_Instance;

typedef CPthreadValSelf_Context Context;

static void Worker( rtems_task_argument arg )
{
  Context *ctx;

  ctx = (Context *) arg;

  while ( true ) {
    (void) ReceiveAnyEvents();
    ctx->worker_retval = pthread_self();
  }
}

static void CPthreadValSelf_Setup( CPthreadValSelf_Context *ctx )
{
  rtems_status_code sc;

  SetSelfPriority( PRIO_NORMAL );
  SetSelfAffinityOne( 0 );
  sc = rtems_task_ident(
    RTEMS_WHO_AM_I,
    RTEMS_SEARCH_ALL_NODES,
    &ctx->runner_id
  );
  T_rsc_success( sc );
  ctx->worker_id = CreateTask( "WORK", PRIO_HIGH );
  SetAffinityOne( ctx->worker_id, 0 );
  StartTask( ctx->worker_id, Worker, ctx );
}

static void CPthreadValSelf_Setup_Wrap( void *arg )
{
  CPthreadValSelf_Context *ctx;

  ctx = arg;
  CPthreadValSelf_Setup( ctx );
}

static void CPthreadValSelf_Teardown( CPthreadValSelf_Context *ctx )
{
  DeleteTask( ctx->worker_id );
  SetSelfAffinityAll();
  RestoreRunnerPriority();
}

static void CPthreadValSelf_Teardown_Wrap( void *arg )
{
  CPthreadValSelf_Context *ctx;

  ctx = arg;
  CPthreadValSelf_Teardown( ctx );
}

static T_fixture CPthreadValSelf_Fixture = {
  .setup = CPthreadValSelf_Setup_Wrap,
  .stop = NULL,
  .teardown = CPthreadValSelf_Teardown_Wrap,
  .scope = NULL,
  .initial_context = &CPthreadValSelf_Instance
};

/**
 * @brief Call pthread_self() in the runner task.
 */
static void CPthreadValSelf_Action_0( CPthreadValSelf_Context *ctx )
{
  pthread_t id;

  id = pthread_self();

  /*
   * Check that the returned value is equal to the identifier of the runner
   * task.
   */
  T_eq_u32( id, ctx->runner_id );
}

/**
 * @brief Let the worker task call pthread_self().
 */
static void CPthreadValSelf_Action_1( CPthreadValSelf_Context *ctx )
{
  ctx->worker_retval = (pthread_t) -1;
  SendEvents( ctx->worker_id, RTEMS_EVENT_0 );

  /*
   * Check that the returned value is equal to the identifier of the worker
   * task.
   */
  T_eq_u32( ctx->worker_retval, ctx->worker_id );
}

/**
 * @fn void T_case_body_CPthreadValSelf( void )
 */
T_TEST_CASE_FIXTURE( CPthreadValSelf, &CPthreadValSelf_Fixture )
{
  CPthreadValSelf_Context *ctx;

  ctx = T_fixture_context();

  CPthreadValSelf_Action_0( ctx );
  CPthreadValSelf_Action_1( ctx );
}

/** @} */
