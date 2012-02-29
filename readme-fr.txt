
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
                              version 1.8.1

    Copyright (C) 1997-2011 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
                            Jérémie Guillaume, François Mouret,
                            Samuel Devulder


1. Introduction
---------------
Teo est un émulateur du micro-ordinateur Thomson TO8 pour PC, fonctionnant
sous MSDOS, Windows et Linux. Il a été initié par Gilles Fétis et développé
par Gilles Fétis, Eric Botcazou, Alexandre Pukall, Jérémie Guillaume et
François Mouret.


2. Comment l'obtenir ?
----------------------
En le téléchargeant depuis la page:

   http://nostalgies.thomsonistes.org/teo_home.html

L'archive principale contient le programme éxécutable de l'émulateur et la
documentation complète; pour des raisons de copyright, les ROMs du TO8
nécessaires à son fonctionnement n'y sont pas incluses, vous devez les
télécharger sous la forme d'une deuxième archive et les installer dans le
même répertoire que le programme éxécutable.


3. Compatibilité avec le TO8
----------------------------
La compatibilité est proche de 100% pour les logiciels n'utilisant pas de
périphériques non émulés et ne contenant pas de protection physique. En
d'autres termes, si un logiciel ne tourne pas sous Teo, alors probablement:
- il requiert la présence d'un périphérique externe autre que la souris,
  le crayon optique, les manettes, les lecteurs de cassettes et disquettes
  (et donc il ne tournera pas tant que ce périphérique ne sera pas émulé),
- ou sa protection physique l'a fait échoué.

Je maintiens une liste des logiciels tournant sous Teo; si vous en possédez
un qui pose problème, envoyez-le moi, j'essaierai d'identifier la cause du
dysfonctionnement et je vous dirai s'il est possible d'y remédier.


4. Problèmes connus
-------------------
- la détection automatique de la carte son dans la version MSDOS peut
  échouer; vous pouvez dans ce cas spécifier manuellement les
  caractéristiques de la carte (type de carte, adresse du port, canal DMA
  et numéro d'IRQ) en éditant le fichier teo.cfg du répertoire principal.


5. Conclusion
-------------
J'espère que Teo répondra à vos attentes; n'hésitez pas à me faire part de
vos remarques et suggestions.


François Mouret
e-mail: fjjm@orange.fr

Eric Botcazou
e-mail: ebotcazou@libertysurf.fr
