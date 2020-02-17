/*************************************************************************
    > File Name: startup.c
    > Author: Kevin
    > Created Time: 2019-12-20
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>

#include <setjmp.h>
#include <pthread.h>

#include "startup.h"
#include "logging.h"

#include "cli.h"
#include "mng.h"
#include "nwk.h"
#include "cfg.h"

/* Defines  *********************************************************** */
enum {
  ARG_DIRTY_ENC,
  ARG_DIRTY_PORT,
  ARG_DIRTY_BR,
  ARG_DIRTY_SOCK_SRV,
  ARG_DIRTY_SOCK_CLT,
  ARG_DIRTY_SOCK_ENC,
};

#define ARG_KEY_ISENC "Encryption"
#define ARG_KEY_PORT "Port"
#define ARG_KEY_BAUDRATE "Baud Rate"
#define ARG_KEY_SOCK_SRV "Socket Server"
#define ARG_KEY_SOCK_CLT "Socket Client"
#define ARG_KEY_SOCK_ENC "Socket Encription"

/* Global Variables *************************************************** */
typedef struct {
  uint8_t key;
  const char *value;
}arg_key_t;

pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
jmp_buf initjmpbuf;

/* Static Variables *************************************************** */
static const arg_key_t arg_keys[] = {
  { ARG_DIRTY_ENC, ARG_KEY_ISENC },
  { ARG_DIRTY_PORT, ARG_KEY_PORT },
  { ARG_DIRTY_BR, ARG_KEY_BAUDRATE },
  { ARG_DIRTY_SOCK_SRV, ARG_KEY_SOCK_SRV },
  { ARG_DIRTY_SOCK_CLT, ARG_KEY_SOCK_CLT },
  { ARG_DIRTY_SOCK_ENC, ARG_KEY_SOCK_ENC },
};

static const int arg_keys_len = sizeof(arg_keys) / sizeof(arg_key_t);

static struct {
  bool started;
  pthread_t tid;
} mng_info;

static proj_args_t projargs = { 0 };
static init_func_t initfs[] = {
  cli_init,
  clr_all,
  init_ncp,
  cfg_init,
  mng_init,
};
static const int inits_num = ARR_LEN(initfs);

/* Static Functions Declaractions ************************************* */
static int setprojargs(int argc, char *argv[]);

void startup(int argc, char *argv[])
{
  err_t e;
  if (ec_success != (e = cli_proc_init(0, NULL))) {
    elog(e);
    return;
  }
  cli_proc(argc, argv); /* should never return */

  logging_deinit();
}

int offsetof_initfunc(init_func_t fn)
{
  int i;
  for (i = 0; i < inits_num; i++) {
    if (initfs[i] == fn) {
      break;
    }
  }
  return i;
}

int cli_proc(int argc, char *argv[])
{
  int ret, tmp;
  err_t e;

#if (__APPLE__ == 1)
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
#endif
  if (0 != setprojargs(argc, argv)) {
    LOGE("Set project args error.\n");
    exit(EXIT_FAILURE);
  }

  ret = setjmp(initjmpbuf);
  LOGM("Program <VERSION - %d.%d.%d> Started Up, Initialization Bitmap - 0x%x\n",
       PROJ_VERSION_MAJOR,
       PROJ_VERSION_MINOR,
       PROJ_VERSION_PATCH,
       ret);
  if (mng_info.started) {
    if (0 != (tmp = pthread_cancel(mng_info.tid))) {
      LOGE("Cancel pthread error[%d:%s]\n", tmp, strerror(tmp));
    }
    get_mng()->state = nil;
  }

  mng_info.started = false;
  if (ret == 0) {
    ret = FULL_RESET;
  }
  for (int i = 0; i < sizeof(int) * 8; i++) {
    if (!IS_BIT_SET(ret, i)) {
      continue;
    }
    if (ec_success != (e = initfs[i](NULL))) {
      elog(e);
      exit(EXIT_FAILURE);
    }
  }

  e = nwk_init(NULL);
  elog(e);

  if (0 != (ret = pthread_create(&mng_info.tid, NULL, mng_mainloop, NULL))) {
    err_exit_en(ret, "pthread create");
  }
  mng_info.started = true;

  cli_mainloop(NULL);

  if (0 != (ret = pthread_join(mng_info.tid, NULL))) {
    err_exit_en(ret, "pthread_join");
  }

  return 0;
}

