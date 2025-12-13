# Makefile (Version Finalisée pour Soutenance)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -lm # Pour lier les fonctions mathématiques (expf, etc.)

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = .

# Définition des cibles des exécutables (seront créés à la racine 'ocr/')
TRAIN_TARGET = $(BINDIR)/main
TEST_TARGET = $(BINDIR)/ocr_test
RECON_BIN = $(BINDIR)/reconstruction_final

# Fichiers Objets (seront créés dans le répertoire obj/)
COMMON_OBJECTS = $(OBJDIR)/neural_net.o $(OBJDIR)/utils.o $(OBJDIR)/data.o
RECON_OBJ = $(OBJDIR)/reconstruction_final.o

# Liste complète de TOUS les objets nécessaires
ALL_OBJECTS = $(COMMON_OBJECTS) $(OBJDIR)/main.o $(OBJDIR)/ocr_test.o $(RECON_OBJ)

# --- Règles de compilation principales ---
all: $(ALL_OBJECTS) $(TRAIN_TARGET) $(TEST_TARGET) $(RECON_BIN)

# Règle générique pour la compilation des objets (.c vers obj/*.o)
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# --- 1. Règle pour l'Exécutable d'Entraînement (main) ---
$(TRAIN_TARGET): $(COMMON_OBJECTS) $(OBJDIR)/main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --- 2. Règle pour l'Exécutable de Test (ocr_test) ---
$(TEST_TARGET): $(COMMON_OBJECTS) $(OBJDIR)/ocr_test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --- 3. Règle pour l'Exécutable de Reconstruction (reconstruction_final) ---
$(RECON_BIN): $(RECON_OBJ) $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --- Règle Spéciale d'Intégration ---
reconstruct: $(RECON_BIN)
	@echo "🐍 Lancement du pont Python (conversion PNG en pixels...)"
	@python3 decoupage/bridge.py
	@echo "🧠 Lancement de la reconnaissance IA C..."
	@./$(RECON_BIN)
	@echo "✅ Fichiers finaux (grid.txt, words.txt) générés."

# Règle de nettoyage (obligatoire)
clean:
	rm -rf $(OBJDIR)
	rm -f $(TRAIN_TARGET) $(TEST_TARGET) $(RECON_BIN)
	rm -f trained_network.bin trained_xnor.bin
	rm -f grid.txt words.txt decoupage/recon_input.txt
