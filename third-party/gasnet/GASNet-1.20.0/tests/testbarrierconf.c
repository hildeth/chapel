/*   $Source: /var/local/cvs/gasnet/tests/testbarrierconf.c,v $
 *     $Date: 2012/09/17 06:05:43 $
 * $Revision: 1.26 $
 * Description: GASNet barrier performance test
 * Copyright 2002, Dan Bonachea <bonachea@cs.berkeley.edu>
 * Terms of use are as specified in license.txt
 */

#include <gasnet.h>

#include <test.h>

#if defined(GASNETE_USING_ELANFAST_BARRIER) 
  #define PERFORM_MIXED_NAMED_ANON_TESTS (!GASNETE_USING_ELANFAST_BARRIER())
#else
  #define PERFORM_MIXED_NAMED_ANON_TESTS 1
#endif


static int do_try = 0;
GASNETT_INLINE(my_barrier_wait)
int my_barrier_wait(int value, int flags) {
  int rc;
  if (do_try) {
    do { rc = gasnet_barrier_try(value, flags); } while (rc == GASNET_ERR_NOT_READY);
  } else {
    rc = gasnet_barrier_wait(value, flags);
  }
  return rc;
}

#define hidx_done_shorthandler   200
volatile int done = 0;
void done_shorthandler(gasnet_token_t token) { done = 1; }
gasnet_handlerentry_t htable[] = { { hidx_done_shorthandler,  done_shorthandler  } };

static void * doTest(void *arg);

static int mynode, nodes, iters;

int main(int argc, char **argv) {
  int pollers = 0;
  int arg;

  GASNET_Safe(gasnet_init(&argc, &argv));
  GASNET_Safe(gasnet_attach(htable, 1, TEST_SEGSZ_REQUEST, TEST_MINHEAPOFFSET));

#if GASNET_PAR
  test_init("testbarrierconf", 0, "[-t] [-p polling_threads] (iters)\n"
            "  The -p option gives a number of polling threads to spawn (default is 0).\n"
            "  The -t option replaces barrier_wait calls with looping on barrier_try");
#else
  test_init("testbarrierconf", 0, "[-t] (iters)\n"
            "  The -t option replaces barrier_wait calls with looping on barrier_try");
#endif

  arg = 1;
  while (argc-arg >= 1) {
   if (!strcmp(argv[arg], "-p")) {
#ifdef GASNET_PAR
    if (argc-arg < 2) {
      if (gasnet_mynode() == 0) {
        fprintf(stderr, "testbarrierconf %s\n", GASNET_CONFIG_STRING);
        fprintf(stderr, "ERROR: The -p option requires an argument.\n");
        fflush(NULL);
      }
      sleep(1);
      gasnet_exit(1);
    }
    pollers = atoi(argv[arg+1]);
    arg += 2;
#else
    if (gasnet_mynode() == 0) {
      fprintf(stderr, "testbarrierconf %s\n", GASNET_CONFIG_STRING);
      fprintf(stderr, "ERROR: The -p option is only available in the PAR configuration.\n");
      fflush(NULL);
    }
    sleep(1);
    gasnet_exit(1);
#endif
   } else if (!strcmp(argv[arg], "-t")) {
    do_try = 1;
    arg += 1;
   } else break;
  }
  if (argc-arg >= 1) iters = atoi(argv[arg]);
  if (iters <= 0) iters = 1000;
  if (argc-arg >= 2) test_usage();

  mynode = gasnet_mynode();
  nodes = gasnet_nodes();

  if (mynode == 0) {
      const char * mode = do_try ? "try" : "wait";
#ifdef GASNET_PAR
      printf("Running barrier_%s conformance test with %d iterations and %i extra polling theads...\n", mode, iters,pollers);
#else
      printf("Running barrier_%s conformance test with %d iterations...\n", mode, iters);
#endif
      fflush(stdout);
  }
  BARRIER();

#ifdef GASNET_PAR
  if (pollers)
      test_createandjoin_pthreads(pollers+1,doTest,NULL,0);
  else
#endif
      doTest(NULL);

  MSG("done.");

  gasnet_exit(0);
  return 0;
}

