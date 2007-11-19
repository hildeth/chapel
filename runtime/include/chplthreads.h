#ifndef _chplthreads_h_
#define _chplthreads_h_

typedef void* (*_void_star_fun_p)(void*);

#include "chpl_cfg_threads.h"

_chpl_mutex_p _chpl_mutex_new(void);      // malloc and init a mutex
int _chpl_mutex_init(_chpl_mutex_p mutex);
int _chpl_mutex_lock(_chpl_mutex_p mutex);
int _chpl_mutex_trylock(_chpl_mutex_p mutex);
int _chpl_mutex_unlock(_chpl_mutex_p mutex);
int _chpl_mutex_destroy(_chpl_mutex_p mutex);

_chpl_condvar_p _chpl_condvar_new(void);  // malloc and init a condvar
int _chpl_condvar_init(_chpl_condvar_p cond);
int _chpl_condvar_destroy(_chpl_condvar_p cond);
int _chpl_condvar_signal(_chpl_condvar_p cond);
int _chpl_condvar_broadcast(_chpl_condvar_p cond);
int _chpl_condvar_wait_full(_chpl_condvar_p cond, _chpl_mutex_p mutex, volatile _bool *is_full);
int _chpl_condvar_wait_empty(_chpl_condvar_p cond, _chpl_mutex_p mutex, volatile _bool *is_full);

void _chpl_init_sync_aux(_chpl_sync_aux_t *);
int _chpl_sync_lock(_chpl_sync_aux_t *);
int _chpl_sync_unlock(_chpl_sync_aux_t *);
int _chpl_sync_wait_full_and_lock(_chpl_sync_aux_t *);
int _chpl_sync_wait_empty_and_lock(_chpl_sync_aux_t *);
int _chpl_sync_mark_and_signal_full(_chpl_sync_aux_t *);
int _chpl_sync_mark_and_signal_empty(_chpl_sync_aux_t *);
int _chpl_sync_is_full(_chpl_sync_aux_t *);

// Chapel system thread control
void    initChplThreads(void);             // main thread init's thread support
void    exitChplThreads(void);             // called by the main thread

_uint64 _chpl_thread_id(void);             // return caller's thread id
void    _chpl_thread_init(void);           // setup per-thread state
_bool   _chpl_get_serial(void);            // set dynamic serial state
void    _chpl_set_serial(_bool state);     // set dynamic serial state true or false
void    _chpl_serial_delete(_bool *p);


// macros to init, touch, and free reference-counted Chapel variables
#define _CHPL_REFC_INIT(_var, _refc, _mutex)            \
  *_refc = 0;                                           \
  _chpl_mutex_init(_mutex)

#define _CHPL_REFC_TOUCH(_var, _refc, _mutex)           \
  _chpl_mutex_lock(_mutex);                             \
  (*_refc)++;                                           \
  _chpl_mutex_unlock(_mutex)

#define _CHPL_REFC_FREE(_var, _refc, _mutex, ln, fn)    \
  _chpl_mutex_lock(_mutex);                             \
  (*_refc)--;                                           \
  if ((_refc) == 0) {                                   \
    _chpl_free( &(_var), ln, fn);                       \
    _chpl_free(_refc, ln, fn);                          \
    _chpl_mutex_unlock(_mutex);                         \
    _chpl_mutex_destroy(_mutex);                        \
    _chpl_free(_mutex, ln, fn);                         \
  } else {                                              \
    _chpl_mutex_unlock(_mutex);                         \
  }


// Fork and wait on nthreads. Used to implement Chapel's cobegin statement.
// Return aggregate thread join error.
int
_chpl_cobegin (int                      nthreads, // number of threads
               _chpl_threadfp_t        *fp,       // ptrs to functions to fork
               _chpl_threadarg_t       *a,        // function args
               _chpl_cobegin_wkspace_t *twrk);    // temporary work space

// Fork one thread.  Do not wait. Used to implement Chapel's begin statement.
// Return thread creation error.
int
_chpl_begin (_chpl_threadfp_t  fp,                // function to fork
             _chpl_threadarg_t a);                // function arg

#endif  // _chplthreads_h_
