# How to send mock data.

Device Created for mock testing : 40ED98507B4D

`coap-client -m POST coap://coap.thingsboard.cloud:5683/api/v1/{device-token}/telemetry -t json -e "json_data"`

# Data Types
TS (timestamp) is not used in the Thingsboard so it does not matter.

### AI
```Json
{ai:{"n":101,"e1":202,"e2":8,"e3":432,"ts":1698783084259}}
```
### Env
```Json
{env:{"temp":2823,"hum":5475,"atmp":10100,"ts":1698592390263}}
```
### Bat
```Json
{bat:{"v":94,"ts":1698592390264}}
```
### Dev
```Json
{dev:{"imei":"151901930720238","iccid":"2990011928130558473","modV":"mfw_nrf9160_1.3.5","brdV":"thingy91_nrf9160","appV":"1.0.0-development","ts":1698591148339}}
```
### Location
```Json
{gnss:{"lat":40.979099808901054,"lng":29.061921840934104,"spd":10}}
```
