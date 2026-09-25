#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

/*
 * ACAP 3 launcher: elflibcheck needs APPNAME to be ELF, and acap-startstop,
 * respawnd and list.cgi use pidof(APPNAME). So this stays resident, restarts
 * start.sh whenever it dies, and only exits on SIGTERM/SIGINT.
 */

static volatile int g_stop = 0;
static volatile pid_t g_child = -1;

static void sig_forward(int sig) {
    g_stop = 1;
    if (g_child > 0)
        kill(g_child, sig);
}

int main(void)
{
    signal(SIGTERM, sig_forward);
    signal(SIGINT,  sig_forward);
    signal(SIGCHLD, SIG_DFL);

    while (!g_stop) {
        pid_t pid = fork();
        if (pid == 0) {
            signal(SIGTERM, SIG_DFL);
            signal(SIGINT,  SIG_DFL);
            execl("/usr/local/packages/Tailscale_VPN/start.sh",
                  "/usr/local/packages/Tailscale_VPN/start.sh", (char *)0);
            _exit(127);
        }
        if (pid < 0) {
            sleep(5);
            continue;
        }
        g_child = pid;

        int status;
        pid_t ret;
        do {
            ret = waitpid(pid, &status, 0);
        } while (ret == -1 && errno == EINTR && !g_stop);

        g_child = -1;

        if (!g_stop) {
            sleep(3);
        }
    }
    return 0;
}
