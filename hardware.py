from datetime import datetime

try:
    from gpiozero import OutputDevice  # type: ignore
    GPIO_DISPONIVEL = True
except ImportError:
    GPIO_DISPONIVEL = False


class Relay:
    def __init__(self, pin: int, active_high: bool = True):
        self.pin = pin
        self.active_high = active_high
        self._device = None

        if GPIO_DISPONIVEL:
            try:
                self._device = OutputDevice(
                    pin,
                    active_high=active_high,
                    initial_value=False
                )
                print(f"GPIO {pin} configurado com sucesso.")
            except Exception as e:
                print(f"Erro ao iniciar GPIO: {e}")
        else:
            print("GPIO não disponível. Rodando em modo simulação.")

    def on(self):
        hora = datetime.now().strftime('%H:%M:%S')
        if self._device:
            self._device.on()
        print(f"[{hora}] ⚡ RELÉ LIGADO (GPIO {self.pin})")

    def off(self):
        hora = datetime.now().strftime('%H:%M:%S')
        if self._device:
            self._device.off()
        print(f"[{hora}] 💤 RELÉ DESLIGADO")

    def toggle(self):
        if self._device:
            self._device.toggle()