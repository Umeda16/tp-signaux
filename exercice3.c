#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit, EXIT_FAILURE
#include <unistd.h>     // fork, getpid, getppid, pause, alarm
#include <signal.h>     // signal, kill, SIGALRM, SIGUSR1, SIGUSR2
#include <sys/types.h>  // pid_t

// Compteurs d'heure, minute, seconde
volatile sig_atomic_t heures   = 0;
volatile sig_atomic_t minutes  = 0;
volatile sig_atomic_t secondes = 0;

// PIDs utiles pour la communication
pid_t pid_H = 0;  // processus heures (père)
pid_t pid_M = 0;  // processus minutes (fils 1)

// --- Handler du processus S (secondes) ---
void handler_secondes(int sig) {
    (void)sig;  // éviter un warning

    secondes++;

    if (secondes >= 60) {
        secondes = 0;
        // Informer le processus M que l'on est passé de 59 à 0
        kill(pid_M, SIGUSR1);
    }

    // Affichage du point de vue des secondes
    printf("[S] Heure : %02d:%02d:%02d\n", heures, minutes, secondes);
    fflush(stdout);

    // Reprogrammer l'alarme pour la seconde suivante
    alarm(1);
}

// --- Handler du processus M (minutes) ---
void handler_minutes(int sig) {
    (void)sig;

    minutes++;

    if (minutes >= 60) {
        minutes = 0;
        // Informer le processus H que l'on est passé de 59 à 0
        kill(pid_H, SIGUSR2);
    }

    // Affichage du point de vue des minutes
    printf("[M] Heure : %02d:%02d:%02d\n", heures, minutes, secondes);
    fflush(stdout);
}

// --- Handler du processus H (heures) ---
void handler_heures(int sig) {
    (void)sig;

    heures = (heures + 1) % 24;  // heures modulo 24

    // Affichage du point de vue des heures
    printf("[H] Heure : %02d:%02d:%02d\n", heures, minutes, secondes);
    fflush(stdout);
}

int main(void) {
    pid_t pid;

    // Le processus initial sera le processus H (heures)
    pid_H = getpid();

    // ----- Création du processus M (minutes) -----
    pid = fork();
    if (pid < 0) {
        perror("fork M");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // ---- Nous sommes dans le processus M ----
        pid_H = getppid();  // le père est H

        // Installer le handler pour SIGUSR1 (signal venant de S)
        if (signal(SIGUSR1, handler_minutes) == SIG_ERR) {
            perror("signal minutes");
            exit(EXIT_FAILURE);
        }

        // Boucle d'attente des signaux
        while (1) {
            pause();
        }

    } else {
        // Nous sommes toujours dans le père (H)
        pid_M = pid;  // garder le PID de M

        // ----- Création du processus S (secondes) -----
        pid = fork();
        if (pid < 0) {
            perror("fork S");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // ---- Nous sommes dans le processus S ----
            // Ici, pid_M a été initialisé dans le père avant le fork,
            // S en hérite donc et peut lui envoyer des signaux.

            // Installer le handler pour SIGALRM
            if (signal(SIGALRM, handler_secondes) == SIG_ERR) {
                perror("signal secondes");
                exit(EXIT_FAILURE);
            }

            // Programmer la première alarme dans 1 seconde
            alarm(1);

            // Boucle d'attente des signaux
            while (1) {
                pause();
            }

        } else {
            // ---- Nous restons dans le processus H ----

            // Installer le handler pour SIGUSR2 (signal venant de M)
            if (signal(SIGUSR2, handler_heures) == SIG_ERR) {
                perror("signal heures");
                exit(EXIT_FAILURE);
            }

            // Boucle d'attente des signaux dans H
            while (1) {
                pause();
            }
        }
    }

    return 0;
}
