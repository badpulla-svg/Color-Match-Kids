#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>

// Español
#define T_FACIL_ES       1
#define T_MEDIO_ES       2
#define T_DIFICIL_ES     3
#define T_NUEVO_ES       4
#define T_INTENTA_ES     5
#define T_ACIERTO_ES     16
#define T_VICTORIA_ES    18
#define T_COLOR_ROJO_ES  20
#define T_COLOR_AZUL_ES  22
#define T_COLOR_VERDE_ES 24

// Kichwa
#define T_FACIL_KQ       6
#define T_MEDIO_KQ       7
#define T_DIFICIL_KQ     8
#define T_NUEVO_KQ       9
#define T_INTENTA_KQ     10
#define T_ACIERTO_KQ     17
#define T_VICTORIA_KQ    19
#define T_COLOR_ROJO_KQ  21
#define T_COLOR_AZUL_KQ  23
#define T_COLOR_VERDE_KQ 25

// INGLES
#define T_FACIL_EN       26   
#define T_MEDIO_EN       27   
#define T_DIFICIL_EN     28   

#define T_NUEVO_EN       29   
#define T_INTENTA_EN     30   
#define T_IDIOMA_EN      31   

#define T_ACIERTO_EN     35   
#define T_VICTORIA_EN    36   
#define T_APLAUSOS       37   

#define T_COLOR_ROJO_EN  32   
#define T_COLOR_AZUL_EN  33   
#define T_COLOR_VERDE_EN 34   

// Idiomas y audio on/off
#define T_IDIOMA_ES      11
#define T_IDIOMA_KQ      12
#define T_IDIOMA_COMB    13   // combinado ES+KQ+EN
#define T_AUDIO_ON       14
#define T_AUDIO_OFF      15

// TIRAS LEDS
#define LED_PIN       6
#define NUM_LEDS      72
#define USED_LEDS     72
#define BRIGHTNESS    180
#define LEDS_POR_SECCION  8

CRGB leds[NUM_LEDS];

// PULSANTES
#define BUTTON_PIN        2      
#define BUTTON_IDIOMA     3      

// CONEXION BLUETOOTJ
#define BT Serial1   

// MODULO DFPLAYER
DFRobotDFPlayerMini mp3;
bool dfOk = false;

// ESTADOS GLOBAL
int modo = 0;          // 0=fácil, 1=medio, 2=difícil
int idioma = 0;        // 0=ES, 1=KQ, 2=COMB (ES+KQ+EN), 3=EN
bool audioHabilitado = true;

int seccionColor[9];   // patrón del tablero

unsigned long tPressed = 0;
bool prevButton = HIGH;

unsigned long idiomaPressStart = 0;

// DETECCION DE COLOR 
// Casillas 0 a 8
// ROJO -> A en GND
// AZUL -> B en GND
// VERDE -> A y B en GND

const int pinA[9] = {30, 32, 34, 36, 38, 40, 42, 44, 46};
const int pinB[9] = {31, 33, 35, 37, 39, 41, 43, 45, 47};

// -1=vacío, 0=rojo, 1=azul, 2=verde
int jugadorColor[9];
int ultimoJugadorColor[9];
bool victoriaAnunciada = false;


void setup() {

  Serial.begin(9600);
  BT.begin(9600);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_IDIOMA, INPUT_PULLUP);

  // Entradas del tablero
  for (int i = 0; i < 9; i++) {
    pinMode(pinA[i], INPUT_PULLUP);
    pinMode(pinB[i], INPUT_PULLUP);
    jugadorColor[i] = -1;
    ultimoJugadorColor[i] = -1;
  }

  // Modulo DFPlayer
  Serial2.begin(9600);
  if (!mp3.begin(Serial2)) {
    Serial.println("⚠ DFPlayer no inició");
    dfOk = false;
  } else {
    Serial.println("✔ DFPlayer listo");
    mp3.volume(30);
    dfOk = true;
  }

  generarPatron();
  mostrarPatron();
}

// LOOP

void loop() {
  leerBoton();
  leerBotonIdioma();
  leerBluetooth();
  actualizarTableroJugador();   // detección de fichas
}



// BOTÓN PRINCIPAL (PATRÓN Y  MODOS)
 
void leerBoton() {

  bool estado = digitalRead(BUTTON_PIN);

  if (prevButton == HIGH && estado == LOW)
    tPressed = millis();

  if (prevButton == LOW && estado == HIGH) {

    unsigned long d = millis() - tPressed;

    if (d < 800) {
      generarPatron();
      mostrarPatron();
      reproducirNuevoPatron();
    } else {
      cambiarModo();
    }
  }

  prevButton = estado;
}



