#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

volatile sig_atomic_t ping_count = 0;
volatile sig_atomic_t pong_count = 0;
volatile sig_atomic_t max_pairs = ;  // nombre de ping/pong, valeur par défaut

pid_t pid_ping = 0;  // PID du processus ping (père)
pid_t pid_pong = 0;  // PID du processus pong (fils)

/* Handler pour le processus PING (père) */
void handler_ping(int sig) {
    (void)sig;  // éviter un warning "paramètre inutilisé" du compilateur

    ping_count++;
    printf("ping%u ", (unsigned int)ping_count);
    fflush(stdout);

    // envoyer le signal à PONG pour qu'il réponde
    kill(pid_pong, SIGUSR2);
}

/* Handler pour le processus PONG (fils) */
void handler_pong(int sig) {
    (void)sig;

    pong_count++;
    printf("pong%u ", (unsigned int)pong_count);
    fflush(stdout);

    // si on a fini tous les ping/pong, on arrête tout
    if (pong_count >= max_pairs) {
        printf("\n");
        fflush(stdout);
        // dire au processus ping de s'arrêter
        kill(pid_ping, SIGTERM);
        // ce processus s'arrête aussi
        exit(0);
    }

    // sinon, demander au processus PING de continuer
    kill(pid_ping, SIGUSR1);
}

int main(int argc, char *argv[]) {
    pid_t pid;

    // si l'utilisateur donne un nombre de couples en argument
    if (argc >= 2) {
        int n = atoi(argv[1]);
        if (n <= 0) {
            fprintf(stderr, "Usage: %s [nb_couples_ping_pong]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        max_pairs = n;
    }

    // Création du processus fils
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* ---- Processus fils : PONG ---- */
        pid_pong = getpid();
        pid_ping = getppid();

        // Installer le handler pour SIGUSR2
        if (signal(SIGUSR2, handler_pong) == SIG_ERR) {
            perror("signal pong");
            exit(EXIT_FAILURE);
        }

        // Boucle d'attente des signaux
        while (1) {
            pause();  // attendre un signal
        }

    } else {
        /* ---- Processus père : PING ---- */
        pid_ping = getpid();
        pid_pong = pid;

        // Installer le handler pour SIGUSR1
        if (signal(SIGUSR1, handler_ping) == SIG_ERR) {
            perror("signal ping");
            exit(EXIT_FAILURE);
        }

        // Lancer le premier "ping" en envoyant un signal à soi-même
        kill(pid_ping, SIGUSR1);

        // Boucle d'attente des signaux
        // Le processus père sera tué par le fils avec SIGTERM à la fin
        while (1) {
            pause();  // attendre un signal
        }
    }

    return 0;
}
