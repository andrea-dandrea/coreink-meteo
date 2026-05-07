# M5Stack Unit ENV-III - Documentazione Prodotto

Fonte: https://docs.m5stack.com/en/unit/envIII
SKU: U001-C
Data consultazione: 2026-05-05

## Descrizione

Unit ENV-III è un sensore ambientale che integra SHT30 e QMP6988 per rilevare
temperatura, umidità e pressione atmosferica. Il SHT30 è un sensore digitale di
temperatura e umidità ad alta precisione e basso consumo. Il QMP6988 è un sensore
di pressione assoluta progettato per applicazioni mobili.

## Specifiche Tecniche

| Parametro | Valore |
|-----------|--------|
| Sensore temperatura/umidità | SHT30 |
| Sensore pressione | QMP6988 |
| Range temperatura | -40 ~ 120°C |
| Precisione temperatura | ±0.2°C (range 0~60°C) |
| Range umidità | 10 ~ 90 %RH |
| Precisione umidità | ±2% RH |
| Range pressione | 300 ~ 1100 hPa |
| Risoluzione pressione | 0.06 Pa |
| Precisione pressione | ±3.9 Pa |
| Protocollo comunicazione | I2C |
| Indirizzo I2C SHT30 | 0x44 |
| Indirizzo I2C QMP6988 | 0x70 |
| Temperatura operativa | 0 ~ 60°C |
| Dimensioni | 32.0 x 24.0 x 8.0mm |
| Peso | 6.2g |
| Interfaccia | HY2.0-4P (Grove) |

## PinMap

| GND | 5V | SDA | SCL |
|-----|----|----|-----|
| GND | 5V | (Port A) | (Port A) |

Connesso al CoreInk sulla porta HY2.0-4P: SDA=G32, SCL=G33

## Applicazioni
- Stazioni meteorologiche
- Monitoraggio ambientale
- Monitoraggio depositi granaglie

## Confronto con altre versioni ENV

| | ENV | ENV-II | ENV-III | ENV-IV | ENV HAT-C |
|---|-----|--------|---------|--------|-----------|
| Sensori | DHT12+BMP280 | SHT30+BMP280 | SHT30+QMP6988 | SHT40+BMP280 | BME688 |
| Ind. I2C | DHT12:0x5C BMP280:0x76 | SHT30:0x44 BMP280:0x76 | SHT30:0x44 QMP6988:0x70 | SHT40:0x44 BMP280:0x76 | BME688:0x77 |
| Prec. Temp | ±0.5°C | ±0.2°C | ±0.2°C | ±0.2°C | ±0.5°C |
| Prec. Umid | ±5%RH | ±2%RH | ±2%RH | ±1.8%RH | ±3%RH |

## Datasheet Componenti
- QMP6988: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/enviii/QMP6988%20Datasheet.pdf
- SHT30: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/SHT3x_Datasheet_digital.pdf

## Software Arduino
- Libreria: https://github.com/m5stack/M5Unit-ENV
- Esempio CoreInk: https://github.com/m5stack/M5Unit-ENV/blob/master/examples/Unit_ENVIII_M5Core/Unit_ENVIII_M5Core.ino