// BOTÓN DE IDIOMA: CICLO ES → KQ → COMB → EN + MUTE

void leerBotonIdioma() {

  bool pressed = !digitalRead(BUTTON_IDIOMA);

  if (pressed && idiomaPressStart == 0)
    idiomaPressStart = millis();

  if (pressed && idiomaPressStart != 0) {

    // mantener 3s → mute/unmute
    if (millis() - idiomaPressStart > 3000) {

      audioHabilitado = !audioHabilitado;
      idiomaPressStart = 0;

      if (dfOk) {
        if (audioHabilitado) mp3.play(T_AUDIO_ON);
        else mp3.play(T_AUDIO_OFF);
      }
      return;
    }
  }

  // toque corto → cambiar idioma
  if (!pressed && idiomaPressStart != 0) {

    unsigned long d = millis() - idiomaPressStart;
    idiomaPressStart = 0;

    if (d < 3000 && d > 80) {

      idioma++;
      if (idioma > 3) idioma = 0;

      if (dfOk && audioHabilitado) {
        if (idioma == 0) mp3.play(T_IDIOMA_ES);
        else if (idioma == 1) mp3.play(T_IDIOMA_KQ);
        else if (idioma == 2) mp3.play(T_IDIOMA_COMB);
        else if (idioma == 3) mp3.play(T_IDIOMA_EN);
      }

      Serial.print("Idioma = ");
      Serial.println(idioma);
    }
  }
}


 
// BLUETOOTH

void leerBluetooth() {

  if (!BT.available()) return;

  char c = BT.read();
  Serial.print("BT: ");
  Serial.println(c);

  // ----- IDIOMAS -----
  if (c == 'E') { idioma = 0; if (dfOk && audioHabilitado) mp3.play(T_IDIOMA_ES);  return; }
  if (c == 'K') { idioma = 1; if (dfOk && audioHabilitado) mp3.play(T_IDIOMA_KQ);  return; }
  if (c == 'C') { idioma = 2; if (dfOk && audioHabilitado) mp3.play(T_IDIOMA_COMB);return; }
  if (c == 'I') { idioma = 3; if (dfOk && audioHabilitado) mp3.play(T_IDIOMA_EN);  return; }

  // ----- AUDIO ON/OFF DESDE APP -----
  if (c == 'S') {  // 'S' de Sonido
    audioHabilitado = !audioHabilitado;
    if (dfOk) {
      if (audioHabilitado) mp3.play(T_AUDIO_ON);
      else mp3.play(T_AUDIO_OFF);
    }
    Serial.print("Audio habilitado: ");
    Serial.println(audioHabilitado);
    return;
  }

  // ----- MODOS -----
  if (c == 'F') { modo = 0; indicadorModo(); generarPatron(); mostrarPatron(); reproducirModo(); return; }
  if (c == 'M') { modo = 1; indicadorModo(); generarPatron(); mostrarPatron(); reproducirModo(); return; }
  if (c == 'D') { modo = 2; indicadorModo(); generarPatron(); mostrarPatron(); reproducirModo(); return; }

  // ----- NUEVO PATRÓN -----
  if (c == 'N') { generarPatron(); mostrarPatron(); reproducirNuevoPatron(); return; }
}



// CAMBIO DE MODO

void cambiarModo() {
  modo = (modo + 1) % 3;
  indicadorModo();
  generarPatron();
  mostrarPatron();
  reproducirModo();
}



// INDICADOR VISUAL DEL MODO

void indicadorModo() {

  CRGB color;

  if (modo == 0) color = CRGB::Green;
  if (modo == 1) color = CRGB::Blue;
  if (modo == 2) color = CRGB::Red;

  for (int i = 0; i < 3; i++) {
    fill_solid(leds, USED_LEDS, color);
    FastLED.show();
    delay(350);
    FastLED.clear();
    FastLED.show();
    delay(250);
  }
}



// GENERAR PATRÓN

void generarPatron() {

  int cantidad;
  if (modo == 0) cantidad = 3;
  if (modo == 1) cantidad = 5;
  if (modo == 2) cantidad = 9;

  for (int i = 0; i < 9; i++) {
    seccionColor[i] = -1;
    jugadorColor[i] = -1;
    ultimoJugadorColor[i] = -1;
  }

  victoriaAnunciada = false;

  for (int c = 0; c < cantidad; c++) {
    int pos;
    do { pos = random(0, 9); } while (seccionColor[pos] != -1);
    seccionColor[pos] = random(0, 3);  // 0 R, 1 B, 2 G
  }
}



