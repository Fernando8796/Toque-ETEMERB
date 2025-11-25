import os
import json
import time
import threading
from datetime import datetime
from flask import Flask, render_template, jsonify, request

# --- INTEGRAÇÃO GPIOZERO ---
# Tenta importar a biblioteca. Se falhar (ex: rodando no Windows), usa um Mock para não travar.
try:
    from gpiozero import OutputDevice # type: ignore
    GPIO_DISPONIVEL = True
except ImportError:
    GPIO_DISPONIVEL = False
    print("Biblioteca gpiozero não encontrada. O sistema rodará em modo SIMULAÇÃO.")

# --- CONFIGURAÇÃO ---
app = Flask(__name__)
ARQUIVO_DB = 'horarios.json'
PIN_DO_RELE = 23  # Porta GPIO (BCM) onde o relé está conectado

# Inicialização do Hardware (GPIO)
rele_fisico = None

if GPIO_DISPONIVEL:
    try:
        # active_high=True -> Envia 3.3v para ligar (Padrão)
        # active_high=False -> Envia 0v (GND) para ligar (Comum em módulos de relé azuis "Low Trigger")
        # DICA: Se o relé ligar sozinho ao iniciar o programa, mude active_high para False aqui:
        rele_fisico = OutputDevice(PIN_DO_RELE, active_high=True, initial_value=False)
        print(f"--- GPIO {PIN_DO_RELE} Configurado com Sucesso ---")
    except Exception as e:
        print(f"Erro ao iniciar pinos GPIO: {e}")
        GPIO_DISPONIVEL = False

# Estado global do sistema (Memória RAM)
estado_sistema = {
    "status": "Ativo",      # Ativo, Desativado, ParadoHoje
    "tocando": False,       # True se o relé estiver LIGADO neste exato momento
    "ultima_acao": None     # Log simples da última ação
}

def acionar_rele_fisico(ligar):
    """Controla o pino físico do Raspberry Pi usando gpiozero"""
    global rele_fisico
    
    try:
        hora_atual = datetime.now().strftime('%H:%M:%S')
        
        if ligar:
            if rele_fisico: 
                rele_fisico.on() # Liga o GPIO
            print(f"[{hora_atual}] ⚡ RELÉ LIGADO (GPIO {PIN_DO_RELE})")
        else:
            if rele_fisico: 
                rele_fisico.off() # Desliga o GPIO
            print(f"[{hora_atual}] 💤 RELÉ DESLIGADO")
            
    except Exception as e:
        print(f"Erro crítico ao acionar hardware: {e}")

# --- PERSISTÊNCIA DE DADOS ---
def carregar_horarios():
    if not os.path.exists(ARQUIVO_DB):
        return []
    try:
        with open(ARQUIVO_DB, 'r', encoding='utf-8') as file:
            return json.load(file)
    except:
        return []

def salvar_horarios(lista):
    try:
        with open(ARQUIVO_DB, 'w', encoding='utf-8') as f:
            json.dump(lista, f, indent=4)
    except Exception as e:
        print(f"Erro ao salvar DB: {e}")

# --- LÓGICA DO TOQUE (CORE) ---
def thread_toque(duracao):
    """Executa o toque em background para não travar o servidor"""
    with app.app_context(): # Garante contexto se precisar logar algo complexo
        if estado_sistema['tocando']:
            return # Evita sobreposição (tocar se já estiver tocando)

        estado_sistema['tocando'] = True
        estado_sistema['ultima_acao'] = f"Tocou às {datetime.now().strftime('%H:%M:%S')}"
        
        acionar_rele_fisico(True)
        time.sleep(duracao)
        acionar_rele_fisico(False)
        
        estado_sistema['tocando'] = False

def disparar_campainha(duracao=10):
    """Inicia a thread do toque"""
    t = threading.Thread(target=thread_toque, args=(duracao,))
    t.daemon = True # Thread morre se o app fechar
    t.start()

