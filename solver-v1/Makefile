# Compilateur
CC = gcc

# Options de compilation
# -Wall -Wextra : Affiche tous les avertissements (très utile pour éviter les bugs)
# -O2           : Optimisation du code pour la rapidité
# -g            : Ajoute des infos de débogage (utile si vous utilisez gdb ou valgrind)
CFLAGS = -Wall -Wextra -O2 -g

# Nom de l'exécutable final
TARGET = solver

# Fichier source
SRC = solver.c

# Règle par défaut (exécutée quand on tape juste "make")
all: $(TARGET)

# Règle de compilation
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Règle pour nettoyer les fichiers générés (exécutables)
clean:
	rm -f $(TARGET)
	# Sous Windows (PowerShell/CMD), remplacez la ligne ci-dessus par :
	# del /f $(TARGET).exe

# Indique que 'all' et 'clean' ne sont pas des fichiers
.PHONY: all clean