// PATRÓN

void mostrarPatron() {

  FastLED.clear();

  for (int s = 0; s < 9; s++) {

    int id = seccionColor[s];
    if (id == -1) continue;

    CRGB color;

    if (id == 0) color = CRGB::Red;
    if (id == 1) color = CRGB::Blue;
    if (id == 2) color = CRGB::Green;

    int inicio = s * LEDS_POR_SECCION;

    for (int i = 0; i < LEDS_POR_SECCION; i++)
      leds[inicio + i] = color;
  }

  FastLED.show();
}



// AUDIO: NUEVO PATRÓN

void reproducirNuevoPatron() {

  if (!dfOk || !audioHabilitado) return;

  if (idioma == 0) mp3.play(T_NUEVO_ES);
  else if (idioma == 1) mp3.play(T_NUEVO_KQ);
  else if (idioma == 3) mp3.play(T_NUEVO_EN);
  else if (idioma == 2) {
    mp3.play(T_NUEVO_ES); delay(2000);
    mp3.play(T_NUEVO_KQ); delay(2000);
    mp3.play(T_NUEVO_EN);
  }
}



// AUDIO: MODO (FACIL/MEDIO/DIFICIL)

void reproducirModo() {

  if (!dfOk || !audioHabilitado) return;

  // Español
  if (idioma == 0) {
    if (modo == 0) mp3.play(T_FACIL_ES);
    if (modo == 1) mp3.play(T_MEDIO_ES);
    if (modo == 2) mp3.play(T_DIFICIL_ES);
  }
  // Kichwa
  else if (idioma == 1) {
    if (modo == 0) mp3.play(T_FACIL_KQ);
    if (modo == 1) mp3.play(T_MEDIO_KQ);
    if (modo == 2) mp3.play(T_DIFICIL_KQ);
  }
  // Combinado ES + KQ + EN
  else if (idioma == 2) {
    if (modo == 0) {
      mp3.play(T_FACIL_ES); delay(2000);
      mp3.play(T_FACIL_KQ); delay(2000);
      mp3.play(T_FACIL_EN);
    }
    if (modo == 1) {
      mp3.play(T_MEDIO_ES); delay(2000);
      mp3.play(T_MEDIO_KQ); delay(2000);
      mp3.play(T_MEDIO_EN);
    }
    if (modo == 2) {
      mp3.play(T_DIFICIL_ES); delay(2000);
      mp3.play(T_DIFICIL_KQ); delay(2000);
      mp3.play(T_DIFICIL_EN);
    }
  }
  // Inglés
  else if (idioma == 3) {
    if (modo == 0) mp3.play(T_FACIL_EN);
    if (modo == 1) mp3.play(T_MEDIO_EN);
    if (modo == 2) mp3.play(T_DIFICIL_EN);
  }
}

// LECTURA DE UNA CASILLA DEL TABLERO DEL GUAGUA

int leerCasilla(int c) {
  int a = digitalRead(pinA[c]);
  int b = digitalRead(pinB[c]);

  // Pull-up: HIGH = sin conectar, LOW = conectado a GND
  if (a == HIGH && b == HIGH) return -1; // vacío
  if (a == LOW  && b == HIGH) return 0;  // rojo
  if (a == HIGH && b == LOW ) return 1;  // azul
  if (a == LOW  && b == LOW ) return 2;  // verde

  return -1;
}

// ACTUALIZAR TABLERO: ACIERTO / ERROR / VICTORIA

void actualizarTableroJugador() {

  // Leer todas las casillas
  for (int i = 0; i < 9; i++) {
    jugadorColor[i] = leerCasilla(i);
  }

  // Detectar cambios
  for (int i = 0; i < 9; i++) {

    if (jugadorColor[i] != ultimoJugadorColor[i]) {

      int nuevo = jugadorColor[i];
      ultimoJugadorColor[i] = nuevo;

      // Si quedó vacío, no hacemos nada
      if (nuevo == -1) continue;

      // Si el patrón no requiere color en esa casilla → error
      if (seccionColor[i] == -1) {
        reproducirError();
        continue;
      }

      // Si el color no coincide → error
      if (nuevo != seccionColor[i]) {
        reproducirError();
        continue;
      }

      // Si llegó aquí, acierto
      reproducirAcierto(nuevo);
    }
  }

  // Verificar si ya completó el patrón
  
if (!victoriaAnunciada) {
  bool completo = true;

  for (int i = 0; i < 9; i++) {

    if (seccionColor[i] == -1) {
      continue; // casillas que no forman parte del patrón
    }

    if (jugadorColor[i] != seccionColor[i]) {
      completo = false;
      break;
    }
  }

  if (completo) {

  if (modo == 0 || modo == 1) {   // si es fácil o medio
     reproducirAplausos();
     } else {                        // si es difícil
    reproducirVictoria();
  }
    victoriaAnunciada = true;
  }
}
}



