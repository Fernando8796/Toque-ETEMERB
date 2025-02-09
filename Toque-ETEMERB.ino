#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define led 16  // pino do led de conexão
#define rele 17 // pino do rele

// Configuração da rede Wi-Fi
const char* ssid = "teste";
const char* password = "123@qwe";

WiFiUDP ntpUDP; // Cria uma instância do cliente UDP
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800);

void ligarToque(int segundos) {
  digitalWrite(rele, HIGH);
  delay(segundos * 1000);
  digitalWrite(rele, LOW);
}

void conectarAoWiFi(){
  WiFi.begin(ssid, password);
  Serial.println("Conectando...");
  delay(5000);

  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(led, LOW);
    WiFi.disconnect();
    conectarAoWiFi();
  } else {
    digitalWrite(led, HIGH);
    Serial.printf("Conectado à %s\n", ssid);
    Serial.println("IP: " + WiFi.localIP().toString());
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  pinMode(rele, OUTPUT);

  // Conecta ao Wi-Fi
  conectarAoWiFi();
  
  // Inicializa o cliente NTP
  timeClient.begin();
}

void loop() {

  // Testa a conexão
  if (WiFi.status() != WL_CONNECTED) {
    conectarAoWiFi();
  }

  timeClient.update();

  // Pega o horário atual
  String horario = timeClient.getFormattedTime();
  Serial.println("Hora atual de Brasília: " + horario);

  // Calcula o dia da semana baseado no timestamp atual
  int diaSemana = timeClient.getDay();

  //Se for Domingo ou Sábado
  if(diaSemana == 0 || diaSemana == 6){
    delay(1000);
    return;
  }

  // HORÁRIO DOS TOQUES (INTEGRAL)
  if(horario == "07:30:00")
    ligarToque(10);
  else if(horario == "07:45:00")
    ligarToque(10);
  else if(horario == "08:20:00")
    ligarToque(10);
  else if(horario == "09:10:00")
    ligarToque(10);
  else if(horario == "09:30:00")
    ligarToque(15);
  else if(horario == "10:20:00")
    ligarToque(10);
  else if(horario == "11:10:00")
    ligarToque(10);
  else if(horario == "12:00:00")
    ligarToque(10);
  else if(horario == "13:20:00")
    ligarToque(15);
  else if(horario == "13:30:00")
    ligarToque(15);
  else if(horario == "14:10:00")
    ligarToque(10);
  else if(horario == "15:00:00")
    ligarToque(10);
  else if(horario == "15:20:00")
    ligarToque(15);
  else if(horario == "15:30:00")
    ligarToque(15);
  else if(horario == "16:10:00")
    ligarToque(10);
  else if(horario == "17:00:00")
    ligarToque(10);

  // HORÁRIO DOS TOQUES (SUBSEQUENTE)
  if(horario == "19:00:00")
    ligarToque(10);
  else if(horario == "19:40:00")
    ligarToque(10);
  else if(horario == "20:20:00")
    ligarToque(10);
  else if(horario == "21:00:00")
    ligarToque(10);
  else if(horario == "21:40:00")
    ligarToque(10);
  else if(horario == "22:00:00")
    ligarToque(10);

  delay(1000); // Atualiza a cada segundo
}