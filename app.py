import os
import json
import time
import threading
from datetime import datetime
from flask import Flask, render_template, jsonify, request

# --- INTEGRAÇÃO GPIOZERO ---
try:
    from gpiozero import OutputDevice # type: ignore
    GPIO_DISPONIVEL = True
except ImportError:
    GPIO_DISPONIVEL = False
    print("Biblioteca gpiozero não encontrada. O sistema rodará em modo SIMULAÇÃO.")

# --- CONFIGURAÇÃO ---
app = Flask(__name__)
ARQUIVO_DB = 'horarios.json'
PIN_DO_RELE = 23  # Porta GPIO (BCM)

rele_fisico = None

if GPIO_DISPONIVEL:
    try:
        # active_high=True -> Envia 3.3v para ligar. Mude para False se seu relé for Low Trigger.
        rele_fisico = OutputDevice(PIN_DO_RELE, active_high=True, initial_value=False)
        print(f"--- GPIO {PIN_DO_RELE} Configurado com Sucesso ---")
    except Exception as e:
        print(f"Erro ao iniciar pinos GPIO: {e}")
        GPIO_DISPONIVEL = False

# Estado global do sistema
estado_sistema = {
    "status": "Ativo",      # Ativo, Desativado, ParadoHoje
    "tocando": False,       # True se o relé estiver LIGADO
    "ultima_acao": None
}

def acionar_rele_fisico(ligar):
    """Controla o pino físico do Raspberry Pi"""
    global rele_fisico
    try:
        hora_atual = datetime.now().strftime('%H:%M:%S')
        if ligar:
            if rele_fisico: rele_fisico.on()
            print(f"[{hora_atual}] ⚡ RELÉ LIGADO (GPIO {PIN_DO_RELE})")
        else:
            if rele_fisico: rele_fisico.off()
            print(f"[{hora_atual}] 💤 RELÉ DESLIGADO")
    except Exception as e:
        print(f"Erro crítico ao acionar hardware: {e}")

# --- PERSISTÊNCIA DE DADOS ---
def carregar_horarios():
    if not os.path.exists(ARQUIVO_DB): return []
    try:
        with open(ARQUIVO_DB, 'r', encoding='utf-8') as file:
            return json.load(file)
    except: return []

def salvar_horarios(lista):
    try:
        with open(ARQUIVO_DB, 'w', encoding='utf-8') as f:
            json.dump(lista, f, indent=4)
    except Exception as e:
        print(f"Erro ao salvar DB: {e}")

# --- LÓGICA DO TOQUE (ATUALIZADA) ---
def thread_toque(duracao):
    """Executa o toque com capacidade de interrupção"""
    with app.app_context():
        if estado_sistema['tocando']:
            return 

        estado_sistema['tocando'] = True
        estado_sistema['ultima_acao'] = f"Tocou às {datetime.now().strftime('%H:%M:%S')}"
        
        acionar_rele_fisico(True)
        
        # Loop fracionado: dorme 0.1s várias vezes para verificar cancelamento
        passos = int(duracao * 10) 
        for _ in range(passos):
            if not estado_sistema['tocando']: 
                break # Sai do loop se o status mudar para False
            time.sleep(0.1)
            
        acionar_rele_fisico(False)
        estado_sistema['tocando'] = False

def disparar_campainha(duracao=10):
    t = threading.Thread(target=thread_toque, args=(duracao,))
    t.daemon = True
    t.start()

# --- ROTAS API ---
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/status')
def get_status():
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
        if not novo or not novo.get('time'): return jsonify({"erro": "Inválido"}), 400
        if any(h['time'] == novo['time'] for h in horarios): return jsonify({"erro": "Duplicado"}), 409
        horarios.append(novo)
        horarios.sort(key=lambda x: x['time'])
        salvar_horarios(horarios)
        return jsonify({"msg": "Salvo"}), 201
    elif request.method == 'DELETE':
        alvo = request.json
        horarios = [h for h in horarios if h['time'] != alvo['time']]
        salvar_horarios(horarios)
        return jsonify({"msg": "Removido"}), 200

@app.route('/api/tocar', methods=['POST'])
def api_tocar_manual():
    disparar_campainha(duracao=10)
    return jsonify({"msg": "Comando enviado"})

# NOVA ROTA PARA PARAR O TOQUE
@app.route('/api/parar', methods=['POST'])
def api_parar_manual():
    estado_sistema['tocando'] = False # Quebra o loop da thread
    acionar_rele_fisico(False) # Garante desligamento imediato
    print(f"[{datetime.now().strftime('%H:%M:%S')}] 🛑 TOQUE INTERROMPIDO MANUALMENTE")
    return jsonify({"msg": "Toque interrompido"})

@app.route('/api/config', methods=['POST'])
def api_config():
    novo_status = request.json.get('status')
    if novo_status in ["Ativo", "Desativado", "ParadoHoje"]:
        estado_sistema['status'] = novo_status
        return jsonify(estado_sistema)
    return jsonify({"erro": "Status inválido"}), 400

# --- ROBÔ MONITOR ---
def servico_monitoramento():
    print("--- Monitor Iniciado ---")
    ultimo_minuto_verificado = None

    while True:
        try:
            agora = datetime.now()
            hora_hhmm = agora.strftime("%H:%M")
            segundos = agora.second

            if hora_hhmm == "00:00" and segundos < 5 and estado_sistema['status'] == 'ParadoHoje':
                estado_sistema['status'] = 'Ativo'

            if estado_sistema['status'] == 'Ativo':
                if segundos == 0 and hora_hhmm != ultimo_minuto_verificado:
                    horarios = carregar_horarios()
                    for h in horarios:
                        if h['time'] == hora_hhmm:
                            print(f"⏰ DISPARO AUTOMÁTICO: {hora_hhmm}")
                            disparar_campainha(duracao=10)
                            ultimo_minuto_verificado = hora_hhmm
                            break
            
            if segundos > 5: ultimo_minuto_verificado = None
            time.sleep(0.5)
        except Exception as e:
            print(f"Erro monitor: {e}")
            time.sleep(5)

if __name__ == '__main__':
    if not os.environ.get("WERKZEUG_RUN_MAIN"):
        monitor = threading.Thread(target=servico_monitoramento)
        monitor.daemon = True
        monitor.start()
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)
