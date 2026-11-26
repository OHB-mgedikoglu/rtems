/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup CPthreadReqEqual
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
 * @defgroup CPthreadReqEqual spec:/c/pthread/req/equal
 *
 * @ingroup TestsuitesValidationNoClock0
 *
 * @{
 */

typedef enum {
  CPthreadReqEqual_Pre_T1_Self,
  CPthreadReqEqual_Pre_T1_Other,
  CPthreadReqEqual_Pre_T1_NA
} CPthreadReqEqual_Pre_T1;

typedef enum {
  CPthreadReqEqual_Pre_T2_Same,
  CPthreadReqEqual_Pre_T2_Different,
  CPthreadReqEqual_Pre_T2_NA
} CPthreadReqEqual_Pre_T2;

typedef enum {
  CPthreadReqEqual_Post_Status_NonZero,
  CPthreadReqEqual_Post_Status_Zero,
  CPthreadReqEqual_Post_Status_NA
} CPthreadReqEqual_Post_Status;

typedef struct {
  uint8_t Skip : 1;
  uint8_t Pre_T1_NA : 1;
  uint8_t Pre_T2_NA : 1;
  uint8_t Post_Status : 2;
} CPthreadReqEqual_Entry;

/**
 * @brief Test context for spec:/c/pthread/req/equal test case.
 */
typedef struct {
  /**
   * @brief This member specifies the `t1` parameter value.
   */
  pthread_t t1;

  /**
   * @brief This member specifies the `t2` parameter value.
   */
  pthread_t t2;

  /**
   * @brief This member contains the identifier of the thread other than the
   *   thread identified by the `t1` parameter.
   */
  pthread_t other;

  /**
   * @brief This member contains the return value of the pthread_equal() call.
   */
  int status;

  /**
   * @brief This member contains the identifier of the worker task.
   */
  rtems_id worker_id;

  struct {
    /**
     * @brief This member defines the pre-condition states for the next action.
     */
    size_t pcs[ 2 ];

    /**
     * @brief If this member is true, then the test action loop is executed.
     */
    bool in_action_loop;

    /**
     * @brief This member contains the next transition map index.
     */
    size_t index;

    /**
     * @brief This member contains the current transition map entry.
     */
    CPthreadReqEqual_Entry entry;

    /**
     * @brief If this member is true, then the current transition variant
     *   should be skipped.
     */
    bool skip;
  } Map;
} CPthreadReqEqual_Context;

static CPthreadReqEqual_Context CPthreadReqEqual_Instance;

static const char *const CPthreadReqEqual_PreDesc_T1[] =
  { "Self", "Other", "NA" };

static const char *const CPthreadReqEqual_PreDesc_T2[] =
  { "Same", "Different", "NA" };

static const char *const *const CPthreadReqEqual_PreDesc[] =
  { CPthreadReqEqual_PreDesc_T1, CPthreadReqEqual_PreDesc_T2, NULL };

static void CPthreadReqEqual_Pre_T1_Prepare(
  CPthreadReqEqual_Context *ctx,
  CPthreadReqEqual_Pre_T1   state
)
{
  switch ( state ) {
    case CPthreadReqEqual_Pre_T1_Self: {
      /*
       * While the `t1` parameter identifies the calling thread.
       */
      ctx->t1 = pthread_self();
      ctx->other = ctx->worker_id;
      break;
    }

    case CPthreadReqEqual_Pre_T1_Other: {
      /*
       * While the `t1` parameter identifies a thread other than the calling
       * thread.
       */
      ctx->t1 = ctx->worker_id;
      ctx->other = pthread_self();
      break;
    }

    case CPthreadReqEqual_Pre_T1_NA:
      break;
  }
}

static void CPthreadReqEqual_Pre_T2_Prepare(
  CPthreadReqEqual_Context *ctx,
  CPthreadReqEqual_Pre_T2   state
)
{
  switch ( state ) {
    case CPthreadReqEqual_Pre_T2_Same: {
      /*
       * While the `t2` parameter identifies the thread identified by the `t1`
       * parameter.
       */
      ctx->t2 = ctx->t1;
      break;
    }

    case CPthreadReqEqual_Pre_T2_Different: {
      /*
       * While the `t2` parameter identifies a thread other than the thread
       * identified by the `t1` parameter.
       */
      ctx->t2 = ctx->other;
      break;
    }

    case CPthreadReqEqual_Pre_T2_NA:
      break;
  }
}

static void CPthreadReqEqual_Post_Status_Check(
  CPthreadReqEqual_Context    *ctx,
  CPthreadReqEqual_Post_Status state
)
{
  switch ( state ) {
    case CPthreadReqEqual_Post_Status_NonZero: {
      /*
       * The return value of pthread_equal() shall be a value other than zero.
       */
      T_ne_int( ctx->status, 0 );
      break;
    }

    case CPthreadReqEqual_Post_Status_Zero: {
      /*
       * The return value of pthread_equal() shall be zero.
       */
      T_eq_int( ctx->status, 0 );
      break;
    }

    case CPthreadReqEqual_Post_Status_NA:
      break;
  }
}

static void CPthreadReqEqual_Setup( CPthreadReqEqual_Context *ctx )
{
  ctx->worker_id = CreateTask( "WORK", PRIO_HIGH );
}

static void CPthreadReqEqual_Setup_Wrap( void *arg )
{
  CPthreadReqEqual_Context *ctx;

  ctx = arg;
  ctx->Map.in_action_loop = false;
  CPthreadReqEqual_Setup( ctx );
}

static void CPthreadReqEqual_Teardown( CPthreadReqEqual_Context *ctx )
{
  DeleteTask( ctx->worker_id );
}

static void CPthreadReqEqual_Teardown_Wrap( void *arg )
{
  CPthreadReqEqual_Context *ctx;

  ctx = arg;
  ctx->Map.in_action_loop = false;
  CPthreadReqEqual_Teardown( ctx );
}

static void CPthreadReqEqual_Action( CPthreadReqEqual_Context *ctx )
{
  ctx->status = pthread_equal( ctx->t1, ctx->t2 );
}

/* clang-format off */

static const CPthreadReqEqual_Entry
CPthreadReqEqual_Entries[] = {
  { 0, 0, 0, CPthreadReqEqual_Post_Status_NonZero },
  { 0, 0, 0, CPthreadReqEqual_Post_Status_Zero }
};

static const uint8_t
CPthreadReqEqual_Map[] = {
  0, 1, 0, 1
};

/* clang-format on */

static size_t CPthreadReqEqual_Scope( void *arg, char *buf, size_t n )
{
  CPthreadReqEqual_Context *ctx;

  ctx = arg;

  if ( ctx->Map.in_action_loop ) {
    return T_get_scope( CPthreadReqEqual_PreDesc, buf, n, ctx->Map.pcs );
  }

  return 0;
}

static T_fixture CPthreadReqEqual_Fixture = {
  .setup = CPthreadReqEqual_Setup_Wrap,
  .stop = NULL,
  .teardown = CPthreadReqEqual_Teardown_Wrap,
  .scope = CPthreadReqEqual_Scope,
  .initial_context = &CPthreadReqEqual_Instance
};

static inline CPthreadReqEqual_Entry CPthreadReqEqual_PopEntry(
  CPthreadReqEqual_Context *ctx
)
{
  size_t index;

  index = ctx->Map.index;
  ctx->Map.index = index + 1;
  return CPthreadReqEqual_Entries[ CPthreadReqEqual_Map[ index ] ];
}

static void CPthreadReqEqual_TestVariant( CPthreadReqEqual_Context *ctx )
{
  CPthreadReqEqual_Pre_T1_Prepare( ctx, ctx->Map.pcs[ 0 ] );
  CPthreadReqEqual_Pre_T2_Prepare( ctx, ctx->Map.pcs[ 1 ] );
  CPthreadReqEqual_Action( ctx );
  CPthreadReqEqual_Post_Status_Check( ctx, ctx->Map.entry.Post_Status );
}

/**
 * @fn void T_case_body_CPthreadReqEqual( void )
 */
T_TEST_CASE_FIXTURE( CPthreadReqEqual, &CPthreadReqEqual_Fixture )
{
  CPthreadReqEqual_Context *ctx;

  ctx = T_fixture_context();
  ctx->Map.in_action_loop = true;
  ctx->Map.index = 0;

  for (
    ctx->Map.pcs[ 0 ] = CPthreadReqEqual_Pre_T1_Self;
    ctx->Map.pcs[ 0 ] < CPthreadReqEqual_Pre_T1_NA;
    ++ctx->Map.pcs[ 0 ]
  ) {
    for (
      ctx->Map.pcs[ 1 ] = CPthreadReqEqual_Pre_T2_Same;
      ctx->Map.pcs[ 1 ] < CPthreadReqEqual_Pre_T2_NA;
      ++ctx->Map.pcs[ 1 ]
    ) {
      ctx->Map.entry = CPthreadReqEqual_PopEntry( ctx );
      CPthreadReqEqual_TestVariant( ctx );
    }
  }
}

/** @} */
