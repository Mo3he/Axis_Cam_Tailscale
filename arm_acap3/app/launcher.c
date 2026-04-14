#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

/*
 * ACAP 3 supervisor launcher for Tailscale_VPN.
 *
 * elflibcheck.sh requires APPNAME to be an ELF binary.
 * acap-startstop / respawnd / list.cgi all use pidof(APPNAME) for status.
 *
 * This binary NEVER exits voluntarily — it loops restarting start.sh if it
 * dies, so:
 *   - pidof Tailscale_VPN always finds this process → UI shows "Running"
 *   - respawnd never triggers (it only fires when APPNAME exits)
 *   - If tailscaled OOMs and start.sh exits, we cleanly restart it
 *
 * To stop the app, acap-startstop calls stop_daemon which sends SIGTERM here.
 * We forward SIGTERM/SIGINT to the child and then exit.
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
            /* child: reset signals and exec start.sh */
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
            /* start.sh died unexpectedly — wait before restarting */
            sleep(3);
        }
    }
    return 0;
}
