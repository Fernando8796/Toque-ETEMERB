import os
import json

ARQUIVO_DB = 'horarios.json'

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