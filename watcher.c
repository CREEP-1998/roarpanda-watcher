#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

volatile sig_atomic_t running = 1;

void handle_signal(int signal)
{
  running = 0;
}

int main(int argc, char *argv[])
{
  const char *path;
  if (argv[1] == NULL)
  {
    path = "/root/roarpanda";
  }
  else
  {
    path = argv[1];
  }
  int length, i = 0;
  int fd;
  int wd;
  char buffer[BUF_LEN];

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

  fd = inotify_init();

  if (fd < 0)
  {
    perror("inotify_init");
    exit(EXIT_FAILURE);
  }

  wd = inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);

  if (wd == -1)
  {
    perror("inotify_add_watch");
    exit(EXIT_FAILURE);
  }

  while (running)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int retval = select(fd + 1, &rfds, NULL, NULL, &timeout);

    if (retval == -1)
    {
      if (errno == EINTR)
      {
        continue;
      }
      else
      {
        perror("select");
        break;
      }
    }
    else if (retval)
    {
      length = read(fd, buffer, BUF_LEN);

      if (length < 0)
      {
        perror("read");
        exit(EXIT_FAILURE);
      }

      i = 0;
      while (i < length)
      {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        if (event->len)
        {
          if (event->mask & IN_CREATE)
          {
            if (event->mask & IN_ISDIR)
            {
              printf("The directory %s was created.\n", event->name);
            }
            else
            {
              printf("The file %s was created.\n", event->name);
            }
          }
          else if (event->mask & IN_DELETE)
          {
            if (event->mask & IN_ISDIR)
            {
              printf("The directory %s was deleted.\n", event->name);
            }
            else
            {
              printf("The file %s was deleted.\n", event->name);
            }
          }
          else if (event->mask & IN_MODIFY)
          {
            if (event->mask & IN_ISDIR)
            {
              printf("The directory %s was modified.\n", event->name);
            }
            else
            {
              printf("The file %s was modified.\n", event->name);
            }
          }
        }
        i += EVENT_SIZE + event->len;
      }
    }
  }

  inotify_rm_watch(fd, wd);
  close(fd);

  printf("Exiting program...\n");
  return 0;
}