// AUDIO: ERROR

void reproducirError() {

  if (!dfOk || !audioHabilitado) return;

  if (idioma == 0) mp3.play(T_INTENTA_ES);
  else if (idioma == 1) mp3.play(T_INTENTA_KQ);
  else if (idioma == 3) mp3.play(T_INTENTA_EN);
  else if (idioma == 2) {
    mp3.play(T_INTENTA_ES); delay(2000);
    mp3.play(T_INTENTA_KQ); delay(2000);
    mp3.play(T_INTENTA_EN);
  }
}
 
// AUDIO: ACIERTO (MODO FÁCIL = COLOR / MEDIO-DIFÍCIL = "MUY BIEN")

void reproducirAcierto(int colorId) {

  if (!dfOk || !audioHabilitado) return;

  // Modo FÁCIL
  if (modo == 0) {

    // Español
    if (idioma == 0) {
      if (colorId == 0) mp3.play(T_COLOR_ROJO_ES);
      if (colorId == 1) mp3.play(T_COLOR_AZUL_ES);
      if (colorId == 2) mp3.play(T_COLOR_VERDE_ES);
    }
    // Kichwa
    else if (idioma == 1) {
      if (colorId == 0) mp3.play(T_COLOR_ROJO_KQ);
      if (colorId == 1) mp3.play(T_COLOR_AZUL_KQ);
      if (colorId == 2) mp3.play(T_COLOR_VERDE_KQ);
    }
    // Inglés
    else if (idioma == 3) {
      if (colorId == 0) mp3.play(T_COLOR_ROJO_EN);
      if (colorId == 1) mp3.play(T_COLOR_AZUL_EN);
      if (colorId == 2) mp3.play(T_COLOR_VERDE_EN);
    }
    // Combinado ES + KQ + EN
    else if (idioma == 2) {
      if (colorId == 0) {
        mp3.play(T_COLOR_ROJO_ES); delay(1500);
        mp3.play(T_COLOR_ROJO_KQ); delay(1500);
        mp3.play(T_COLOR_ROJO_EN);
      }
      if (colorId == 1) {
        mp3.play(T_COLOR_AZUL_ES); delay(1500);
        mp3.play(T_COLOR_AZUL_KQ); delay(1500);
        mp3.play(T_COLOR_AZUL_EN);
      }
      if (colorId == 2) {
        mp3.play(T_COLOR_VERDE_ES); delay(1500);
        mp3.play(T_COLOR_VERDE_KQ); delay(1500);
        mp3.play(T_COLOR_VERDE_EN);
      }
    }
  }
  // MODO MEDIO / DIFÍCIL: solo "muy bien"
  else {

    if (idioma == 0) mp3.play(T_ACIERTO_ES);
    else if (idioma == 1) mp3.play(T_ACIERTO_KQ);
    else if (idioma == 3) mp3.play(T_ACIERTO_EN);
    else if (idioma == 2) {
      mp3.play(T_ACIERTO_ES); delay(2000);
      mp3.play(T_ACIERTO_KQ); delay(2000);
      mp3.play(T_ACIERTO_EN);
    }
  }
}

// AUDIO: VICTORIA

void reproducirVictoria() {

  if (!dfOk || !audioHabilitado) return;

  if (idioma == 0) mp3.play(T_VICTORIA_ES);
  else if (idioma == 1) mp3.play(T_VICTORIA_KQ);
  else if (idioma == 3) mp3.play(T_VICTORIA_EN);
  else if (idioma == 2) {
    mp3.play(T_VICTORIA_ES); delay(2500);
    mp3.play(T_VICTORIA_KQ); delay(2500);
    mp3.play(T_VICTORIA_EN);
  }
}

void reproducirAplausos() {

  if (!dfOk || !audioHabilitado) return;

  delay(1500);
  mp3.play(T_APLAUSOS);
}