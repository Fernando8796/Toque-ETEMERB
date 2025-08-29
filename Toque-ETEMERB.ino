#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "time.h"

// Configuração do WiFi
const char* ssid = "RedeTeste";
const char* password = "1234@qwer";

// Configuração do servidor NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset = -3 * 3600;  // Fuso horário (Brasil: GMT-3)

// Lista de horários dos toques
const char* horariosToque[] = {
  "07:30:00", "07:45:00", "08:20:00", "09:10:00",
  "09:30:00", "10:20:00", "11:10:00", "12:00:00",
  "13:20:00", "13:30:00", "14:10:00", "15:00:00",
  "15:20:00", "15:30:00", "16:10:00", "17:00:00"
};
const int numToques = sizeof(horariosToque) / sizeof(horariosToque[0]);

AsyncWebServer server(80);
String statusToque = "Ativo";
unsigned char paradoHoje = 0;
unsigned char desativado = 0;

// Variáveis para controlar o toque sem usar delay()
unsigned long tempoInicioToque = 0;
const int duracaoToque = 10; // Duração do toque em segundos

// Variável que controla o relé
const int pinoRele = 0;

// ================= Funções de toque =================
void iniciarToque() {
  if (statusToque != "Tocando") {
    Serial.println("Iniciando toque...");
    statusToque = "Tocando";
    digitalWrite(pinoRele, HIGH);
    tempoInicioToque = millis();
  }
}

