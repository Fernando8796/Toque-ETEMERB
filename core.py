import time
import threading
from datetime import datetime
from storage import carregar_horarios, salvar_horarios
from hardware import Relay

# Objeto global de hardware
relay = Relay(pin=19)

# Estado global do sistema
estado_sistema = {
    "status": "Ativo",  # Ativo, Desativado, ParadoHoje
    "tocando": False,
    "ultima_acao": "Sistema iniciado"
}

def thread_toque(duracao):
    if estado_sistema['tocando']:
        return 

    estado_sistema['tocando'] = True
    estado_sistema['ultima_acao'] = f"Tocou às {datetime.now().strftime('%H:%M:%S')}"
    
    relay.on()
    
    # Loop fracionado para permitir interrupção imediata
    passos = int(duracao * 10) 
    for _ in range(passos):
        if not estado_sistema['tocando']: 
            break
        time.sleep(0.1)
        
    relay.off()
    estado_sistema['tocando'] = False

def disparar_campainha(duracao=10):
    t = threading.Thread(target=thread_toque, args=(duracao,))
    t.daemon = True
    t.start()

def parar_campainha():
    estado_sistema['tocando'] = False
    relay.off()

def servico_monitoramento():
    print("--- Monitor de Horários Iniciado ---")
    ultimo_minuto_verificado = None

    while True:
        try:
            agora = datetime.now()
            hora_hhmm = agora.strftime("%H:%M")
            segundos = agora.second

            # Reset diário do status "ParadoHoje"
            if hora_hhmm == "00:00" and segundos < 5 and estado_sistema['status'] == 'ParadoHoje':
                estado_sistema['status'] = 'Ativo'

            if estado_sistema['status'] == 'Ativo':
                if segundos == 0 and hora_hhmm != ultimo_minuto_verificado:
                    horarios = carregar_horarios()
                    if any(h['time'] == hora_hhmm for h in horarios):
                        print(f"⏰ DISPARO AUTOMÁTICO: {hora_hhmm}")
                        disparar_campainha(duracao=10)
                        ultimo_minuto_verificado = hora_hhmm
            
            if segundos > 5: 
                ultimo_minuto_verificado = None
                
            time.sleep(0.8)
        except Exception as e:
            print(f"Erro no monitor: {e}")
            time.sleep(5)