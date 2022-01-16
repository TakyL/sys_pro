
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#define taille 80                //Taille max du mot
#define PORT 6000               //Port utilise
#define MAX_BUFFER 1000       //Taille max du buffer
#define MAX_CLIENTS 10       //Nombre de clients max autorises
#define FICHIER "test2.txt" //Fichier utilise pour la recherche du mot

//Variables globales
const char *EXIT = "exit";
pthread_mutex_t mutex;
int fdSocketAttente;
int fdSocketCommunication;
struct sockaddr_in coordonneesServeur;
struct sockaddr_in coordonneesAppelant;

char tampon[MAX_BUFFER]; //Tammpon
char motchoisit[taille];
int etatjeu = 1;     //1 si partie en cours, 0 si la partie est terminee
int nbmots_fichiers; //Nombre de mots du fichier

//Definition des structures
typedef struct
{
    int id;                       //Identifiant d'un joueur
    int vie;                     //Nombre de vie d'un joueur
    int nbvictoire;             //Nombre de victoire
    char *masque[taille];      //Masque du joueur
    char coup;                //coup du joueur
    char *motdujeu;          //Mot du joueur
} joueur; //Joueur

typedef struct
{
    int idjoueur;
    joueur jo[MAX_CLIENTS];
} liste_joueur; //Liste des joueurs

int total_mot_fichier(FILE *f, int total, int caract); //Calcule le nombre total de mot dans un fichier
int Ouverturefichier(FILE *fichier1); //Ouvre un fichier de type FILE
void initjoueur(liste_joueur *liste_j); //Initialise  la liste des joueurs et les joueurs
char *masque_mot(char *mot); //Permet de masquer un mot : ***
void recherchemot(char *motPioche, int fdSocketCommunication); //Fonction de recherche d'un mot aleatoire se trouvant dans un fichier et l'envoie au joueur
void affichage_liste_joueur(liste_joueur *liste_j); //Affiche la liste de joueurs : IP + Nombre de vie restantes
int verif_statut(char verif_lettre[], int lon_mot); //Determine si le joueur gagne ou pas
int recherche_lettre(char lettre, char *motPioche, char *verif_lettre); //Recherche si la lettre joué appartient au mot

