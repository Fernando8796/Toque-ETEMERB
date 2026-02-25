from flask import Blueprint, render_template, jsonify, request
from datetime import datetime
import core
import storage

api_bp = Blueprint('api', __name__)

@api_bp.route('/')
def index():
    return render_template('index.html')

@api_bp.route('/api/status')
def get_status():
    return jsonify({
        "sistema": core.estado_sistema,
        "hora_servidor": datetime.now().strftime("%H:%M:%S")
    })

@api_bp.route('/api/horarios', methods=['GET', 'POST', 'DELETE'])
def api_horarios():
    horarios = storage.carregar_horarios()
    
    if request.method == 'GET':
        return jsonify(horarios)
    
    elif request.method == 'POST':
        novo = request.json
        if not novo or not novo.get('time'): return jsonify({"erro": "Inválido"}), 400
        if any(h['time'] == novo['time'] for h in horarios): return jsonify({"erro": "Duplicado"}), 409
        
        horarios.append(novo)
        horarios.sort(key=lambda x: x['time'])
        storage.salvar_horarios(horarios)
        return jsonify({"msg": "Salvo"}), 201

    elif request.method == 'DELETE':
        alvo = request.json
        horarios = [h for h in horarios if h['time'] != alvo['time']]
        storage.salvar_horarios(horarios)
        return jsonify({"msg": "Removido"}), 200

@api_bp.route('/api/tocar', methods=['POST'])
def api_tocar_manual():
    core.disparar_campainha(duracao=10)
    return jsonify({"msg": "Comando enviado"})

@api_bp.route('/api/parar', methods=['POST'])
def api_parar_manual():
    core.parar_campainha()
    return jsonify({"msg": "Toque interrompido"})

@api_bp.route('/api/config', methods=['POST'])
def api_config():
    novo_status = request.json.get('status')
    if novo_status in ["Ativo", "Desativado", "ParadoHoje"]:
        core.estado_sistema['status'] = novo_status
        return jsonify(core.estado_sistema)
    return jsonify({"erro": "Status inválido"}), 400