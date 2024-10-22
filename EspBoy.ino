#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Matrice des boutons (conservation des broches d'origine)
const int ROW_PINS[] = {7, 8, 1};  // Broches de lignes pour la matrice de boutons
const int COL_PINS[] = {20, 10, 0};  // Broches de colonnes pour la matrice de boutons

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables pour le contrôle du pixel
int pixelX = SCREEN_WIDTH / 2;
int pixelY = SCREEN_HEIGHT / 2;
int enemyX = 10;
int enemyY = 20;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200; // Délai de debounce en ms

// Variables pour le combat
int playerHealth = 10;
int enemyHealth = 10;
bool inCombat = false;
bool gameEnded = false;
bool playerWon = false;

const int speakerPin = 3; // Pin où le haut-parleur est connecté
unsigned long lastNoteTime = 0;
int currentNote = 0;

// Fréquences des notes (en Hz)
const int notes[] = { 
  523, 392, 440, 523, 440, 392, // Intro
  523, 392, 440, 523, 440, 392, 
  330, 294, 330, 349, 392, 440, // Partie suivante
  330, 294, 330, 349, 392, 440, 
  523, 392, 440, 523, 440, 392, // Reprise de l'intro
  523, 392, 440, 523, 440, 392,
  330, 294, 330, 349, 392, 440, // Partie suivante répétée
  330, 294, 330, 349, 392, 440  
};

// Durées des notes (en ms)
const int durations[] = { 
  250, 250, 250, 250, 250, 250, // Durées pour l'intro
  250, 250, 250, 250, 250, 250,
  250, 250, 250, 250, 250, 250, // Durées pour la partie suivante
  250, 250, 250, 250, 250, 250,
  250, 250, 250, 250, 250, 250, // Durées pour la reprise de l'intro
  250, 250, 250, 250, 250, 250,
  250, 250, 250, 250, 250, 250, // Durées pour la partie suivante répétée
  250, 250, 250, 250, 250, 250  
};

void setup() {
  Serial.begin(115200);

  // Initialisation de l'écran OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  drawPixel();
  
  // Configuration des boutons (matrice)
  setupButtonMatrix();
}

void loop() {
  //   for (int i = 0; i < sizeof(notes) / sizeof(notes[0]); i++) {
  //   int duration = map(i, 0, sizeof(notes) / sizeof(notes[0]) - 1, 200, 1000); // Mapper les durées entre 200 et 1000 ms
  //   tone(speakerPin, notes[i]); // Jouer la note
  //   delay(duration); // Attendre la durée de la note
  //   noTone(speakerPin); // Arrêter le son
  //   delay(100); // Pause entre les notes
  // }

  // Gérer la musique
  playMusic();

  if (inCombat && !gameEnded) {
    // Afficher l'état de combat
    displayCombat();
  } else if (gameEnded) {
    displayEndScreen(); // Afficher l'écran de fin
  } else {
    handleButtonPress();
  }
}

// Fonction pour jouer la musique
void playMusic() {
  unsigned long currentTime = millis();
  
  if (currentNote < sizeof(notes) / sizeof(notes[0])) {
    if (currentTime - lastNoteTime >= durations[currentNote]) {
      tone(speakerPin, notes[currentNote]); // Jouer la note
      lastNoteTime = currentTime; // Mettre à jour le temps de la dernière note
      currentNote++;
    }
  } else {
    currentNote = 0; // Réinitialiser pour rejouer la musique
  }
}

// Fonction pour configurer la matrice de boutons
void setupButtonMatrix() {
  for (int row = 0; row < 3; row++) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], HIGH); // Activer les lignes à HIGH
  }
  
  for (int col = 0; col < 3; col++) {
    pinMode(COL_PINS[col], INPUT_PULLUP); // Entrée avec résistance de tirage interne
  }
}

// Gestion de la pression des boutons avec debounce
void handleButtonPress() {
  for (int row = 0; row < 3; row++) {
    digitalWrite(ROW_PINS[row], LOW); // Activer la ligne actuelle

    for (int col = 0; col < 3; col++) {
      if (digitalRead(COL_PINS[col]) == LOW) {
        // Détection de pression avec debounce
        if (millis() - lastDebounceTime > debounceDelay) {
          lastDebounceTime = millis();
          if (!inCombat) {
            movePixel(row, col);
            checkForCombat();
          } else if (gameEnded) {
            resetGame(); // Réinitialiser si le jeu est terminé et un bouton est pressé
          } else {
            handleCombatInput(row, col);
          }
        }
      }
    }
    
    digitalWrite(ROW_PINS[row], HIGH); // Désactiver la ligne actuelle
  }
}

// Déplacement du pixel en fonction du bouton pressé
void movePixel(int row, int col) {
  // Haut (bouton de la rangée 0, colonne 0)
  if (row == 0 && col == 0) {
    if (pixelY > 0) pixelY--; // Limite supérieure
  }
  // Bas (bouton de la rangée 2, colonne 0)
  else if (row == 2 && col == 0) {
    if (pixelY < SCREEN_HEIGHT - 1) pixelY++; // Limite inférieure
  }
  // Gauche (bouton de la rangée 1, colonne 0)
  else if (row == 1 && col == 0) {
    if (pixelX > 0) pixelX--; // Limite gauche
  }
  // Droite (bouton de la rangée 0, colonne 1)
  else if (row == 0 && col == 1) {
    if (pixelX < SCREEN_WIDTH - 1) pixelX++; // Limite droite
  }
  handleButtonPress();
  drawPixel();
}

