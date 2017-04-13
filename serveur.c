#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "serveur.h" // contient les prototypes des fonctions et les defines

void main(int argc, char *argv[])
{
int sd;
struct sockaddr_in sa;		/* Structure Internet sockaddr_in */
struct hostent *hptr ; 		/* Infos sur le serveur */
int port;        		/* Numero de port du serveur */

int newsd;			/* Id de la socket entrante */
struct sockaddr_in newsa;	/* sockaddr_in de la connection entrante */
int newsalength;
struct hostent *newhptr; 	/* Infos sur le client suivant /etc/hosts */


/* verification du nombre d'arguments de la ligne de commande */
if (argc != 2) {
	printf("pingserveurTCP. Erreur d'arguments\n");
	printf("Syntaxe : %% pingserveurTCP numero_port\n");
	exit(1);
}

/* Recuperation numero port passe en argument */
port = atoi(argv[1]);

printf("\n\tProjet Mini-Serveur Web\npar Forest Fabien et Juglin Guillaume\n");


/* Initialisation la structure sockaddr sa avec les infos  formattees : */

/* Famille d'adresse : AF_INET = PF_INET */
sa.sin_family = AF_INET;

/* Initialisation du numero du port */
sa.sin_port = htons(port);
sa.sin_addr.s_addr = INADDR_ANY;

/* Creation de la socket TCP */
if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	printf("Probleme lors de la creation de socket\n");
	exit(1);
}

/* Bind sur la socket */
if(bind(sd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	printf("Probleme avec le bind\n");
	exit(1);
}

/* Initialisation de la queue d'ecoute des requetes (5 max) */
listen(sd, 5);

printf("Serveur pingTCP en ecoute...\n");

while(1) {

/* Ne pas oublier : newsalength contient la taille de la structure sa attendue */
	newsalength = sizeof(newsa) ;

	if((newsd = accept(sd, ( struct sockaddr * ) &newsa, &newsalength)) < 0 ) { // Ouverture du socket
		printf("Erreur sur accept\n");
		exit(1);
	}
	reception(&newsd); // On réceptionne et on répond
	close(newsd); // On ferme le socket
}

/* Fermeture du serveur. Never reached */
close(sd);

printf("Fermeture du Serveur\n");
exit(0);
}