// ================== Setup ==================
void setup() {
  Serial.begin(115200);
  pinMode(pinoRele, OUTPUT);

  // Conectar WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConectado ao WiFi!");
  Serial.println(WiFi.localIP());

  // Configurar NTP
  configTime(gmtOffset, 0, ntpServer);

  // Index/Página principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){

    if (request->hasParam("hora") && request->hasParam("minuto")) {
      // Obter a data atual do ESP32 para manter a data correta
      struct tm timeinfo_now;
      getLocalTime(&timeinfo_now);
      
      // Extrair os valores de hora e minuto dos argumentos da URL
      int hora = 0;
      int minuto = 0;
      hora = request->getParam("hora")->value().toInt();
      minuto = request->getParam("minuto")->value().toInt();
      
      // Log para verificar os valores
      Serial.print("Valores recebidos via GET: ");
      Serial.print("Hora: ");
      Serial.print(hora);
      Serial.print(", Minuto: ");
      Serial.println(minuto);

      // Configurar a nova estrutura de tempo
      struct tm timeinfo;
      memset(&timeinfo, 0, sizeof(timeinfo));
      timeinfo.tm_hour = hora;
      timeinfo.tm_min  = minuto;
      timeinfo.tm_sec  = 0;
      timeinfo.tm_mday = timeinfo_now.tm_mday;
      timeinfo.tm_mon  = timeinfo_now.tm_mon; 
      timeinfo.tm_year = timeinfo_now.tm_year;

      // Atualizar a hora do sistema
      time_t t = mktime(&timeinfo);
      struct timeval tv = { .tv_sec = t };
      settimeofday(&tv, NULL);
    }

    const char html[] PROGMEM = R"rawliteral(
    <!doctypehtml><html lang=pt-br><meta charset=UTF-8><meta name=viewport content="width=device-width,initial-scale=1"><title>Toque Automático ETEMERB</title><style>body{font-family:"Segoe UI",Arial,sans-serif;text-align:center;background:#f0f2f5;margin:0;padding:0;color:#333}header{background:linear-gradient(135deg,#0056b3,#007bff);color:#fff;padding:18px 0;font-size:1.5em;font-weight:700;box-shadow:0 2px 6px rgba(0,0,0,.2)}#clock{font-size:2.5em;margin:15px 0 5px 0;color:#222}.badge{display:inline-block;padding:6px 12px;font-size:1em;font-weight:700;color:#fff;background-color:#17a2b8;border-radius:20px;box-shadow:0 2px 4px rgba(0,0,0,.15)}.btn{display:block;margin:12px auto;padding:12px 18px;font-size:1em;border:none;border-radius:8px;cursor:pointer;width:240px;transition:transform .15s ease,background .3s ease;box-shadow:0 3px 6px rgba(0,0,0,.15)}.btn:hover{transform:scale(1.05)}.btn-primary{background:#007bff;color:#fff}.btn-primary:hover{background:#0056b3}.btn-danger{background:#dc3545;color:#fff}.btn-danger:hover{background:#a71d2a}.btn-secondary{background:#6c757d;color:#fff}.btn-secondary:hover{background:#565e64}.status-box{display:inline-block;background:#fff;padding:18px;margin:12px;border-radius:12px;box-shadow:0 3px 8px rgba(0,0,0,.12);width:220px}.status-title{font-weight:700;margin-bottom:12px;color:#444}.status{font-size:1.2em;padding:8px;border-radius:6px;color:#fff;font-weight:700}.active{background:#28a745}.stopped{background:#6c757d}.tocando{background:#ffc107;color:#212529}.popup{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,.5);justify-content:center;align-items:center;z-index:1000}.popup-content{background:#fff;padding:20px;border-radius:10px;width:320px;text-align:center;box-shadow:0 4px 10px rgba(0,0,0,.25);position:relative}.popup-content h2{margin-top:0}.popup-content label{display:block;margin-top:10px;text-align:left;font-size:.9em;color:#333}.popup-content input,.popup-content select{width:100%;padding:8px;margin-top:5px;border-radius:6px;border:1px solid #ccc}.popup-content button{margin-top:15px;padding:10px;width:100%;border:none;border-radius:6px;background:#007bff;color:#fff;cursor:pointer}.popup-content button:hover{background:#0056b3}.close{position:absolute;top:10px;right:15px;font-size:22px;cursor:pointer;color:#666}</style><header>Toque automático ETEMERB</header><div id=clock>00:00:00</div><div id=weekday class=badge>Domingo</div><div><div class=status-box><div class=status-title>Status</div><div id=toque-status class="status stopped">Desativado</div></div></div><button class="btn btn-primary"onclick=tocarAgora()>⏰ Tocar agora</button><button id=btn-ajustar class="btn btn-secondary">🕑 Ajustar Horário</button><button class="btn btn-danger"onclick=pararHoje()>🛑 Parar apenas hoje</button><button class="btn btn-secondary"onclick=desativar()>🔕 Desativar</button><button class="btn btn-primary"onclick=reativar()>✅ Reativar sistema</button><footer>© 2025 José Fernando - Espaço CRIA</footer><div id=popup class=popup><div class=popup-content><span id=close class=close>×</span><h2>Ajustar Horário</h2><form action=/ ><label for=hora>Hora:</label><input type=number id=hora name=hora min=0 max=23 required><label for=minuto>Minuto:</label><input type=number id=minuto name=minuto min=0 max=59 required><button type=submit>Salvar</button></form></div></div><script>async function atualizarEstado(){try{const t=await fetch("/pegarEstado"),e=await t.json();let a=e.status,n=document.getElementById("toque-status");"Ativo"==a?(n.textContent="Ativo",n.className="status active"):"Tocando"==a?(n.textContent="Tocando",n.className="status tocando"):"ParadoHoje"==a?(n.textContent="Parado (hoje)",n.className="status stopped"):"Desativado"==a&&(n.textContent="Desativado",n.className="status stopped"),document.getElementById("clock").textContent=e.horario;const o=["Domingo","Segunda","Terça","Quarta","Quinta","Sexta","Sábado"],s=parseInt(e.diaSemana);document.getElementById("weekday").textContent=o[s]}catch(t){document.getElementById("clock").textContent="EE:EE:EE",document.getElementById("weekday").textContent="-"}}function tocarAgora(){fetch("/tocar").then((()=>{let t=document.getElementById("toque-status"),e=t.textContent,a=t.className;t.textContent="Tocando...",t.className="status tocando",setTimeout((()=>{t.textContent=e,t.className=a}),1e4)}))}function pararHoje(){fetch("/pararHoje").then((()=>{let t=document.getElementById("toque-status");t.textContent="Parado (hoje)",t.className="status stopped"}))}function desativar(){fetch("/desativar").then((()=>{let t=document.getElementById("toque-status");t.textContent="Desativado",t.className="status stopped"}))}function reativar(){fetch("/reativar").then((()=>{let t=document.getElementById("toque-status");t.textContent="Ativo",t.className="status active"}))}setInterval(atualizarEstado,500);let popup=document.getElementById("popup"),btn=document.getElementById("btn-ajustar"),closeBtn=document.getElementById("close");btn.onclick=function(){let t=new Date;document.getElementById("hora").value=t.getHours(),document.getElementById("minuto").value=t.getMinutes(),popup.style.display="flex"},closeBtn.onclick=function(){popup.style.display="none"},window.onclick=function(t){t.target==popup&&(popup.style.display="none")}</script>
    )rawliteral";

    request->send(200, "text/html", html);
  });

  // API para pegar o estado do relógio
  server.on("/pegarEstado", HTTP_GET, [](AsyncWebServerRequest *request){
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      request->send(500, "text/plain", "Falha ao obter hora");
      return;
    }

    // Formatar horário (HH:MM:SS)
    char horario[9];
    strftime(horario, sizeof(horario), "%H:%M:%S", &timeinfo);

    // Dia da semana (0 = domingo, 1 = segunda, ...)
    int diaSemana = timeinfo.tm_wday;

    // Ajusta o status do toque
    String status = statusToque;
    if (paradoHoje) status = "ParadoHoje";
    else if (desativado) status = "Desativado";

    // Montar JSON {status, horario, diaDaSemana}
    char buffer[100];
    sprintf(buffer, "{ \"status\":\"%s\", \"horario\":\"%s\", \"diaSemana\":\"%d\" }",status.c_str(),  horario, diaSemana);

    // Enviar resposta JSON
    request->send(200, "application/json", buffer);
  });
  
  // API para tocar
  server.on("/tocar", HTTP_GET, [](AsyncWebServerRequest *request){  
    iniciarToque();
    request->send(200);
  });

  // API para parar de tocar por hoje
  server.on("/pararHoje", HTTP_GET, [](AsyncWebServerRequest *request){
    paradoHoje = 1;
    statusToque = "ParadoHoje";
    request->send(200);
  });
  
  // API para desativar o toque
  server.on("/desativar", HTTP_GET, [](AsyncWebServerRequest *request){
    desativado = 1;
    statusToque = "Desativado";
    request->send(200);
  });

  // API para reativar
  server.on("/reativar", HTTP_GET, [](AsyncWebServerRequest *request){
    desativado = 0;
    paradoHoje = 0;
    statusToque = "Ativo";
    request->send(200);
  });

  server.begin();
}