/**
 * @defgroup project arguments
 * @{ */
static void print_usage(const char *name)
{
  fprintf(stderr, "Usage: %s -m mode [-p serial_port] [-b baud_rate]"
                  " [-s server_domain_socket_path] [-e is_domain_socket_encrypted? 0/1]"
                  " [-f XML_file_path]\n",
          name);
  fprintf(stderr, "     -m mode                         s - secure NCP with UNIX domain socket, i - insecure NCP, connect to UART directly\n");
  fprintf(stderr, "     -p serial_port                  Which UART port to connect to    - [Valid when in 'u' mode]\n");
  fprintf(stderr, "     -b baud_rate                    UART baud rate                   - [Valid when in 'u' mode]\n");
  fprintf(stderr, "     -s server_domain_socket_path    Server UNIX domain socket path   - [Valid when in 's' mode]\n");
  fprintf(stderr, "     -c client_domain_socket_path    Client UNIX domain socket path   - [Valid when in 's' mode]\n");
  fprintf(stderr, "     -e is_domain_socket_encrypted   1 - encrypted, 0 - not encrypted - [Valid when in 's' mode]\n");
  /* fprintf(stderr, "     -f XML_file_path                XML node configuration file path\n"); */

  exit(EXIT_FAILURE);
}

const proj_args_t *getprojargs(void)
{
  return &projargs;
}

static void store_args(lbitmap_t *dirty, int argc, char *argv[])
{
  char c;

  while (-1 != (c = getopt(argc, argv, "m:p:b:s:c:e:f:"))) {
    switch (c) {
      case 'm':
        BIT_SET(*dirty, ARG_DIRTY_ENC);
        if (optarg[0] == 's') {
          projargs.enc = true;
        } else if (optarg[0] == 'u') {
          projargs.enc = false;
        } else {
          print_usage(argv[0]);
          exit(0);
        }
        break;
      case 'p':
        BIT_SET(*dirty, ARG_DIRTY_PORT);
        strcpy(projargs.serial.port, optarg);
        break;
      case 'b':
        BIT_SET(*dirty, ARG_DIRTY_BR);
        projargs.serial.br = atoi(optarg);
        break;
      case 's':
        BIT_SET(*dirty, ARG_DIRTY_SOCK_SRV);
        strcpy(projargs.sock.srv, optarg);
        break;
      case 'c':
        BIT_SET(*dirty, ARG_DIRTY_SOCK_CLT);
        strcpy(projargs.sock.clt, optarg);
        break;
      case 'e':
        BIT_SET(*dirty, ARG_DIRTY_SOCK_ENC);
        projargs.sock.enc = (bool)atoi(optarg);
        break;
      /* case 'f': */
      /* connArg.configFilePath = optarg; */
      /* break; */
      default:
        break;
    }
  }
}

static inline void __store_cache_config(const char *str, uint8_t key)
{
  char *val;
  val = strchr(str, '=');
  while (*val == ' ') {
    val++;
  }
  if (key == ARG_DIRTY_ENC) {
    if (val[0] == '0') {
      projargs.enc = false;
    } else {
      projargs.enc = true;
    }
  } else if (key == ARG_DIRTY_PORT) {
    if (projargs.enc) {
      return;
    }
    strcpy(projargs.serial.port, val);
  } else if (key == ARG_DIRTY_BR) {
    if (projargs.enc) {
      return;
    }
    projargs.serial.br = atoi(optarg);
  } else if (key == ARG_DIRTY_SOCK_SRV) {
    if (!projargs.enc) {
      return;
    }
    strcpy(projargs.sock.srv, optarg);
  } else if (key == ARG_DIRTY_SOCK_CLT) {
    if (!projargs.enc) {
      return;
    }
    strcpy(projargs.sock.clt, optarg);
  } else if (key == ARG_DIRTY_SOCK_ENC) {
    if (!projargs.enc) {
      return;
    }
    projargs.sock.enc = (bool)atoi(optarg);
  }
}