int main(int argc, char const *argv[])
{
    int demande_mot =0; //Demande de mot lors d'une relance
    char *verif_lettre = NULL; //Permet de verifier une lettre
    int relancer = 1; //1 quand il veut rejouer sinon 0
    etatjeu = 1;      
    int nbRecu;
    int longueurAdresse;
    int pid;
    char *copie_mot = (char *)malloc(taille * sizeof(char)); //Varaible qui sert à copie le mot joué
    char *mot_brut = (char *)malloc(taille * sizeof(char)); //Variable qui sert à garder le mot brut joueur
    int coup=3; // Nombre de coup pour le joueur (variable temporaire)


    liste_joueur *lj; // Liste des joueurs
    //initjoueur(lj); //Initialisation de la liste
    fdSocketAttente = socket(PF_INET, SOCK_STREAM, 0);

    if (fdSocketAttente < 0)
    {
        printf("socket incorrecte\n");
        exit(EXIT_FAILURE);
    }

    // On prépare l’adresse d’attachement locale
    longueurAdresse = sizeof(struct sockaddr_in);
    memset(&coordonneesServeur, 0x00, longueurAdresse);

    // connexion de type TCP
    coordonneesServeur.sin_family = PF_INET;
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    // le port d'écoute
    coordonneesServeur.sin_port = htons(PORT);

    if (bind(fdSocketAttente, (struct sockaddr *)&coordonneesServeur, sizeof(coordonneesServeur)) == -1)
    {
        printf("erreur de bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(fdSocketAttente, 5) == -1)
    {
        printf("erreur de listen\n");
        exit(EXIT_FAILURE);
    }

    socklen_t tailleCoord = sizeof(coordonneesAppelant);

    int nbClients = 0;

    printf("En attente de client \n");
    while (nbClients < MAX_CLIENTS)
    {
        if ((fdSocketCommunication = accept(fdSocketAttente, (struct sockaddr *)&coordonneesAppelant, &tailleCoord)) == -1)
        {
            printf("erreur de accept\n");
            exit(EXIT_FAILURE);
        }

        printf("Client connecté - %s:%d: \n", inet_ntoa(coordonneesAppelant.sin_addr), ntohs(coordonneesAppelant.sin_port));

        if ((pid = fork()) == 0)
        {
            close(fdSocketAttente);
            send(fdSocketCommunication, "Bievenue au jeu du PENDU !\n", 28, 0); //Envoie un message d'accueil
            
               
            //Boucle de jeu
            while (etatjeu == 1 && relancer == 1) //Tant que le joueur veut relancer et que le jeu est en cours
            {

                printf("\n En attente de %s:%d \n", inet_ntoa(coordonneesAppelant.sin_addr),  ntohs(coordonneesAppelant.sin_port));
                
                nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0); //En attente d'un message
                
                //Quand un message est reçu
                if (nbRecu > 0)
                {
                    tampon[nbRecu] = 0; //Vide le buffer pour affichage
                    printf("Tampon : %s\n", tampon);

                    if ((demande_mot == 1)||(strstr(tampon,"ok")&&demande_mot==0)) //Demande de mot
                    {
                       // printf("ETAT %d\n",demande_mot);
                        /*
                        if(demande_mot == 1) 
                        {
                            send(fdSocketCommunication,"\nRedemarrage de la partie !\n",29,0); //Envoie un message (effet similaire à la ligne 123)
                            demande_mot = 0;
                        }*/
                        recherchemot(motchoisit, fdSocketCommunication); //Recherche un mot dans le dictionnaire
                        
                        verif_lettre = malloc(strlen(motchoisit) * sizeof(int));
                        mot_brut = malloc(strlen(motchoisit) * sizeof(int));

                        strcpy(mot_brut, motchoisit); //Récupère la valeur du brut du masque
                      //  printf("JE DEBUG1 %s:%s\n",motchoisit,mot_brut);

                        masque_mot(motchoisit); //Applique un masque ***
                        //printf("JE DEBUG2\n");
                        send(fdSocketCommunication, motchoisit, strlen(motchoisit) - 1, 0); //Envoie le mot caché au joueur
                       
                       // printf("ETAT2 %d\n",demande_mot);
                        demande_mot = 0;
                    }
                    else if (nbRecu == 1) //Le joueur joue
                    {
                        //printf("JE DEBUG2 : %d:%s:%s\n", tampon[nbRecu], motchoisit, verif_lettre);
                        if (!recherche_lettre(tampon[nbRecu], mot_brut, verif_lettre)) //Si lettre jouée pas présente
                        {
                           // printf("PAS PRESENTE\n");
                           // lj->jo->vie--;
                            coup--;
                            send(fdSocketCommunication,motchoisit,strlen(motchoisit)-1,0);
                        }
                        else //Lettre présente + On renvoie le mot
                        {
                            //printf("PRESENTE\n");
                            //Copie la réponse

                            if (copie_mot != NULL) strcpy(copie_mot, tampon);
                            //Releve les caractères cachés
                            for (int o = 0; o != '\0'; o++)
                            {
                                if (tampon[nbRecu] == copie_mot[o])
                                    copie_mot[o] = tampon[nbRecu];
                                else
                                    copie_mot[o] = '*'; //Par securite
                            }
                            //Renvoie
                           // printf("Verif debug 3 %s\n",copie_mot);
                            send(fdSocketCommunication, copie_mot, strlen(copie_mot) - 1, 0);

                             //Si le joueur gagne
                            //if (verif_statut(verif_lettre, strlen(motchoisit)) && lj->jo->vie > 0)
                        if(verif_statut(verif_lettre, strlen(motchoisit)) && coup > 0)
                        {
                            //printf("CDT1\n");
                            printf("\n %s a gagne !\n", inet_ntoa(coordonneesAppelant.sin_addr));
                           
                            
                            send(fdSocketCommunication, "Vous avez gagne !\n", 20, 0);
                            etatjeu = 0;
                        }
                        //Si le joueur perd
                        else
                        {
                           // printf("CDT2\n");
                            printf("\n %s a perdu !\n", inet_ntoa(coordonneesAppelant.sin_addr));
                            
                           
                            send(fdSocketCommunication, "Vous avez perdu !\n", 19, 0);
                            etatjeu = 0;
                        }

                        }//Fin boucle else 
                       
                    }//Fin boucle joueur qui joue une lettre

                    //printf("Recu : %d \n", nbRecu);
                } //Fin de reception : joueur 

                //Quand le joueur a fini une partie
                if (etatjeu == 0)
                {
                    printf("Demande de relance au joueur :%s : %d\n", inet_ntoa(coordonneesAppelant.sin_addr), ntohs(coordonneesAppelant.sin_port)); 
                    
                    char *demande = "Voulez-vous rejouez ? y/n\n";

                    //Envoie du message
                    send(fdSocketCommunication, demande, 27, 0);
                    nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0);
                    
                    //controle sa réponse
                    if (strstr(tampon, "y")) //Si il repond y 
                    {
                        printf("COUCOU\n");
                        etatjeu = 1;
                        relancer = 1;
                        demande_mot = 1;
                        free(verif_lettre);
                        free(copie_mot);
                    }
                    else if (strstr(tampon, "n"))//Si il repond n
                    {
                        relancer = 0;
                        send(fdSocketCommunication, "FIN DU JEU DU PENDU \n", 22, 0);
                        free(verif_lettre);
                        free(copie_mot);
                        nbClients--;
                        //Sortie de boucle
                    }
                    else //Si autre lettre : On redemande le message
                    {
                        send(fdSocketCommunication, demande, 27, 0);
                    }
                }
            }
            printf("\nFin du fork\n");

            exit(EXIT_SUCCESS);
        }

        nbClients++;
        // lj->idjoueur++; //Implémente le nb de joueur

    }

    close(fdSocketCommunication);
    close(fdSocketAttente);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        wait(NULL);
    }

    printf("Fin du programme.\n");
    return EXIT_SUCCESS;
}


        //FONCTIONS

