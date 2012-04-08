#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tab[4]={0xff,0x01,0x03c,0x01};

FILE *fw,*fr;
int noct;


int mygetc(void)
{
    int a=fgetc(fr);

    if (a==EOF)
    {
        printf("erreur : fichier source incomplet\n");
        exit(EXIT_FAILURE);
    }

    return a&255;
}


int myputc(int a)
{
    fputc(a,fw);

    noct++;

    if (noct==0x4000)
    {
         fclose(fw);
         printf("ok\n");
         exit(EXIT_SUCCESS);
    }

    return 0;
}


int main(int argc,char **argv)
{
    int i,step;
    int c,n;
    char buf[256];

    printf("GetMemo7 1.0 par Sylvain HUET\n");
    printf("R‚cuperation d'une cartouche Memo7 a partir d'un fichier .k7\n");
    printf("La cartouche a du ˆtre sauvegard‚e par : SAVEM\"MEMO7\",0,&H3FFF,0\n\n");

    if ((argc<2)
      ||((strcmp(".k7",&argv[1][strlen(argv[1])-3]))
          &&(strcmp(".K7",&argv[1][strlen(argv[1])-3]))))
    {
        printf("usage : getmemo7 fichier.k7\n");
        exit(EXIT_FAILURE);
    }

    strcpy(buf,argv[1]);
    strcpy(&buf[strlen(buf)-3],".rom");

    printf("ouverture de %s\n",argv[1]);

    if ((fr=fopen(argv[1],"rb"))==NULL)
    {
        printf("fichier introuvable\n");
        exit(EXIT_FAILURE);
    }

    printf("ecriture de %s\n",buf);

    if ((fw=fopen(buf,"wb"))==NULL)
    {
        printf("‚criture impossible\n");
        exit(EXIT_FAILURE);
    }

    noct=0;
    step=0;

    while (1)
    {
        c=mygetc();

        if (c==tab[step])
            step++;
        else if (c==tab[0])
            step=1;
        else step=0;

        if (step==4)
        {
            n=mygetc();
            if (noct==0)
            {
                for(i=0;i<5;i++) mygetc();
                for(i=0;i<n-5;i++) myputc(mygetc());
            }
            else
                for(i=0;i<n;i++) myputc(mygetc());

            step=0;
        }
    }

    exit(EXIT_SUCCESS);
}

