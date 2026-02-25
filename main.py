import os
import threading
from flask import Flask
from routes import api_bp
import core

app = Flask(__name__)
app.register_blueprint(api_bp)

if __name__ == '__main__':
    # Inicia o robô monitor em uma thread separada
    # O check de WERKZEUG evita que o Flask dispare a thread duas vezes no modo debug
    if not os.environ.get("WERKZEUG_RUN_MAIN"):
        monitor = threading.Thread(target=core.servico_monitoramento)
        monitor.daemon = True
        monitor.start()

    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)