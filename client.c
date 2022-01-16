#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


#define PORT 6000
#define MAX_BUFFER 1000

char *alphabet[26]={"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"};
const char *EXIT = "exit";
char statutjeu = 1; //1 si partie en cours, sinon 0

void lireMessage(char tampon[]); //Lit un message
int testQuitter(char tampon[]);


int main(int argc , char const *argv[]) {
    int fdSocket;
    int nbRecu;
    struct sockaddr_in coordonneesServeur;
    int longueurAdresse;
    char tampon[MAX_BUFFER];

    fdSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (fdSocket < 0) {
        printf("socket incorrecte\n");
        exit(EXIT_FAILURE);
    }

    // On prépare les coordonnées du serveur
    longueurAdresse = sizeof(struct sockaddr_in);
    memset(&coordonneesServeur, 0x00, longueurAdresse);

    // connexion de type TCP
    coordonneesServeur.sin_family = PF_INET;
    // adresse du serveur
    inet_aton("127.0.0.1", &coordonneesServeur.sin_addr);
    // le port d'écoute du serveur
    coordonneesServeur.sin_port = htons(PORT);


    if (connect(fdSocket, (struct sockaddr *) &coordonneesServeur, sizeof(coordonneesServeur)) == -1) {
        printf("connexion impossible\n");
        exit(EXIT_FAILURE);
    }

    printf("connexion établie\n");

        
    int temp = 0; //Variable temporaire de debug
    //Fonction void 
    while (statutjeu == 1) { //
          

        nbRecu = recv(fdSocket, tampon, MAX_BUFFER, 0); //En attente du serveur :
            //Quand le client recoit un message du serveur
        if (nbRecu > 0) {
               
            if((strstr(tampon,"Bievenue au jeu du PENDU !\n") )|| (strstr(tampon,"\nRedemarrage de la partie !\n")) )  //Message d'accueil ou de relancement d'une partie
            {
                     tampon[nbRecu] = 0;
                   
                    if(temp == 0) send(fdSocket, "ok", 3, 0); //Envoie du message ok
                     printf("%s\n", tampon); //Affichage du message
                     
                     nbRecu = recv(fdSocket, tampon, MAX_BUFFER, 0); //En attente du premier mot
                     tampon[nbRecu] = 0;
                     printf("\nLe mot : %s\n",tampon); //Affichage du mot masqué envoye par le serveur
                     temp = 2;
            } 
                //Sinon
            else
            {
            tampon[nbRecu] = 0;
            printf("Recu : %s\n", tampon);
                //Comparaison arrêt du jeu 
            if(strstr(tampon,"FIN DU JEU DU PENDU \n")) statutjeu = 0;


                //On quitte la boucle
            if (testQuitter(tampon)) {   break; }
            }
        } 


         lireMessage(tampon);
    
        if (testQuitter(tampon)) {
            break; // on quitte la boucle
        }
        
        // on envoie le message au serveur
        send(fdSocket, tampon, strlen(tampon), 0);
        printf("\nMessage envoyé\n");
        
      
        
    
    }
    printf("\nFin de la connection\n");

    close(fdSocket);

    return EXIT_SUCCESS;
}


//Fonctions
void lireMessage(char tampon[]) {
  int verifchecker =1;
    do
    {
    
    printf("Saisir une lettre a envoyer (minuscule seulement) : \n");
    fgets(tampon, MAX_BUFFER, stdin);
    strtok(tampon, "\n");

    

        //Verif taille de la lettre saisie
    if(strlen(tampon) != 1)   { printf("\nATTENTION : Erreur de saisie, seulement une lettre doit etre envoyee \n");}
    
    else{

        
    /*Controle si la lettre appartient a l'alphabet*/
        do{
        for(int k=0; k <= 25 ;k++)
        {
        if((strcmp(tampon,alphabet[k])) == 0 && strlen(tampon) == 1)    {verifchecker = 0;}
        }
        }
        while(verifchecker == 1);
    
    }
    } while (strlen(tampon) != 1);
}

int testQuitter(char tampon[]) {
    return strcmp(tampon, EXIT) == 0;
}
