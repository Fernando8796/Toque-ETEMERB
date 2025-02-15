#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h> // Lib que deixa funções rodando em segundo plano

class ContadorInterno {
private:
  volatile uint32_t CONTADOR; // Variável que vai ter o tempo do dia em segundos
  volatile int diaSemana;  // Variável para armazenar o dia da semana

public:

  ContadorInterno(){
    CONTADOR = 0;
    diaSemana = 0;
  }

  void incrementarContador(){
    CONTADOR++;

    // Verifica se completou um dia (86400 segundos)
    if (CONTADOR >= 86400) {
      CONTADOR = 0; // Reseta o contador para o início do próximo dia
      diaSemana = (diaSemana + 1) % 7; // Incrementa o dia da semana (0 = Domingo, 6 = Sábado)
    }
  }

  void ajustarContador(int diaDaSemana, int horas[3]) {
    this->diaSemana = diaDaSemana;
    CONTADOR = (horas[0] * 3600) + (horas[1] * 60) + horas[2];
  }

  int pegarHoras(){
    return (CONTADOR / 3600) % 24;
  }
  int pegarMinutos(){
    return (CONTADOR / 60) % 60;
  }
  int pegarSegundos(){
    return CONTADOR % 60;
  }

  String pegarHoraFormatada(){
    int horas = pegarHoras();
    int minutos = pegarMinutos();
    int segundos = pegarSegundos();

    return (horas < 10 ? "0" : "") + String(horas) + ":" +
           (minutos < 10 ? "0" : "") + String(minutos) + ":" +
           (segundos < 10 ? "0" : "") + String(segundos);
  }

  uint32_t pegarContador(){
    return CONTADOR;
  }

  int pegarDiaSemana() {
    return diaSemana;
  }
};

WiFiUDP ntpUDP; // Cria uma instância do cliente UDP
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800);

Ticker timer; // Criação de um ticker
ContadorInterno relogio; // Criação de um relógio interno

#define led 16  // Pino do led de conexão
#define rele 17 // Pino do rele

// Configuração da rede Wi-Fi
const char* ssid = "teste";
const char* password = "123@qwe";

void incrementarRelogio() {
    relogio.incrementarContador();
}

void ligarToque(int segundos) {
  digitalWrite(rele, HIGH);
  delay(segundos * 1000);
  digitalWrite(rele, LOW);
}

int conectarAoWiFi(){

  int tentativas = 1;

  WiFi.begin(ssid, password);
  Serial.println("Conectando...");
  delay(5000);

  while (tentativas <= 5 && WiFi.status() != WL_CONNECTED){
    digitalWrite(led, LOW);
    WiFi.disconnect();

    WiFi.begin(ssid, password);
    Serial.println("Conectando...");
    delay(5000);
    tentativas++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(led, HIGH);
    Serial.printf("Conectado à %s\n", ssid);
    Serial.println("IP: " + WiFi.localIP().toString());
    return 1;
  } else {
    return 0;
  }
}

// Função Principal
void setup() {
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  pinMode(rele, OUTPUT);

  int conexao;
  do {
    conexao = conectarAoWiFi(); // Conecta ao Wi-Fi 
  } while (conexao == 0);
  
  timeClient.begin(); // Inicializa o cliente NTP
  timeClient.update(); // Atualiza o Cliente NTP

  int horas[] = {timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()};

  relogio.ajustarContador(timeClient.getDay(), horas);

  //Desconecta o WiFi para economizar recursos
  WiFi.disconnect();

  // Chama a função  que incrementará 1 segundo na contagem assíncronamente
  timer.attach(1.0, incrementarRelogio);
}

void loop() {

  String horario = relogio.pegarHoraFormatada();
  String DIA = String(relogio.pegarDiaSemana());

  Serial.println("Dia da Semana: " + DIA);
  Serial.println("Horário: " + horario);

  // Todo dia às seis horas da manhã fazer uma sincronização com o horário
  if(horario == "06:00:00"){
    if(conectarAoWiFi()){
      timeClient.begin(); // Inicializa o cliente NTP
      timeClient.update(); // Atualiza o Cliente NTP

      int horas[] = {timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()};

      relogio.ajustarContador(timeClient.getDay(), horas);

      //Desconecta o WiFi para economizar recursos
      WiFi.disconnect();
    }
  }

  // Pega o dia da semana
  int diaDaSemana = relogio.pegarDiaSemana();

  //Se for Domingo ou Sábado
  if(diaDaSemana == 0 || diaDaSemana == 6){
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
    ligarToque(10);
  else if(horario == "10:20:00")
    ligarToque(10);
  else if(horario == "11:10:00")
    ligarToque(10);
  else if(horario == "12:00:00")
    ligarToque(10);
  else if(horario == "13:20:00")
    ligarToque(10);
  else if(horario == "13:30:00")
    ligarToque(10);
  else if(horario == "14:10:00")
    ligarToque(10);
  else if(horario == "15:00:00")
    ligarToque(10);
  else if(horario == "15:20:00")
    ligarToque(10);
  else if(horario == "15:30:00")
    ligarToque(10);
  else if(horario == "16:10:00")
    ligarToque(10);
  else if(horario == "17:00:00")
    ligarToque(10);

  // HORÁRIO DOS TOQUES (SUBSEQUENTE)
  /*
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
  */

  delay(1000); // Atualiza a cada segundo
}