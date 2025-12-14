# Compilateur
CC = gcc

# Options de compilation
# -Wall : affiche tous les avertissements
# -O2   : optimisation du code
CFLAGS = -Wall -O2

# Bibliothèques à lier
# -lm : bibliothèque mathématique (nécessaire pour sqrt, pow, etc.)
LIBS = -lm

# Liste des exécutables à produire
TARGETS = detection words_cut letter_cut

# Règle par défaut (exécutée si on tape juste "make")
all: $(TARGETS)

# Règles de compilation pour chaque programme
detection: detection.c stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) detection.c -o detection $(LIBS)

words_cut: words_cut.c stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) words_cut.c -o words_cut $(LIBS)

letter_cut: letter_cut.c stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) letter_cut.c -o letter_cut $(LIBS)

# Règle pour nettoyer les fichiers générés (exécutables)
clean:
	rm -f $(TARGETS)
	# Si vous êtes sous Windows, remplacez la ligne ci-dessus par :
	# del /f $(TARGETS).exe

# Règle pour nettoyer tout (y compris les dossiers de résultats)
clean_results: clean
	rm -rf grid.png words.png letters words words_letters
	# Sous Windows :
	# rmdir /s /q letters words words_letters 2>nul
	# del grid.png words.png 2>nul
