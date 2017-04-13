#define TEXTE 0
#define IMAGE 1
#define OTHER 2

void reception(int *newsd);
void sendFile(int *newsd, const char *name, int type);
void launchScript(int *newsd, char* name);
int detectExtensionType(const char name[]);
int getExtension(const char name[], char extension[]);
int fileLength(const char name[]);