# --- ROTAS API ---

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/status')
def get_status():
    # O frontend chama isso a cada 2 segundos para saber TUDO sobre o sistema
    return jsonify({
        "sistema": estado_sistema,
        "hora_servidor": datetime.now().strftime("%H:%M:%S"),
        "gpio_ativo": GPIO_DISPONIVEL
    })

@app.route('/api/horarios', methods=['GET', 'POST', 'DELETE'])
def api_horarios():
    horarios = carregar_horarios()

    if request.method == 'GET':
        return jsonify(horarios)

    elif request.method == 'POST':
        novo = request.json
        if not novo or not novo.get('time'):
            return jsonify({"erro": "Dados inválidos"}), 400
        
        # Evita duplicatas exatas
        if any(h['time'] == novo['time'] for h in horarios):
             return jsonify({"erro": "Horário já cadastrado"}), 409

        horarios.append(novo)
        horarios.sort(key=lambda x: x['time'])
        salvar_horarios(horarios)
        return jsonify({"msg": "Salvo com sucesso"}), 201

    elif request.method == 'DELETE':
        alvo = request.json
        horarios = [h for h in horarios if h['time'] != alvo['time']]
        salvar_horarios(horarios)
        return jsonify({"msg": "Removido"}), 200

@app.route('/api/tocar', methods=['POST'])
def api_tocar_manual():
    # Toque manual (acionado pelo botão)
    disparar_campainha(duracao=10) # 10 SEGUNDOS CONFORME PEDIDO
    return jsonify({"msg": "Comando enviado"})

@app.route('/api/config', methods=['POST'])
def api_config():
    # Altera status (Ativar/Desativar/PararHoje)
    novo_status = request.json.get('status')
    if novo_status in ["Ativo", "Desativado", "ParadoHoje"]:
        estado_sistema['status'] = novo_status
        print(f"Configuração alterada para: {novo_status}")
        return jsonify(estado_sistema)
    return jsonify({"erro": "Status inválido"}), 400

# --- ROBÔ MONITOR (Background) ---
def servico_monitoramento():
    print("--- Monitor de Horários Iniciado ---")
    ultimo_minuto_verificado = None

    while True:
        try:
            agora = datetime.now()
            hora_hhmm = agora.strftime("%H:%M")
            segundos = agora.second

            # 1. Reset diário do "ParadoHoje" (Meia-noite)
            if hora_hhmm == "00:00" and segundos < 5 and estado_sistema['status'] == 'ParadoHoje':
                estado_sistema['status'] = 'Ativo'
                print("Sistema reativado automaticamente (novo dia).")

            # 2. Verificação de Horários
            # Só verifica se estiver ATIVO e se ainda não verificou neste minuto
            if estado_sistema['status'] == 'Ativo':
                # Verifica apenas no segundo 00 para precisão
                if segundos == 0 and hora_hhmm != ultimo_minuto_verificado:
                    horarios = carregar_horarios()
                    for h in horarios:
                        if h['time'] == hora_hhmm:
                            print(f"⏰ DISPARO AUTOMÁTICO: {hora_hhmm} - {h.get('label')}")
                            disparar_campainha(duracao=10) # 10 SEGUNDOS
                            ultimo_minuto_verificado = hora_hhmm
                            break # Evita disparar 2x se houver duplicidade errada no JSON
            
            # Reset do controle de minuto (para garantir que funcione na proxima hora)
            if segundos > 5:
                ultimo_minuto_verificado = None

            time.sleep(0.5) # Alta precisão
            
        except Exception as e:
            print(f"Erro no loop do monitor: {e}")
            time.sleep(5) # Espera mais se der erro

if __name__ == '__main__':
    # Inicia Monitor em Thread Separada
    if not os.environ.get("WERKZEUG_RUN_MAIN"):
        monitor = threading.Thread(target=servico_monitoramento)
        monitor.daemon = True
        monitor.start()

    # Inicia Servidor Flask
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)