// Vérifier si le joueur rencontre l'ennemi pour commencer le combat
void checkForCombat() {
  if (pixelX == enemyX && pixelY == enemyY) {
    inCombat = true;
    display.clearDisplay();
    display.display();
  }
}

// Entrée de combat (sélection d'attaques)
void handleCombatInput(int row, int col) {
  if (row == 0 && col == 2) { // "Start" pour Attaque 1
    playerAttack(1);
  } else if (row == 2 && col == 2) { // "Select" pour Attaque 2
    playerAttack(2);
  }
  enemyTurn();
}

// Actions du joueur
void playerAttack(int attackType) {
  int damage = 0;
  int hitChance = random(0, 100);
  
  if (attackType == 1) { // Attaque 1 : faible, plus de chance
    if (hitChance < 80) {
      damage = 2;
      enemyHealth -= damage;
      displayMessage("Attaque A! -2 PV");
    } else {
      displayMessage("Attaque ratee!");
    }
  } else if (attackType == 2) { // Attaque 2 : puissante, moins de chance
    if (hitChance < 50) {
      damage = 6;
      enemyHealth -= damage;
      displayMessage("Attaque B! -6 PV");
    } else {
      displayMessage("Attaque ratee!");
    }
  }

  checkCombatEnd();
}

// Tour de l'ennemi
void enemyTurn() {
  int attackType = random(1, 3); // Choisir une attaque 1 ou 2 au hasard
  int damage = 0;
  int hitChance = random(0, 100);
  
  if (attackType == 1) {
    if (hitChance < 70) {
      damage = 2;
      playerHealth -= damage;
      displayMessage("Ennemi Attaque A! -2 PV");
    } else {
      displayMessage("Ennemi ratee!");
    }
  } else if (attackType == 2) {
    if (hitChance < 40) {
      damage = 3;
      playerHealth -= damage;
      displayMessage("Ennemi Attaque B! -3 PV");
    } else {
      displayMessage("Ennemi ratee!");
    }
  }
  checkCombatEnd();
}

// Affichage du combat
void displayCombat() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.print("Joueur PV: ");
  display.print(playerHealth);
  display.setCursor(0, 10);
  display.print("Ennemi PV: ");
  display.print(enemyHealth);
  display.setCursor(0, 30);
  display.print("A: Charge (2,chance+)");
  display.setCursor(0, 40);
  display.print("B: Laser  (6,chance-)");
  display.display();
  handleButtonPress();
}

// Vérifier la fin du combat
void checkCombatEnd() {
  if (playerHealth <= 0) {
    displayMessage("Vous avez perdu!");
    gameEnded = true;
    playerWon = false;
  } else if (enemyHealth <= 0) {
    displayMessage("Vous avez gagne!");
    gameEnded = true;
    playerWon = true;
  }
}

// Afficher l'écran de fin avec animation de victoire
void displayEndScreen() {
  display.clearDisplay();

  if (playerWon) {
    // Animation de Victoire : Confettis
    for (int i = 0; i < 50; i++) {
      int x = random(0, SCREEN_WIDTH);
      int y = random(0, SCREEN_HEIGHT);
      display.drawPixel(x, y, SSD1306_WHITE);
      display.display();
      delay(20);  // Petit délai pour montrer chaque "confetti"
    }
    delay(1000);  // Pause avant le message final

    // Afficher "Victoire!"
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.print("Victoire!");
    display.display();
  } else {
    // Afficher "Défaite!"
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.print("Defaite!");
    display.display();
  }

  // Attendre que le joueur appuie sur un bouton pour réinitialiser le jeu
  while (true) {
    for (int row = 0; row < 3; row++) {
      digitalWrite(ROW_PINS[row], LOW);
      for (int col = 0; col < 3; col++) {
        if (digitalRead(COL_PINS[col]) == LOW) {
          resetGame();
          return;
        }
      }
      digitalWrite(ROW_PINS[row], HIGH);
    }
  }
}

// Réinitialiser après le combat
void resetGame() {
  playerHealth = 10;
  enemyHealth = 10;
  inCombat = false;
  gameEnded = false;
  pixelX = SCREEN_WIDTH / 2;
  pixelY = SCREEN_HEIGHT / 2;
  drawPixel();
}

// Afficher un message temporaire
void displayMessage(const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print(msg);
  display.display();
  delay(1000);
  displayCombat(); // Retour à l'affichage de combat
}

// Afficher les pixels du joueur et de l'ennemi
void drawPixel() {
  display.clearDisplay();
  display.drawPixel(pixelX, pixelY, SSD1306_WHITE);
  display.drawPixel(enemyX, enemyY, SSD1306_WHITE);
  display.display();
}