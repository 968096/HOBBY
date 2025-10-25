import requests
import serial
import time

# --- CONFIGURAÇÕES ---
REPO = "FluxGarage/RoboEyes"
# Encontre a porta serial correta no PlatformIO ou no Gerenciador de Dispositivos
# Pode ser "COM3" no Windows ou "/dev/ttyUSB0", "/dev/tty.usbmodemXXXX" no Mac/Linux
SERIAL_PORT = "/dev/ttyUSB0" # <-- CORRIGIDO!
BAUD_RATE = 9600
UPDATE_INTERVAL_SECONDS = 60 # Atualiza a cada 60 segundos

# Conecta à porta serial
try:
    arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2) # Espera a conexão serial estabilizar
    print(f"Conectado ao Arduino na porta {SERIAL_PORT}")
except serial.SerialException as e:
    print(f"Erro: Não foi possível abrir a porta serial {SERIAL_PORT}.")
    print(f"Verifique se a porta está correta e se não está sendo usada por outro programa (como o Monitor Serial do PlatformIO).")
    exit()

def get_repo_stats():
    """Busca as estatísticas do repositório na API do GitHub."""
    url = f"https://api.github.com/repos/{REPO}"
    try:
        response = requests.get(url)
        response.raise_for_status() # Lança um erro se a requisição falhar
        data = response.json()

        # Extrai os dados que queremos
        stars = data.get("stargazers_count", 0)
        forks = data.get("forks_count", 0)
        issues = data.get("open_issues_count", 0)

        return stars, forks, issues
    except requests.exceptions.RequestException as e:
        print(f"Erro ao buscar dados do GitHub: {e}")
        return None, None, None

print("Iniciando monitoramento do GitHub...")
while True:
    print("Buscando novas estatísticas...")
    stars, forks, issues = get_repo_stats()

    if stars is not None:
        # Formata os dados em uma string simples separada por vírgulas
        # O formato é: "S<estrelas>,F<forks>,I<issues>\n"
        # O "S", "F", "I" ajudam o Arduino a identificar os dados
        data_string = f"S{stars},F{forks},I{issues}\n"

        # Envia a string para o Arduino
        arduino.write(data_string.encode('utf-8'))
        print(f"Dados enviados: {data_string.strip()}")

    print(f"Aguardando {UPDATE_INTERVAL_SECONDS} segundos para a próxima atualização...")
    time.sleep(UPDATE_INTERVAL_SECONDS)
