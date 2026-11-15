import digitalio
import time


class StatusLED:
    def __init__(self, pin, active_low=True):
        self._pin = pin
        self._active_low = active_low

        self._io = digitalio.DigitalInOut(pin)
        self._io.direction = digitalio.Direction.OUTPUT
        self._io.value = int(active_low)

    def __str__(self):
        return f"Status LED Pin {self._pin}"

    def __repr__(self):
        return f"StatusLED({self._pin}, active_low={self_active_low})"

    def on(self):
        self._io.value = int(not self._active_low)

    def off(self):
        self._io.value = int(self._active_low)

    def toggle(self):
        self._io.value = 1 - self._io.value

    def blink(self, num, delay=0.5, initial_delay=0.5):
        self.off()
        time.sleep(initial_delay)
        for _ in range(num):
            self.on()
            time.sleep(delay)
            self.off()
            time.sleep(delay)