#define LINE_MAX_LENGTH 100
#define FMT "%s = %s\n"
static inline void __append_cfg(char *buf, const char *key, const char *val)
{
  char line[LINE_MAX_LENGTH] = { 0 };
  sprintf(line, FMT, key, val);
  strcat(buf, line);
}

static int _setprojargs(int argc, char *argv[])
{
  size_t len;
  FILE *fp;
  lbitmap_t dirty = 0;
  char tmp[LINE_MAX_LENGTH];
  char *buf = NULL;

  if (argc > 1 && !strcmp(argv[1], "--help")) {
    print_usage(argv[0]);
    exit(0);
  }

  if (argc > 1) {
    store_args(&dirty, argc, argv);
  }

  if (-1 == access(CONFIG_CACHE_FILE_PATH, F_OK)) {
    /* File not exist */
    fp = fopen(CONFIG_CACHE_FILE_PATH, "w");
    fclose(fp);
  }

  fp = fopen(CONFIG_CACHE_FILE_PATH, "r");
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  buf = calloc(1, len + arg_keys_len * LINE_MAX_LENGTH);

  if (len) {
    rewind(fp);
    while (NULL != fgets(tmp, LINE_MAX_LENGTH, fp)) {
      for (int i = 0; i < arg_keys_len; i++) {
        if (!strstr(tmp, arg_keys[i].value)) {
          continue;
        }
        if (IS_BIT_SET(dirty, arg_keys[i].key)) {
          /* Handle the encription or not here */
          if (arg_keys[i].key == ARG_DIRTY_ENC) {
            char *val;
            val = strchr(tmp, '=');
            while (*val == ' ') {
              val++;
            }
            *val = projargs.enc ? '1' : '0';
            strcat(buf, tmp);
          }
          BIT_CLR(dirty, ARG_DIRTY_ENC);
          break;
        }
        /* Not dirty, load it */
        strcat(buf, tmp);
        __store_cache_config(tmp, arg_keys[i].key);
      }
    }
  }

  /* Sanity check */
  if ((projargs.enc && (!projargs.sock.srv[0] || !projargs.sock.clt[0]))
      || (!projargs.enc && (!projargs.serial.port[0] || !projargs.serial.br))) {
    print_usage(argv[0]);
    exit(1);
  }

  if (dirty) {
    for (int i = 0; i < arg_keys_len && dirty; i++) {
      if (!IS_BIT_SET(dirty, arg_keys[i].key)) {
        continue;
      }
      if (arg_keys[i].key == ARG_DIRTY_PORT) {
        __append_cfg(buf, arg_keys[i].value, projargs.serial.port);
      } else if (arg_keys[i].key == ARG_DIRTY_BR) {
        char v[20] = { 0 };
        sprintf(v, "%u", projargs.serial.br);
        __append_cfg(buf, arg_keys[i].value, v);
      } else if (arg_keys[i].key == ARG_DIRTY_SOCK_SRV) {
        __append_cfg(buf, arg_keys[i].value, projargs.sock.srv);
      } else if (arg_keys[i].key == ARG_DIRTY_SOCK_CLT) {
        __append_cfg(buf, arg_keys[i].value, projargs.sock.clt);
      } else if (arg_keys[i].key == ARG_DIRTY_SOCK_ENC) {
        char v[20] = { 0 };
        sprintf(v, "%d", projargs.sock.enc);
        __append_cfg(buf, arg_keys[i].value, v);
      } else {
        ASSERT(0);
      }
      BIT_CLR(dirty, arg_keys[i].key);
    }
  }

  /* TODO */
  /* Write back to the file */
  return 0;
}

static int setprojargs(int argc, char *argv[])
{
#if 0
  /* TODO: Impl pending */
#else
  projargs.enc = false;
  memcpy(projargs.serial.port, PORT, sizeof(PORT));
  projargs.serial.br = 115200;
  projargs.initialized = true;
  return 0;
#endif
}

/**  @} */
