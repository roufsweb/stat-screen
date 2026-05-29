#ifndef TELEMETRY_DATA_H
#define TELEMETRY_DATA_H

#include <Arduino.h>

#pragma pack(push, 1)
struct TelemetryPacket {
    uint8_t magic1;         // 0xAA
    uint8_t magic2;         // 0x55
    uint8_t packet_type;    // 0x01 = Telemetry, 0x02 = Config
    uint8_t cpu_load;       // 0 - 100 %
    uint8_t cpu_temp;       // 0 - 120 °C
    uint16_t cpu_freq;      // CPU frequency in MHz
    uint8_t gpu_load;       // 0 - 100 %
    uint8_t gpu_temp;       // 0 - 120 °C
    uint8_t gpu_vram;       // 0 - 100 %
    uint8_t ram_percent;    // 0 - 100 %
    uint16_t ram_used_mb;   // Used RAM in MB
    uint16_t ram_total_mb;  // Total RAM in MB
    uint8_t disk_percent;   // 0 - 100 %
    float net_dl_rate;      // MB/s
    float net_ul_rate;      // MB/s
    uint8_t active_cards;   // Bit 0: CPU, Bit 1: GPU, Bit 2: SYSTEM
    uint8_t cycle_sec;      // Screen auto-rotation interval in seconds
    uint32_t uptime_sec;    // Host PC uptime in seconds
    uint8_t reserved;       // For alignment / future expansion
    uint8_t checksum;       // XOR checksum of bytes 0 to 30
};
#pragma pack(pop)

struct TelemetryState {
    uint8_t cpu_load = 0;
    uint8_t cpu_temp = 0;
    uint16_t cpu_freq = 0;
    uint8_t gpu_load = 0;
    uint8_t gpu_temp = 0;
    uint8_t gpu_vram = 0;
    uint8_t ram_percent = 0;
    uint16_t ram_used_mb = 0;
    uint16_t ram_total_mb = 16384;
    uint8_t disk_percent = 0;
    float net_dl_rate = 0.0f;
    float net_ul_rate = 0.0f;
    uint8_t active_cards = 0x07; // Default show all: CPU (bit0), GPU (bit1), SYSTEM (bit2)
    uint8_t cycle_sec = 3;       // Default 3 seconds rotation duration
    uint32_t uptime_sec = 0;
    bool online = false;
    uint32_t last_update = 0;
};

inline bool parse_telemetry_packet(const uint8_t* buffer, size_t len, TelemetryState& state) {
    if (len < 32) return false;
    const TelemetryPacket* packet = reinterpret_cast<const TelemetryPacket*>(buffer);
    
    // Verify magic bytes
    if (packet->magic1 != 0xAA || packet->magic2 != 0x55) return false;
    
    // Verify XOR Checksum
    uint8_t crc = 0;
    for (size_t i = 0; i < 31; ++i) {
        crc ^= buffer[i];
    }
    if (crc != packet->checksum) return false;
    
    // Extract values into the state struct
    state.cpu_load = packet->cpu_load;
    state.cpu_temp = packet->cpu_temp;
    state.cpu_freq = packet->cpu_freq;
    state.gpu_load = packet->gpu_load;
    state.gpu_temp = packet->gpu_temp;
    state.gpu_vram = packet->gpu_vram;
    state.ram_percent = packet->ram_percent;
    state.ram_used_mb = packet->ram_used_mb;
    state.ram_total_mb = packet->ram_total_mb;
    state.disk_percent = packet->disk_percent;
    state.net_dl_rate = packet->net_dl_rate;
    state.net_ul_rate = packet->net_ul_rate;
    state.active_cards = packet->active_cards;
    state.cycle_sec = packet->cycle_sec;
    state.uptime_sec = packet->uptime_sec;
    state.online = true;
    state.last_update = millis();
    
    return true;
}

#endif // TELEMETRY_DATA_H
