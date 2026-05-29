import time
import serial
import serial.tools.list_ports
import os
import shutil
import string

def get_drives():
    drives = []
    # Check drive letters from A to Z on Windows
    for letter in string.ascii_uppercase:
        drive = f"{letter}:\\"
        if os.path.exists(drive):
            drives.append(drive)
    return drives

def find_pico_drive():
    for drive in get_drives():
        info_path = os.path.join(drive, "INFO_UF2.TXT")
        if os.path.exists(info_path):
            return drive
    return None

def main():
    print("Searching for Pico/RP2040 COM ports...")
    ports = serial.tools.list_ports.comports()
    pico_port = None
    
    # Check for COM20 specifically or any port with PICO/2E8A
    for p in ports:
        if p.device == "COM20":
            pico_port = p.device
            break
    
    if not pico_port:
        for p in ports:
            if "2E8A" in str(p.hwid).upper() or "PICO" in p.description.upper() or "RP2040" in p.description.upper():
                pico_port = p.device
                break
                
    if not pico_port and len(ports) > 0:
        # Fallback to the first available COM port
        pico_port = ports[0].device

    if pico_port:
        print(f"Found target COM port: {pico_port}")
        print("Sending 1200-baud reset trigger to Pico...")
        try:
            # 1200 baud software reset trigger
            ser = serial.Serial(pico_port, 1200, timeout=1)
            ser.close()
            print("Reset signal transmitted. Waiting for Pico to remount as mass storage drive...")
        except Exception as e:
            print(f"Could not open/close port at 1200 baud: {e}")
            print("Assuming Pico is already in BOOTSEL mode or manually reset.")
    else:
        print("No serial COM ports detected. Scanning for mounted Pico drives directly...")

    # Wait up to 10 seconds for the drive to appear
    pico_drive = None
    for attempt in range(10):
        pico_drive = find_pico_drive()
        if pico_drive:
            print(f"Detected Pico drive mounted at: {pico_drive}")
            break
        time.sleep(1.0)
        print(f"Waiting for drive... {attempt + 1}/10")

    if not pico_drive:
        print("\n[ERROR] Pico mass storage drive was not detected automatically.")
        print("Please ensure your Pico is in BOOTSEL mode (hold BOOTSEL button while plugging in).")
        print("Available drives: ", get_drives())
        return False

    uf2_source = r"e:\rouf\hardware-project\stat-screen\pico_firmware\build\arduino.mbed_rp2040.pico\pico_firmware.ino.uf2"
    if not os.path.exists(uf2_source):
        print(f"[ERROR] Source UF2 file not found: {uf2_source}")
        return False

    destination = os.path.join(pico_drive, "pico_firmware.uf2")
    print(f"Copying compiled binary to Pico: {uf2_source} -> {destination}")
    try:
        shutil.copyfile(uf2_source, destination)
        print("Flash completed successfully! The Pico will now reboot into live diagnostics mode.")
        return True
    except Exception as e:
        print(f"[ERROR] Copy failed: {e}")
        return False

if __name__ == "__main__":
    main()
