## PTAI Embedded IoT Device

The firmware is based on the [NRF Asset Tracker V2 example](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html), with added support for the Thingsboard IoT Device Management platform that uses CoAP as a communication protocol.

Please check the example and the [docs folder](ai_tag/docs) folder for further information related to the application.

## Usage

Here's how to run it:

1. In the `overlay-thingsboard.conf` file, the hostname, device provision key, and device provision secret should be filled. The provision key and provision secret can be obtained from the Device Profiles in Thingsboard.

2. Because device provision **is currently not supported**, a new device should be created using the 'Add Device' option in the Entities/Devices profile in Thingsboard IoT interface. Device Profile that is used in the previous step should be used here.

3. After the device is created, click on the device and obtain the access token via the 'Manage Credentials' button. Fill in the device token configuration in the overlay file.

4. Add the overlay while building the application.

5. Flash the app, and you are ready to go.