int total_mot_fichier(FILE *f, int total, int caract)
{
    do //COmpter nb de mots
    {
        caract = fgetc(f);
        if (caract == '\n')
            total++; //ou ++
    } while (caract != EOF);
    return total;
}



//Permet d'ouvrir le fichier
int Ouverturefichier(FILE *fichier1)
{

    if ((fichier1 = fopen(FICHIER, "r")) < 0)
    {
        printf("Erreur ouverture fichier \n");
        return -1;
    }
    else
        return 0;
}

void initjoueur(liste_joueur *liste_j)
{
    FILE *f;

    int total = 0, nb = 0;

    liste_j->idjoueur = 0;

    /*
liste_j->jo.vie = 6;
liste_j->jo.nbvictoire = 0;
liste_j->jo.compteur_br = 0;
liste_j->jo.id =0;
liste_j->jo.coup2 =0;
liste_j->jo.statut =2;
*/
    //liste_j->jo.masque = 0;
}

char *masque_mot(char *mot) //Permet de masquer un mot
{
    //printf("%s\n",mot);
    for (int i = 0; mot[i] != '\0'; i++)
    {
        mot[i] = '*';
    }
    return mot;
}

void recherchemot(char *motPioche, int fdSocketCommunication) //Fonction de recherche d'un mot aleatoire se trouvant dans un fichier et l'envoie au joueur

{

    FILE *fichier = NULL;

    int nbmot = 0;        // Nombre de mot dans le dico
    int numMotChoisi = 0; //Numero mot choisi aleatoire

    int carLu = 0, nombreMax = 0;
    liste_joueur *joueur_act;

    fichier = fopen(FICHIER, "r");

    if (fichier == NULL)
    {
        printf("Erreur lors du chargement du fichier\n");
        send(fdSocketCommunication, "Erreur lors du chargement du fichier\n", 38, 0);
    }

    nbmot = total_mot_fichier(fichier, nbmot, carLu);
    nbmots_fichiers = nbmot;

    //Pioche au hasard
    srand(time(NULL));
    numMotChoisi = (rand() % nbmot);

    rewind(fichier); //Permet de revenir au début du fichier

    while (numMotChoisi > 0)
    {
        carLu = fgetc(fichier);
        if (carLu == '\n')
            numMotChoisi--;
    }
    fgets(motPioche, 100, fichier);
    motPioche[strlen(motPioche) - 1] = '\0'; //Permet d'ajouter une butée pour la lecture du mot

    printf("%s", motPioche);

    int position = 0;
    /*
joueur_act->jo->socketcom =0;
joueur_act->idjoueur = 1;

	for (int i = 0; i < joueur_act->idjoueur; i++) {
		if (joueur_act->jo->socketcom == fdSocketCommunication) {
			position = i;
		}
	}
    /*
    joueur_act->jo[position].vie = 5;
   joueur_act->jo[position].motdujeu = motPioche;
    
   
   joueur_act->jo[position].masque[strlen(motPioche)] = masque_mot(motPioche); 
    joueur_act->nbmots_fichier = nbmot;

     send(fdSocketCommunication, joueur_act->jo[position].masque,strlen(motPioche), 0);
*/
    fclose(fichier);
}

void affichage_liste_joueur(liste_joueur *liste_j)
{
    printf("%d Utilisateurs dans le jeu\n", liste_j->idjoueur);
    for (int i = 0; i < liste_j->idjoueur; i++)
    {
        printf("--Liste des joueurs--\n IP:%s Vies restantes:%i\n", inet_ntoa(coordonneesAppelant.sin_addr), liste_j->jo->vie);
    }
}

int verif_statut(char verif_lettre[], int lon_mot) //Determine si le joueur gagne ou pas
{

    int statut = 1; //
    for (int i = 0; i < lon_mot; i++)
    {
        if (verif_lettre[i] == 0)
            statut = 0;
    }
    return statut;
}

int recherche_lettre(char lettre, char *motPioche, char *verif_lettre) //Recherche si la lettre joué appartient au mot
{
    //printf("init recherchelettre : %c,%s,%s\n",lettre,motPioche,verif_lettre);
    int pos = 0;
    for (int i = 0; motPioche[i] != '\0'; i++) //Boucle qui permet de voir jusqu'à la fin du mot (cf butée)
    {
        if (lettre == motPioche[i]) //Si la lettre appartient au mot
        {
            verif_lettre[i] = 1;
            pos = i;
        }
    }
  //  printf("debug recherchelettre : %d\n",pos);
    return pos;
}
