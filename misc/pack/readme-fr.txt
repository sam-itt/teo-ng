
                            CREATION DES PACKAGES

Les scripts suivants permettent de de créer facilement les packages
pour la distribution de Teo et de Cc90hfe.
'pack.sh' doit être exécuté en dernier.

djmake.bat
----------

Lancer 'misc\pack\djmake.bat' à partir du répertoire 'teoemulator-code\'.
'djmake.bat' compile les exécutables MSDOS dans les répertoires
'misc\pack\msdos\en' et 'misc\pack\msdos\fr' (les répertoires seront créés
s'ils n'existent pas).

mgwmake.bat
-----------

Lancer 'misc\pack\mgwmake.bat' à partir du répertoire 'teoemulator-code\'.
'mgwmake.bat' compile les exécutables dans le répertoire 'misc\pack\mingw\en'
et 'misc\pack\mingw\fr'  (les répertoires seront créés s'ils n'existent pas).
Le package auto-extractible sera créé en double-cliquant et compilant le
fichier '.iss'. Inno Setup doit être installé (http://www.innosetup.com/).

'inno\teo-big-img.bmp' et 'inno\teo-small-img.bmp' sont utilisés
par les fichiers '*.iss' pour décorer la fenêtre de l'installeur.

pack.sh
-------

Lancer './misc/pack/pack.sh' à partir du répertoire 'teoemulator-code/'.

Tous les packages sont alors dans le répertoire 'misc/pack/'.