// ================== Loop ==================
void loop() {
  // Use uma variável estática para rastrear o dia atual e resetar a flag
  static int diaAnterior = -1;
  static unsigned long ultimoSegundo = 0;

  // Verificação de tempo
  if (millis() - ultimoSegundo >= 1000) {
    ultimoSegundo = millis();
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Falha ao obter hora");
      return;
    }

    // Lógica para resetar a flag 'paradoHoje' no início de um novo dia
    if (diaAnterior != timeinfo.tm_mday && diaAnterior != -1) {
      Serial.println("Novo dia detectado. Resetando 'paradoHoje'...");
      paradoHoje = 0;
      if (!desativado) {
        statusToque = "Ativo";
      }
    }
    diaAnterior = timeinfo.tm_mday;
    
    char horario[9];
    sprintf(horario, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    Serial.println(horario);
    
    // Verifica os horários de toque apenas se o sistema estiver ativo e não for fim de semana
    if (!paradoHoje && !desativado && timeinfo.tm_wday != 0 && timeinfo.tm_wday != 6) {
      verificarToque(String(horario));
    }
  }

  // Se o toque estiver ativo, verifica se a duração já passou.
  if (statusToque == "Tocando" && (millis() - tempoInicioToque) >= (duracaoToque * 1000)) {
    Serial.println("Toque finalizado.");
    digitalWrite(pinoRele, LOW);
    statusToque = "Ativo";
  }
}

// Função para verificar se a hora atual corresponde a um toque agendado
void verificarToque(String horarioAtual) {
  for (int i = 0; i < numToques; i++) {
    if (horarioAtual == horariosToque[i]) {
      iniciarToque();
      break;
    }
  }
}
