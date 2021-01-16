# LoRaCon Dokumentation

LoRaCon ist eine Arduino Library, mit der man bidirektional verschlüsselte Nachrichten über LoRa senden kann.

## Konfiguration:

### LoRa konfigurieren und starten

LoRaCon basiert auf der LoRa-Library. Um LoRaCon zu verwenden, muss zuerst LoRa initialisiert werden. Hierzu muss der Header eingebunden werden.

```cpp
#include <LoRa.h>
#include "LoRaCon.hpp"

...

SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

if (!LoRa.begin(868E6))
{
    Serial.println("Starting LoRa failed!");
    while (1);
}
```

### LoRaCon initialisieren

Anschließend muss eine LoRaCon-Instanz erstellt werden. Um eine LoRaCon-Instanz zu erstellen braucht man die DeviceIdentity des eigenen Gerätes und die Referenz auf eine Callbackfunktion die aufgerufen wird, wenn Nachrichten empfangen werden.
Eine DeviceIdentity ist ein struct bestehend aus drei Membervariablen: Den Gerätenamen, die Geräte-ID und ein Key zum Verschlüsseln von Nachrichten.

```cpp
DeviceIdentity sensorDevice1 = {
    .name = "Sensor1",
    .id = 10,
    .key = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
};
```

Die Callbackfunktion hat zwei Parameter. Die DeviceIndentity des Nachrichtensenders und die Nachricht als C-String.

```cpp
void msgReceived(DeviceIdentity *from, char *msg)
{
  // Handle received message
}
```

Damit kann die LoRACon Instanz erstellt werden.

```cpp
loraCon = new LoRaCon(&sensorDevice1, &msgReceived);
```

### Verbindungen zu Geräten hinzufügen

Um Nachrichten zu senden und zu empfangen muss noch jeweils eine Connection zu einem Gerät aufgebaut werden. Dazu wird die DeviceIdentity des Empfängergerätes benötigt.

```cpp
DeviceIdentity gatewayDevice = {
    .name = "Gateway",
    .id = 100,
    .key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}
};

...

loraCon->addNewConnection(&gatewayDevice);
```

## Nachrichten senden:

### Sessions

Wird eine Verbindung (Connection) hinzugefügt, wird versucht eine Session zwischen den beiden Geräten aufzubauen. Falls der Aufbau nicht gelingt, wird es nach einiger Zeit erneut versucht.
Erst wenn die Session aufgebaut ist können Nachrichten ausgetauscht werden.

### Message Queues

Alle Nachrichten, die über LoRaCon gesendet werden, kommen vor dem Senden in eine MessageQueue. Die Nachrichten werden dann nach dem First-In-First-Out Prinzip verschickt. Jede Connection hat ihre eigenen Message Queues.

### Senden

Nachrichten können nur an Geräte gesendet werden, die vorher hinzugefügt wurden vgl. Verbindungen zu Geräten hinzufügen). LoRaCon bietet die Möglichkeit zwei Arten von Nachrichten zu senden.
Der erste Typ von Nachrichten sind „Fire-and-Forget“-Nachrichten. Sie sind die Nachrichten mit höchster Priorität. Diese werden sofern es der Duty-Cycle zulässt sobald wie möglich gesendet. „Fire-and-Forget“-Nachrichten werden ein einziges Mal gesendet und sie besitzen keine Empfangsbestätigung.

```cpp
loraCon->sendFAF(100, message);
```

Der zweite Typ von Nachricht sind „ACK“-Nachrichten. Diese Nachrichten haben geringere Priorität als „Fire-and-Forget“-Nachrichten. Diese Nachrichten werden gespeichert und periodisch erneut gesendet, bis der Empfang der Nachricht vom Empfänger mit einem ACK bestätigt wurde.

```cpp
loraCon->sendDAT(100, message);
```

### Duty Cycle

Das Senden auf der Frequenz von LoRa unterliegt Einschränkungen. In Europa darf pro Stunde nur ein Prozent der Zeit (also 36 Sekunden) gesendet werden. Dies beschränkt die Nachrichten die pro Stunde/ Tag versendet werden können. Innerhalb der Library ist eine Funktionalität implementiert, dass der Duty-Cycle nicht überschritten werden kann.
ABER! Wird langfristig versucht mehr Nachrichten zu senden als der Duty-Cycle zulässt, stauen sich diese in den Message-Queues und wenn diese volllaufen werden die ältesten Nachrichten verworfen.

Berechnung der Airtime:

```
Paketgröße (Bytes) * 8 / Bitrate des verwendeten Spreading Factors * 1000 = Airtime (Millisekunden)

SPF7_125kHz = 5470 (Bits/Sekunde)
```

## Nachrichten empfangen:

Alle empfangenen Nachrichten werden an die Callbackfunktion weitergeleitet. Dort können diese dann verarbeitet werden.

```cpp
void msgReceived(DeviceIdentity *from, char *msg)
{
  processMessage(from, msg);
  Serial.println(msg);
}
```
