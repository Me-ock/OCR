# OCR
Epita's project OCR


# Come 

Creation de deux dossier ->  src : qui regroupe le reseau de neuronne (neural) et des datas pour train notre reseau  
			

			 -> include : nos fichier header 



Avancer : L'ia reconnais sur des matrices de 3x3 et 5x5 et differencie A et b (on commence petit) les tests sont concluants mais elle apprend encore . 
La premiere difficultee est la documentation et comprendre comment entrainer son ia 





2 eme push : IA terminee avec sauvegarde dans un fichier du reseau de neuronne....probleme majeur les data ne sont pas asser fourni et l'IA fait du surapprentissage ce qui fausse les resultats sur la deuxieme moitier de l'alphabet....




        -> src : ./demo_ocr 
		./test_ocr 


  ce que font ces fichier build : demo cree un reseau de neuronne sur 10 000 epoch avec 256 neuronnes cachee..--> suraprentissage je dois elargire la base de donnee.


        -> : poc_XNOR.c



	verifie pour un reseau de neuronne en 2,4,1 (2 couches vis , 4 couches cach , 1 sortie) que notre modele apprend bien et converge vers des resultats satisfesnant qui sont sauvegarder 
	





pour tester ces programe : Make (ce que vous voulez faire TestOCR/DEMOCR/POCXNOR) -> ./le fichier 


a noter que demo et test sont des builds complementaire, vous cree un reseau de neuronne avec demo qui apprend sur une base donner fictive (pas les images ) et enfin vous test son taux de reussites
