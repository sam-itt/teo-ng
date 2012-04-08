#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int mygetc(FILE *f)
{
  if (fgetc(f)==EOF) return EOF;
  return fgetc(f);
}	

int myputc(int c,FILE *f)
{
  return fputc(c,f);
}	

int main(int argc, char **argv)
{
  FILE *fp,*fw;
  int i,l;
  int a,b,c;
  int octet;
  int actuel;
  int state,k=0;
  int j;
  int tampon;
  char *filename;
  char outputname[1024];
  
  printf("conversion .wav 44,1KHz -> .k7 to7\n");
  printf("version 2.1\n");
  
  if (argc<2)
    {
      printf("usage : wav2k7 fichier.wav\n");
      return 0;
    }
  filename=argv[1];

  l=strlen(filename)-4;
  
  if ((l<1)||((strcmp(&filename[l],".wav"))&&(strcmp(&filename[l],".WAV"))))
    {
      printf("usage : mauvais format\n");
      return 0;
    }
  
  printf("ouverture de %s\n",filename);
  fp=fopen(filename,"rb");
  if (fp==NULL)
    {
      printf("fichier %s introuvable\n",filename);
      return 0;
    }
  
  strcpy(outputname,filename);
  strcpy(outputname+l,".k7\0");
  printf("creation de %s\n",outputname);
  fw=fopen(outputname,"wb");
  if (fw==NULL)
    {
      printf("fichier %s impossible a ouvrir\n",outputname);
      return 0;
    }
  
  for(i=0;i<44;i++) fgetc(fp);
  
  
  a=mygetc(fp);
  b=mygetc(fp);
  l=0;
  i=0;
  
  tampon=0;
  j=0;
  
  octet=-1;
  actuel=1;
  state=0; 
  /* 0:attente de 0, 1:1ere boucle, 2: lecture de l'octet, 3:2e boucle */
  while(1)
    {
      if (tampon==0)
        {
          do
            {
              if ((c=mygetc(fp))==EOF)
                {
                  fclose(fp);
                  fclose(fw);
                  return 0;
                }	
              i++;
              tampon++;
                
                if ((b>=a)&&(b>c))
                  {
                    if (i==3) actuel=1;
                    if ((i==4)&&(l==4)) actuel=1;
                    if ((i==5)||(i==6)) actuel=0;
                    if (i>6) actuel=1;
                    l=i;
                    i=0;
                  }
              a=b; b=c;
            }
          while(i);
        }
      tampon--;
      
      switch(state) 
        {
        case 0:
          k=0;
          if (actuel==0) state=1;
          break;
        case 1:
          k++;
          if (k==12)
            {
              k=0;
              octet=256;
              if (actuel==0) state=2;
              else state=0;
            }
          break;
        case 2:
          k++;
          if ((k==24)||(k==49))
            {
              octet=(octet>>1)+256*actuel;
              if (octet&1)
                {	
                  octet>>=1;
                  myputc(octet,fw);
                  k=0;
                  state=3;
                  j++;
                }
            }
          if (k==49) k=0;
          break;
        case 3:
          k++;
          if (k==24) state=0;
          break;
        } 
    } 
}
