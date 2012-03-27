
                            CREATION DES PACKAGES

Les scripts suivants permettent de de créer facilement les packages
pour la distribution de Teo.
'pack.sh' doit être exécuté en dernier.

djmake.bat
----------

Lancer 'misc\pack\djmake.bat' à partir du répertoire 'teo\'.
'djmake.bat' compile les exécutables MSDOS en Français et en Anglais
dans le répertoire 'misc\pack\msdos' (le répertoire est créé si il
n'existe pas).

mgwmake.bat
-----------

Lancer 'misc\pack\mgwmake.bat' à partir du répertoire 'teo\'.
'mgwmake.bat' compile les exécutables 'teow-en.exe' et 'teow-fr.exe'
dans le répertoire 'misc\pack\inno'. Les packages auto-extractibles
seront alors créés en double-cliquant et compilant les fichiers '*.iss'.
Inno Setup doit être installé (http://www.innosetup.com/).

'inno\teo-big-img.bmp' et 'inno\teo-small-img.bmp' sont utilisés
par les fichiers '*.iss' pour décorer la fenêtre de l'installeur.

pack.sh
-------

Lancer './misc/pack/pack.sh' à partir du répertoire 'teo/'.

A l'exécution, 'pack.sh' :
- Compile l'exécutable Linux en mode DEBIAN
- Crée le DEBIAN distribuable Linux pour l'exécutable
- Compile l'exécutable Linux pour le TAR.GZ
- Crée le TAR.GZ distribuable Linux
- Crée le ZIP distribuable Window en Français
- Crée le ZIP distribuable Window en Anglais
- Crée le ZIP distribuable MsDos en Français
- Crée le ZIP distribuable MsDos en Anglais
- Crée le ZIP pour les sources
- Crée le TAR.GZ pour les sources

Tous les packages sont alors dans le répertoire 'misc/pack/'.

