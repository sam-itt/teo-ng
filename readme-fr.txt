
            TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
            TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EEEEEEEEEE      OO          OO
                  TT        EEEEEEEEEE      OO          OO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
                  TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO

                        L'émulateur Thomson TO8
                              version 1.8.2

    Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
                            Jérémie Guillaume, François Mouret,
                            Samuel Devulder


Introduction
------------
Teo est un émulateur du micro-ordinateur Thomson TO8 pour PC, fonctionnant
sous MSDOS, Windows et Linux. Il a été initié par Gilles Fétis et développé
par Gilles Fétis, Eric Botcazou, Alexandre Pukall, Jérémie Guillaume,
François Mouret et Samuel Devulder.


Comment l'obtenir ?
-------------------
En le téléchargeant depuis la page:

   http://sourceforge.net/projects/teoemulator/

L'archive principale contient le programme éxécutable de l'émulateur et la
documentation complète.


Compatibilité avec le TO8
-------------------------
La compatibilité est proche de 100% pour les logiciels n'utilisant pas de
périphériques non émulés et ne contenant pas de protection physique. En
d'autres termes, si un logiciel ne tourne pas sous Teo, alors probablement:
- il requiert la présence d'un périphérique externe autre que la souris,
  le crayon optique, les manettes, les lecteurs de cassettes et disquettes
  (et donc il ne tournera pas tant que ce périphérique ne sera pas émulé),
- ou sa protection physique l'a fait échoué.

Nous maintenons une liste des logiciels tournant sous Teo; si vous en
possédez un qui pose problème, envoyez-le nous, nous essaierons d'identifier
la cause du dysfonctionnement et vous dirons s'il est possible d'y remédier.


Problèmes connus
----------------
- la détection automatique de la carte son dans la version MSDOS peut
  échouer; vous pouvez dans ce cas spécifier manuellement les
  caractéristiques de la carte (type de carte, adresse du port, canal DMA
  et numéro d'IRQ) en éditant le fichier teo.cfg du répertoire principal.