static void * doTest(void *arg) {
  int i = 0;
  int result;

  if (arg) {
    /* I am a polling thread */
    GASNET_BLOCKUNTIL(done);
    return NULL;
  }

  BARRIER();

  if (!PERFORM_MIXED_NAMED_ANON_TESTS) {
    if (mynode == 0) {
      MSG("WARNING: skipping tests which mix named and anonymous barriers, "
          "which are known to fail in this configuration");
    }
    BARRIER();
  }

  /* Test for required failures: */
  for (i = 0; i < iters; ++i) {
    /* node 0 indicates mismatch on entry: */
    gasnet_barrier_notify(0, !mynode ? GASNET_BARRIERFLAG_MISMATCH : 0);
    result = my_barrier_wait(0, !mynode ? GASNET_BARRIERFLAG_MISMATCH : 0);
    if (result != GASNET_ERR_BARRIER_MISMATCH) {
      MSG("ERROR: Failed to detect barrier mismatch indicated on notify.");
      gasnet_exit(1);
    }

    /* ids differ between notify and wait */
    gasnet_barrier_notify(0, 0);
    result = my_barrier_wait(1, 0);
    if (result != GASNET_ERR_BARRIER_MISMATCH) {
      MSG("ERROR: Failed to detect mismatch between id at notify and wait.");
      gasnet_exit(1);
    }

    /* Flags can (as of GASNet 1.20) differ between notify and wait: */
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    result = my_barrier_wait(0, 0);
    if (result != GASNET_OK) {
      MSG("ERROR: Failed to allow anonymous notify with named wait.");
      gasnet_exit(1);
    }
    gasnet_barrier_notify(0, 0);
    result = my_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    if (result != GASNET_OK) {
      MSG("ERROR: Failed to allow named notify with anonymous wait.");
      gasnet_exit(1);
    }

    if (nodes > 1) {
      int j;

      for (j = 0; j < nodes; ++j) {
       if (PERFORM_MIXED_NAMED_ANON_TESTS) {
        /* Mix many named with one anonymous: */
        if (mynode == j) {
          gasnet_barrier_notify(12345, GASNET_BARRIERFLAG_ANONYMOUS);
          result = my_barrier_wait(12345, GASNET_BARRIERFLAG_ANONYMOUS);
        } else {
          gasnet_barrier_notify(5551212, 0);
          result = my_barrier_wait(5551212, 0);
        }
        if (result != GASNET_OK) {
          MSG("ERROR: Failed to match anon notify on node %d with named notify elsewhere.", j);
          gasnet_exit(1);
        }

        /* Mix many named with one anonymous notify plus named wait: */
        if (mynode == j) {
          gasnet_barrier_notify(12345, GASNET_BARRIERFLAG_ANONYMOUS);
          result = my_barrier_wait(5551212, 0);
        } else {
          gasnet_barrier_notify(5551212, 0);
          result = my_barrier_wait(5551212, 0);
        }
        if (result != GASNET_OK) {
          MSG("ERROR: Failed to match anon notify and named wait on node %d with named notify elsewhere.", j);
          gasnet_exit(1);
        }

        /* Mix many named with one anonymous notify plus MISnamed wait: */
        if (mynode == j) {
          gasnet_barrier_notify(12345, GASNET_BARRIERFLAG_ANONYMOUS);
          result = my_barrier_wait(12345, 0);
          if (result != GASNET_ERR_BARRIER_MISMATCH) {
            MSG("ERROR: Failed to detect anon notify and mis-named wait on node %d with named notify elsewhere.", j);
            gasnet_exit(1);
          }
        } else {
          gasnet_barrier_notify(5551212, 0);
          result = my_barrier_wait(5551212, 0);
          /* neither required not prohibited from signalling an error here. */
        }

        /* Mix one named with many anonymous: */
        if (mynode == j) {
          gasnet_barrier_notify(0xcafef00d, 0);
          result = my_barrier_wait(0xcafef00d, 0);
        } else {
          gasnet_barrier_notify(911, GASNET_BARRIERFLAG_ANONYMOUS);
          result = my_barrier_wait(911, GASNET_BARRIERFLAG_ANONYMOUS);
        }
        if (result != GASNET_OK) {
          MSG("ERROR: Failed to match named notify on node %d with anon notify elsewhere.", j);
          gasnet_exit(1);
        }
      }
        /* Mismatched id: */
        gasnet_barrier_notify(mynode == j, 0);
        result = my_barrier_wait(mynode == j, 0);
        if (result != GASNET_ERR_BARRIER_MISMATCH) {
          MSG("ERROR: Failed to detect different id on node %d.", j);
          gasnet_exit(1);
        }

        /* Node j indicates mismatch on entry: */
        gasnet_barrier_notify(0, (j == mynode) ? GASNET_BARRIERFLAG_MISMATCH : 0);
        result = my_barrier_wait(0, (j == mynode) ? GASNET_BARRIERFLAG_MISMATCH : 0);
        if (result != GASNET_ERR_BARRIER_MISMATCH) {
          MSG("ERROR: Failed to detect barrier mismatch indicated on notify by node %d.", j);
          gasnet_exit(1);
        }

#if 0 /* TBD: is this case clearly defined by the spec? */
        /* Node j indicates anon+mismatch on entry: */
        gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS |
                              ((j == mynode) ? GASNET_BARRIERFLAG_MISMATCH : 0));
        result = my_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS |
                                 ((j == mynode) ? GASNET_BARRIERFLAG_MISMATCH : 0));
        if (result != GASNET_ERR_BARRIER_MISMATCH) {
          MSG("ERROR: Failed to detect anonymous barrier mismatch indicated on notify by node %d.");
          gasnet_exit(1);
        }
#endif
      }
    } else if (i == 0) { /* DOB: only warn once per run */
      MSG("WARNING: pair mismatch tests skipped (only 1 node)");
    }

    if (nodes > 2) {
      int j, k;

      for (j = 0; j < nodes; ++j) {
        for (k = 0; k < nodes; ++k) {
	  if (k == j) continue;

         if (PERFORM_MIXED_NAMED_ANON_TESTS) {
          /* Mix two names and anonymous: */
          if (mynode == j) {
            gasnet_barrier_notify(1592, 0);
            result = my_barrier_wait(1592, 0);
          } else if (mynode == k) {
            gasnet_barrier_notify(1776, 0);
            result = my_barrier_wait(1776, 0);
          } else {
            gasnet_barrier_notify(12345, GASNET_BARRIERFLAG_ANONYMOUS);
            result = my_barrier_wait(12345, GASNET_BARRIERFLAG_ANONYMOUS);
          }
          if (result != GASNET_ERR_BARRIER_MISMATCH) {
            MSG("ERROR: Failed to detect mismatched names (on %d and %d) intermixed with anon.", j, k);
            gasnet_exit(1);
          }

          /* Mix one named with many anonymous, of which one gives MISnamed wait: */
          if (mynode == j) {
            gasnet_barrier_notify(511, GASNET_BARRIERFLAG_ANONYMOUS);
            result = my_barrier_wait(511, 0);
            if (result != GASNET_ERR_BARRIER_MISMATCH) {
              MSG("ERROR: Failed to detect anon notify and mis-named wait on node %d with one named notify elsewhere.", k);
              gasnet_exit(1);
            }
          } else {
            gasnet_barrier_notify(42, (mynode == k) ? 0: GASNET_BARRIERFLAG_ANONYMOUS);
            result = my_barrier_wait(42, (mynode == k) ? 0: GASNET_BARRIERFLAG_ANONYMOUS);
            /* neither required not prohibited from signalling an error here. */
          }
         } 
        }
      }
    } else if (i == 0) { /* DOB: only warn once per run */
      MSG("WARNING: multiway mismatch tests skipped (less than 3 nodes)");
    }
    TEST_PROGRESS_BAR(i, iters);
    BARRIER();
  }

  GASNET_Safe(gasnet_AMRequestShort0(mynode, hidx_done_shorthandler));
  return NULL;
}