void reception(int *newsd)
{
	char buf[1024];
 	char name[256];
 	char c;
 	int n, i;

	n = read(*newsd, buf, sizeof(buf)); // On stock le message reçu
	i = 0;
	printf("Message reçu :\n%s\n", buf);
	if(n > 5 && buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T') // On vérifie que le message est plus grand que 5 octet et qu'il commence par GET
	{
		do // On récupère le nom de la ressource demandée
		{
			name[i] = buf[i + 5];
			i++;
		}while(name[i - 1] != ' ');
	name[i - 1] = '\0';
	printf("\nNom de la ressource demandee : %s\n", name);

	n = detectExtensionType(name); // On appelle une fonction pour déterminer le type de ressource

        if(n == 2 && i > 1) sendFile(newsd, "erreur403.html", TEXTE); // C'est une requête dossier, on interdit l'accès aux dossiers seuls
	else if(n == 0 && strstr(name, "cgi-bin/") != NULL) launchScript(newsd, name); // Si name contient "cgi-bin/", on demande quelque chose dans le répertoire cgi-bin, c'est un script
	else if(n == 0 && name[0] == '\0') sendFile(newsd, "index.html", TEXTE); // Sinon on envoi la page d'acceuil si le nom est vide
	else sendFile(newsd, name, n); // Sinon c'est une demande de fichier texte, d'image ou de page, on l'envoie
	}
	else sendFile(newsd, "erreur400.html", 0);	 // Sinon envoi de l'erreur "400 Bad Request"
}
// ----------------------------------------------------------------------------------------------------------------------------
void sendFile(int *newsd, const char* name, int type)
{
  int n, i;
  int longueur;
  char *envoi;
  char *content;
  char extension[10];
  FILE* fichier = NULL;

  fichier = fopen(name, "r"); // On ouvre le fichier demandé
  if(fichier != NULL)
  {
	longueur = fileLength(name); // On récupère sa taille

	envoi = malloc(sizeof(char) * longueur + 400); // On créé une chaîne assez grande pour contenir le texte et l'en-tête

	if(!getExtension(name, extension)) // On récupère l'extension
		sprintf(envoi, "HTTP/1.0 200 OK\n\
Content-Type: text/html; charset=iso-8859-1\n\
Content-Lenght: %d\n\n\0", longueur); // Si il n'y a pas d'extension, texte html par défaut
	else if(type == 1)
sprintf(envoi, "HTTP/1.1 200 OK\n\
Content-Type: image/%s\n\
Content-Lenght: %d\n\n\0", extension, longueur);
	else sprintf(envoi, "HTTP/1.0 200 OK\n\
Content-Type: text/%s; charset=iso-8859-1\n\
Content-Lenght: %d\n\n\0", extension, longueur); // Sinon on précise l'extension (html, css, etc...)

n = strlen(envoi);
i = 0;
do{
   envoi[i + n] = fgetc(fichier); // et on ajoute à la suite de l'en-tête les octets du fichier image, un à un
   i++;			      // à noter que les fonctions de string.h (strcat, strlen, etc...) ne peuvent pas être utilisé car elles utilisent '\0'
}while(!feof(fichier));	      // or, ce caractère est parfois présent dans les fichiers image puisque ce n'est pas du texte mais des données binaires

  	fclose(fichier);
  }
  else // Sinon on prépare une page "404 Not Found"
  {
	envoi = malloc(sizeof(char) * 600);
	sprintf(envoi, "HTTP/1.0 200 OK\n\
Content-Type: text/html; charset=iso-8859-1\n\n\
<html>\n\
<TITLE> 404 NOT FOUND\n\
</TITLE>\n\
</head>\n\
<body>\n\
<h1>\n\
404 Not Found\n\
</h1>\n\
The requested URL was not found on this server.\n\
<p>\n\
</p>\n\
<hr>\n\
<address>\n\
Server at localhost Port superior at 2000\n\
</address>\n\
</body>\n\
</html>");
n = strlen(envoi);
i = 1;
  }
  write(*newsd, envoi, n + i - 1); // On l'envoi
free(envoi);
}
// ----------------------------------------------------------------------------------------------------------------------------
void launchScript(int *newsd, char* name)
{
    char chaine[256];
    int i = 0;
    FILE* fichier = NULL;

    do{
	if(name[i] == '?') name[i] = ' '; // On remplace les ? par des espaces, les arguments seront ainsi séparés du nom du script par des espaces
	i++;
    }while(name[i] != '\0');

    sprintf(chaine, "./%s 1> trace", name); // On prépare la commande système
    printf("%s\n", chaine);
    system(chaine); // Et on l'éxécute et on stock le résultat dans "trace"

    fichier = fopen("trace", "r"); // On ouvre "trace"
    if(fichier == NULL) sendFile(newsd, "erreur500.html", TEXTE); // Si impossible, erreur "500 Internal Server Error"
    else { // Sinon
	fgetc(fichier); // On test pour savoir si la fin du fichier est atteinte
	if(feof(fichier)) sendFile(newsd, "erreurScript.html", TEXTE); // Si le fichier est vide, le script n'a rien donné
	else sendFile(newsd, "trace", TEXTE); // Sinon on envoi ensuite trace
	fclose(fichier);

	system("rm trace");
	}
}

// ----------------------------------------------------------------------------------------------------------------------------
int detectExtensionType(const char name[])
{
    char extension[10] = "\0";

    if(name[strlen(name) - 1] == '/') // Si le dernier caractère du nom est un /, c'est une demande de dossier
		return OTHER; // Dossier

    getExtension(name, extension);

    if(strcmp(extension, "png\0") == 0 || // On vérifie si une extension d'image est présente
	strcmp(extension, "bmp\0") == 0 ||
	strcmp(extension, "jpg\0") == 0 ||
	strcmp(extension, "jpeg\0") == 0 ||
	strcmp(extension, "gif\0") == 0 ||
	strcmp(extension, "tga\0") == 0 ||
	strcmp(extension, "ico\0") == 0 ||
	strcmp(extension, "tiff\0") == 0)
		return IMAGE; // Fichier Image

	return TEXTE; // Sinon fichier texte
}
// ----------------------------------------------------------------------------------------------------------------------------
int getExtension(const char name[], char extension[])
{
    int i, j;
    j = 0;
    for(i = 0; i < strlen(name); i++)// On cherche le dernier . de la chaîne (au cas où il y en aurait plusieurs)
    	{
    	if(name[i] == '.') j = i; // et on mémorise le caractère dans j
   	 }

    if(j == 0) return 0; // Si j=0, aucune extension

    for(i = j + 1; i < strlen(name); i++) // On stock ensuite l'extension dans la chaîne "extension"
        extension[i - j - 1] = name[i];
    extension[i - j - 1] = '\0';

   return 1;
}
// ----------------------------------------------------------------------------------------------------------------------------
int fileLength(const char name[])
{
int i = 0;
char c;
FILE* fichier = NULL;

fichier = fopen(name, "r");
if(fichier == NULL) exit(0);
do
    {
    fread(&c, sizeof(char), 1, fichier);
    i++;
    }while(!feof(fichier));
fclose(fichier);
return i - 1;
}
