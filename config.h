/* Define your modules here */
static const Module modules[] = {
    /*      COMMAND             INTERVAL    */
    {       "weather",          1500        },
    {       "news",             300         },
    {       "cputemp",          5           },
    {       "battery",          30          },
    {       "memory",           5           },
    {       "volume",           0           },
    {       "clock",            1           },
};

/* Redefine if you want to place modbar's named pipe somewhere else */
#define PIPE_PATH "/tmp/modbar.pipe"

/* Symbol to separate modules (blank if none) */
#define SEPARATOR " | "
